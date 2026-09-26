#ifndef FIELD_PROVIDER_PERSISTENCE_H
#define FIELD_PROVIDER_PERSISTENCE_H

#include "permission_store.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace field {

inline const char* accountKindName(AccountKind kind)
{
    switch (kind) {
    case AccountKind::Public:
        return "public";
    case AccountKind::Private:
        return "private";
    }

    return "";
}

inline std::optional<AccountKind> parseAccountKind(std::string_view value)
{
    if (value == "public")
        return AccountKind::Public;

    if (value == "private")
        return AccountKind::Private;

    return std::nullopt;
}

inline const char* capabilityName(Capability capability)
{
    switch (capability) {
    case Capability::AccountIdentityRead:
        return "account.identity.read";
    case Capability::AccountBalanceRead:
        return "account.balance.read";
    case Capability::TransactionPropose:
        return "transaction.propose";
    }

    return "";
}

inline std::optional<Capability> parseCapability(std::string_view value)
{
    if (value == "account.identity.read")
        return Capability::AccountIdentityRead;

    if (value == "account.balance.read")
        return Capability::AccountBalanceRead;

    if (value == "transaction.propose")
        return Capability::TransactionPropose;

    return std::nullopt;
}

inline nlohmann::json permissionStoreToJson(const PermissionStore& store)
{
    nlohmann::json root;
    root["schemaVersion"] = 1;
    root["grants"] = nlohmann::json::array();

    for (const PermissionGrant& grant : store.all()) {
        nlohmann::json capabilities = nlohmann::json::array();

        for (const Capability capability : grant.capabilities)
            capabilities.push_back(capabilityName(capability));

        root["grants"].push_back({
            {"callerKey", grant.caller_key},
            {"accountId", grant.account_id},
            {"accountKind", accountKindName(grant.account_kind)},
            {"capabilities", std::move(capabilities)},
            {"revoked", grant.revoked},
        });
    }

    return root;
}

inline std::string permissionStoreToJsonString(
    const PermissionStore& store)
{
    return permissionStoreToJson(store).dump(2);
}

inline std::optional<PermissionStore> permissionStoreFromJson(
    const nlohmann::json& root)
{
    try {
        if (!root.is_object())
            return std::nullopt;

        if (!root.contains("schemaVersion") ||
            !root["schemaVersion"].is_number_integer() ||
            root["schemaVersion"].get<int>() != 1)
            return std::nullopt;

        if (!root.contains("grants") || !root["grants"].is_array())
            return std::nullopt;

        PermissionStore store;

        for (const auto& item : root["grants"]) {
            if (!item.is_object())
                return std::nullopt;

            if (!item.contains("callerKey") ||
                !item["callerKey"].is_string() ||
                !item.contains("accountId") ||
                !item["accountId"].is_string() ||
                !item.contains("accountKind") ||
                !item["accountKind"].is_string() ||
                !item.contains("capabilities") ||
                !item["capabilities"].is_array() ||
                !item.contains("revoked") ||
                !item["revoked"].is_boolean())
                return std::nullopt;

            PermissionGrant grant;
            grant.caller_key = item["callerKey"].get<std::string>();
            grant.account_id = item["accountId"].get<std::string>();
            grant.revoked = item["revoked"].get<bool>();

            const auto kind = parseAccountKind(
                item["accountKind"].get<std::string>());

            if (!kind.has_value())
                return std::nullopt;

            grant.account_kind = *kind;

            for (const auto& value : item["capabilities"]) {
                if (!value.is_string())
                    return std::nullopt;

                const auto capability =
                    parseCapability(value.get<std::string>());

                if (!capability.has_value())
                    return std::nullopt;

                const auto [it, inserted] =
                    grant.capabilities.insert(*capability);

                if (!inserted)
                    return std::nullopt;
            }

            if (grant.caller_key.empty() || grant.account_id.empty())
                return std::nullopt;

            if (store.find(
                    grant.caller_key,
                    grant.account_id).has_value())
                return std::nullopt;

            if (!store.put(std::move(grant)))
                return std::nullopt;
        }

        return store;
    } catch (...) {
        return std::nullopt;
    }
}

inline std::optional<PermissionStore> permissionStoreFromJsonString(
    std::string_view raw)
{
    const nlohmann::json root =
        nlohmann::json::parse(raw, nullptr, false);

    if (root.is_discarded())
        return std::nullopt;

    return permissionStoreFromJson(root);
}

} // namespace field

#endif

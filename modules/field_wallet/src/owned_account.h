#ifndef FIELD_OWNED_ACCOUNT_H
#define FIELD_OWNED_ACCOUNT_H

#include "provider_permissions.h"
#include "provider_validation.h"

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace field {

struct OwnedAccount {
    std::string account_id;
    AccountKind account_kind =
        AccountKind::Public;
};

inline std::optional<std::vector<OwnedAccount>>
parseOwnedAccounts(
    const nlohmann::json& value)
{
    if (!value.is_array())
        return std::nullopt;

    std::vector<OwnedAccount> accounts;
    accounts.reserve(value.size());

    for (const auto& entry : value) {
        if (!entry.is_object() ||
            !entry.contains("account_id") ||
            !entry["account_id"].is_string() ||
            !entry.contains("is_public") ||
            !entry["is_public"].is_boolean()) {
            return std::nullopt;
        }

        const std::string account_id =
            entry["account_id"].get<std::string>();

        if (!isAccountIdHex(account_id))
            return std::nullopt;

        OwnedAccount account;
        account.account_id = account_id;
        account.account_kind =
            entry["is_public"].get<bool>()
                ? AccountKind::Public
                : AccountKind::Private;

        accounts.push_back(
            std::move(account));
    }

    return accounts;
}

inline std::optional<OwnedAccount>
findOwnedAccount(
    const std::vector<OwnedAccount>& accounts,
    const std::string& account_id)
{
    for (const auto& account : accounts) {
        if (account.account_id == account_id)
            return account;
    }

    return std::nullopt;
}

} // namespace field

#endif

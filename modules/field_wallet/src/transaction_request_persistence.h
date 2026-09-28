#ifndef FIELD_TRANSACTION_REQUEST_PERSISTENCE_H
#define FIELD_TRANSACTION_REQUEST_PERSISTENCE_H

#include "transaction_request_store.h"

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace field {

inline const char* transactionKindName(
    TransactionRequestKind kind)
{
    switch (kind) {
    case TransactionRequestKind::PublicNativeTransfer:
        return "public_native_transfer";
    case TransactionRequestKind::PrivateToPublicNativeTransfer:
        return "private_to_public_native_transfer";
    case TransactionRequestKind::PublicToOwnedPrivateNativeTransfer:
        return "public_to_owned_private_native_transfer";
    case TransactionRequestKind::PrivateToOwnedPrivateNativeTransfer:
        return "private_to_owned_private_native_transfer";
    }

    return "";
}

inline std::optional<TransactionRequestKind>
parseTransactionKind(const std::string& value)
{
    if (value == "public_native_transfer")
        return TransactionRequestKind::PublicNativeTransfer;

    if (value == "private_to_public_native_transfer")
        return TransactionRequestKind::
            PrivateToPublicNativeTransfer;

    if (value == "public_to_owned_private_native_transfer")
        return TransactionRequestKind::
            PublicToOwnedPrivateNativeTransfer;

    if (value == "private_to_owned_private_native_transfer")
        return TransactionRequestKind::
            PrivateToOwnedPrivateNativeTransfer;

    return std::nullopt;
}

inline const char* transactionStatusName(
    TransactionRequestStatus status)
{
    switch (status) {
    case TransactionRequestStatus::Pending:
        return "pending";
    case TransactionRequestStatus::Executing:
        return "executing";
    case TransactionRequestStatus::Succeeded:
        return "succeeded";
    case TransactionRequestStatus::Rejected:
        return "rejected";
    case TransactionRequestStatus::ExecutionFailed:
        return "execution_failed";
    case TransactionRequestStatus::Indeterminate:
        return "indeterminate";
    }

    return "";
}

inline std::optional<TransactionRequestStatus>
parseTransactionStatus(const std::string& value)
{
    if (value == "pending")
        return TransactionRequestStatus::Pending;

    if (value == "executing")
        return TransactionRequestStatus::Executing;

    if (value == "succeeded")
        return TransactionRequestStatus::Succeeded;

    if (value == "rejected")
        return TransactionRequestStatus::Rejected;

    if (value == "execution_failed")
        return TransactionRequestStatus::ExecutionFailed;

    if (value == "indeterminate")
        return TransactionRequestStatus::Indeterminate;

    return std::nullopt;
}

inline const char* persistedAccountKindName(
    AccountKind kind)
{
    return kind == AccountKind::Public
        ? "public"
        : "private";
}

inline std::optional<AccountKind>
parsePersistedAccountKind(const std::string& value)
{
    if (value == "public")
        return AccountKind::Public;

    if (value == "private")
        return AccountKind::Private;

    return std::nullopt;
}

inline std::string transactionStoreToJsonString(
    const TransactionRequestStore& store)
{
    nlohmann::json root;
    root["version"] = 1;
    root["requests"] = nlohmann::json::array();

    for (const auto& request : store.all()) {
        root["requests"].push_back({
            {"id", request.id},
            {"kind", transactionKindName(request.kind)},
            {"callerKey", request.caller_key},
            {"moduleName", request.module_name},
            {"moduleInstance", request.module_instance},
            {"accountId", request.account_id},
            {"accountKind",
             persistedAccountKindName(
                 request.account_kind)},
            {"destinationAccountId",
             request.destination_account_id},
            {"amountLe16Hex",
             request.amount_le16_hex},
            {"status",
             transactionStatusName(
                 request.status)},
            {"result", request.result},
        });
    }

    return root.dump();
}

inline std::optional<TransactionRequestStore>
transactionStoreFromJsonString(
    const std::string& raw)
{
    const auto root =
        nlohmann::json::parse(
            raw,
            nullptr,
            false);

    if (root.is_discarded() ||
        !root.is_object() ||
        !root.contains("version") ||
        !root["version"].is_number_integer() ||
        root["version"].get<int>() != 1 ||
        !root.contains("requests") ||
        !root["requests"].is_array()) {
        return std::nullopt;
    }

    TransactionRequestStore store;

    for (const auto& value : root["requests"]) {
        if (!value.is_object())
            return std::nullopt;

        const char* required_strings[] = {
            "kind",
            "callerKey",
            "moduleName",
            "moduleInstance",
            "accountId",
            "accountKind",
            "destinationAccountId",
            "amountLe16Hex",
            "status",
            "result",
        };

        for (const char* key : required_strings) {
            if (!value.contains(key) ||
                !value[key].is_string()) {
                return std::nullopt;
            }
        }

        if (!value.contains("id") ||
            (!value["id"].is_number_unsigned() &&
             !value["id"].is_number_integer())) {
            return std::nullopt;
        }

        std::uint64_t id = 0;

        if (value["id"].is_number_unsigned()) {
            id = value["id"].get<std::uint64_t>();
        } else {
            const auto signed_id =
                value["id"].get<std::int64_t>();

            if (signed_id <= 0)
                return std::nullopt;

            id =
                static_cast<std::uint64_t>(
                    signed_id);
        }

        const auto kind =
            parseTransactionKind(
                value["kind"].get<std::string>());

        const auto status =
            parseTransactionStatus(
                value["status"].get<std::string>());

        const auto account_kind =
            parsePersistedAccountKind(
                value["accountKind"].
                    get<std::string>());

        if (!kind.has_value() ||
            !status.has_value() ||
            !account_kind.has_value()) {
            return std::nullopt;
        }

        TransactionRequest request;
        request.id = id;
        request.kind = *kind;
        request.caller_key =
            value["callerKey"].get<std::string>();
        request.module_name =
            value["moduleName"].get<std::string>();
        request.module_instance =
            value["moduleInstance"].get<std::string>();
        request.account_id =
            value["accountId"].get<std::string>();
        request.account_kind = *account_kind;
        request.destination_account_id =
            value["destinationAccountId"].
                get<std::string>();
        request.amount_le16_hex =
            value["amountLe16Hex"].
                get<std::string>();
        request.status = *status;
        request.result =
            value["result"].get<std::string>();

        if (!store.restore(request))
            return std::nullopt;
    }

    return store;
}

} // namespace field

#endif

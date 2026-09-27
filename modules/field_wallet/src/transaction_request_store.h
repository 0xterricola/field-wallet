#ifndef FIELD_TRANSACTION_REQUEST_STORE_H
#define FIELD_TRANSACTION_REQUEST_STORE_H

#include "provider_permissions.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace field {

enum class TransactionRequestStatus {
    Pending,
    Executing,
    Succeeded,
    Rejected,
    ExecutionFailed
};

enum class TransactionRequestKind {
    PublicNativeTransfer,
    PrivateToPublicNativeTransfer,
    PublicToOwnedPrivateNativeTransfer,
    PrivateToOwnedPrivateNativeTransfer
};

struct TransactionRequest {
    std::uint64_t id = 0;

    TransactionRequestKind kind =
        TransactionRequestKind::PublicNativeTransfer;

    std::string caller_key;
    std::string module_name;
    std::string module_instance;

    std::string account_id;
    AccountKind account_kind = AccountKind::Public;

    std::string destination_account_id;
    std::string amount_le16_hex;

    TransactionRequestStatus status =
        TransactionRequestStatus::Pending;

    std::string result;
};

class TransactionRequestStore {
public:
    std::optional<std::uint64_t> createPublicTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        if (caller_key.empty() ||
            module_name.empty() ||
            account_id.empty() ||
            destination_account_id.empty() ||
            amount_le16_hex.empty()) {
            return std::nullopt;
        }

        const std::uint64_t id = next_id_++;

        TransactionRequest request;
        request.id = id;
        request.kind =
            TransactionRequestKind::PublicNativeTransfer;
        request.caller_key = caller_key;
        request.module_name = module_name;
        request.module_instance = module_instance;
        request.account_id = account_id;
        request.account_kind = AccountKind::Public;
        request.destination_account_id = destination_account_id;
        request.amount_le16_hex = amount_le16_hex;

        requests_.emplace(id, std::move(request));

        return id;
    }

    std::optional<std::uint64_t> createPrivateToPublicTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        if (caller_key.empty() ||
            module_name.empty() ||
            account_id.empty() ||
            destination_account_id.empty() ||
            amount_le16_hex.empty()) {
            return std::nullopt;
        }

        const std::uint64_t id = next_id_++;

        TransactionRequest request;
        request.id = id;
        request.kind =
            TransactionRequestKind::PrivateToPublicNativeTransfer;
        request.caller_key = caller_key;
        request.module_name = module_name;
        request.module_instance = module_instance;
        request.account_id = account_id;
        request.account_kind = AccountKind::Private;
        request.destination_account_id =
            destination_account_id;
        request.amount_le16_hex = amount_le16_hex;

        requests_.emplace(id, std::move(request));

        return id;
    }

    std::optional<std::uint64_t> createPublicToOwnedPrivateTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        if (caller_key.empty() ||
            module_name.empty() ||
            account_id.empty() ||
            destination_account_id.empty() ||
            amount_le16_hex.empty()) {
            return std::nullopt;
        }

        const std::uint64_t id = next_id_++;

        TransactionRequest request;
        request.id = id;
        request.kind =
            TransactionRequestKind::
                PublicToOwnedPrivateNativeTransfer;
        request.caller_key = caller_key;
        request.module_name = module_name;
        request.module_instance = module_instance;
        request.account_id = account_id;
        request.account_kind = AccountKind::Public;
        request.destination_account_id =
            destination_account_id;
        request.amount_le16_hex = amount_le16_hex;

        requests_.emplace(id, std::move(request));

        return id;
    }

    std::optional<std::uint64_t> createPrivateToOwnedPrivateTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        if (caller_key.empty() ||
            module_name.empty() ||
            account_id.empty() ||
            destination_account_id.empty() ||
            amount_le16_hex.empty()) {
            return std::nullopt;
        }

        const std::uint64_t id = next_id_++;

        TransactionRequest request;
        request.id = id;
        request.kind =
            TransactionRequestKind::
                PrivateToOwnedPrivateNativeTransfer;
        request.caller_key = caller_key;
        request.module_name = module_name;
        request.module_instance = module_instance;
        request.account_id = account_id;
        request.account_kind = AccountKind::Private;
        request.destination_account_id =
            destination_account_id;
        request.amount_le16_hex = amount_le16_hex;

        requests_.emplace(id, std::move(request));

        return id;
    }

    std::optional<TransactionRequest> find(
        std::uint64_t id) const
    {
        const auto it = requests_.find(id);

        if (it == requests_.end())
            return std::nullopt;

        return it->second;
    }

    bool beginExecution(
        std::uint64_t id)
    {
        const auto it = requests_.find(id);

        if (it == requests_.end() ||
            it->second.status !=
                TransactionRequestStatus::Pending) {
            return false;
        }

        it->second.status =
            TransactionRequestStatus::Executing;

        return true;
    }

    bool markSucceeded(
        std::uint64_t id,
        const std::string& result)
    {
        const auto it = requests_.find(id);

        if (it == requests_.end() ||
            it->second.status !=
                TransactionRequestStatus::Executing) {
            return false;
        }

        it->second.status =
            TransactionRequestStatus::Succeeded;

        it->second.result = result;

        return true;
    }

    bool markExecutionFailed(
        std::uint64_t id,
        const std::string& result)
    {
        const auto it = requests_.find(id);

        if (it == requests_.end() ||
            it->second.status !=
                TransactionRequestStatus::Executing) {
            return false;
        }

        it->second.status =
            TransactionRequestStatus::ExecutionFailed;

        it->second.result = result;

        return true;
    }

    bool reject(std::uint64_t id)
    {
        const auto it = requests_.find(id);

        if (it == requests_.end() ||
            it->second.status !=
                TransactionRequestStatus::Pending) {
            return false;
        }

        it->second.status =
            TransactionRequestStatus::Rejected;

        return true;
    }

    bool remove(std::uint64_t id)
    {
        return requests_.erase(id) != 0;
    }

    std::size_t size() const
    {
        return requests_.size();
    }

    std::vector<TransactionRequest> all() const
    {
        std::vector<TransactionRequest> result;
        result.reserve(requests_.size());

        for (const auto& [id, request] : requests_)
            result.push_back(request);

        return result;
    }

private:
    std::uint64_t next_id_ = 1;
    std::map<std::uint64_t, TransactionRequest> requests_;
};

} // namespace field

#endif

#ifndef FIELD_TRANSACTION_STATE_H
#define FIELD_TRANSACTION_STATE_H

#include "transaction_request_repository.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace field {

enum class TransactionStateStatus {
    Uninitialized,
    Ready,
    InvalidStore,
    IoError,
};

class TransactionState {
public:
    void initialize(
        const std::filesystem::path& persistence_dir)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        store_ = TransactionRequestStore{};
        repository_.reset();
        status_ =
            TransactionStateStatus::Uninitialized;

        if (persistence_dir.empty())
            return;

        repository_.emplace(
            persistence_dir /
                "transactions.json");

        const TransactionLoadResult result =
            repository_->load();

        switch (result.status) {
        case TransactionLoadStatus::NotFound:
            status_ =
                TransactionStateStatus::Ready;
            return;

        case TransactionLoadStatus::Invalid:
            status_ =
                TransactionStateStatus::InvalidStore;
            return;

        case TransactionLoadStatus::IoError:
            status_ =
                TransactionStateStatus::IoError;
            return;

        case TransactionLoadStatus::Loaded:
            break;
        }

        TransactionRequestStore candidate =
            result.store;

        bool recovered_execution = false;

        for (const auto& request :
             candidate.all()) {
            if (request.status !=
                TransactionRequestStatus::
                    Executing) {
                continue;
            }

            if (!candidate.markIndeterminate(
                    request.id,
                    "recovered_after_restart")) {
                status_ =
                    TransactionStateStatus::
                        InvalidStore;
                return;
            }

            recovered_execution = true;
        }

        if (recovered_execution &&
            !repository_->save(candidate)) {
            status_ =
                TransactionStateStatus::IoError;
            return;
        }

        store_ = std::move(candidate);
        status_ =
            TransactionStateStatus::Ready;
    }

    bool ready() const
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        return status_ ==
            TransactionStateStatus::Ready;
    }

    TransactionStateStatus status() const
    {
        std::lock_guard<std::mutex> lock(
            mutex_);
        return status_;
    }

    std::optional<TransactionRequest> find(
        std::uint64_t id) const
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (status_ !=
            TransactionStateStatus::Ready) {
            return std::nullopt;
        }

        return store_.find(id);
    }

    std::vector<TransactionRequest> all() const
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (status_ !=
            TransactionStateStatus::Ready) {
            return {};
        }

        return store_.all();
    }

    std::optional<std::uint64_t>
    createPublicTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!writable())
            return std::nullopt;

        TransactionRequestStore candidate =
            store_;

        const auto id =
            candidate.createPublicTransfer(
                caller_key,
                module_name,
                module_instance,
                account_id,
                destination_account_id,
                amount_le16_hex);

        if (!id.has_value() ||
            !repository_->save(candidate)) {
            return std::nullopt;
        }

        store_ = std::move(candidate);
        return id;
    }

    std::optional<std::uint64_t>
    createPrivateToPublicTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!writable())
            return std::nullopt;

        TransactionRequestStore candidate =
            store_;

        const auto id =
            candidate.createPrivateToPublicTransfer(
                caller_key,
                module_name,
                module_instance,
                account_id,
                destination_account_id,
                amount_le16_hex);

        if (!id.has_value() ||
            !repository_->save(candidate)) {
            return std::nullopt;
        }

        store_ = std::move(candidate);
        return id;
    }

    std::optional<std::uint64_t>
    createPublicToOwnedPrivateTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!writable())
            return std::nullopt;

        TransactionRequestStore candidate =
            store_;

        const auto id =
            candidate.
                createPublicToOwnedPrivateTransfer(
                    caller_key,
                    module_name,
                    module_instance,
                    account_id,
                    destination_account_id,
                    amount_le16_hex);

        if (!id.has_value() ||
            !repository_->save(candidate)) {
            return std::nullopt;
        }

        store_ = std::move(candidate);
        return id;
    }

    std::optional<std::uint64_t>
    createPrivateToOwnedPrivateTransfer(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!writable())
            return std::nullopt;

        TransactionRequestStore candidate =
            store_;

        const auto id =
            candidate.
                createPrivateToOwnedPrivateTransfer(
                    caller_key,
                    module_name,
                    module_instance,
                    account_id,
                    destination_account_id,
                    amount_le16_hex);

        if (!id.has_value() ||
            !repository_->save(candidate)) {
            return std::nullopt;
        }

        store_ = std::move(candidate);
        return id;
    }

    bool beginExecution(std::uint64_t id)
    {
        return persistMutation(
            [id](TransactionRequestStore& store) {
                return store.beginExecution(id);
            });
    }

    bool markSucceeded(
        std::uint64_t id,
        const std::string& result)
    {
        return persistMutation(
            [&result, id](
                TransactionRequestStore& store) {
                return store.markSucceeded(
                    id,
                    result);
            });
    }

    bool markExecutionFailed(
        std::uint64_t id,
        const std::string& result)
    {
        return persistMutation(
            [&result, id](
                TransactionRequestStore& store) {
                return store.markExecutionFailed(
                    id,
                    result);
            });
    }

    bool markIndeterminate(
        std::uint64_t id,
        const std::string& result)
    {
        return persistMutation(
            [&result, id](
                TransactionRequestStore& store) {
                return store.markIndeterminate(
                    id,
                    result);
            });
    }

    bool reject(std::uint64_t id)
    {
        return persistMutation(
            [id](TransactionRequestStore& store) {
                return store.reject(id);
            });
    }

private:
    bool writable() const
    {
        return status_ ==
                   TransactionStateStatus::Ready &&
               repository_.has_value();
    }

    template <typename Mutation>
    bool persistMutation(Mutation mutation)
    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!writable())
            return false;

        TransactionRequestStore candidate =
            store_;

        if (!mutation(candidate))
            return false;

        if (!repository_->save(candidate))
            return false;

        store_ = std::move(candidate);
        return true;
    }

    mutable std::mutex mutex_;

    TransactionStateStatus status_ =
        TransactionStateStatus::Uninitialized;

    TransactionRequestStore store_;

    std::optional<
        TransactionRequestRepository>
        repository_;
};

} // namespace field

#endif

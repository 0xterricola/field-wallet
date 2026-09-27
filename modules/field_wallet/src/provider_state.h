#ifndef FIELD_PROVIDER_STATE_H
#define FIELD_PROVIDER_STATE_H

#include "permission_repository.h"

#include <filesystem>
#include <optional>
#include <string>

namespace field {

enum class ProviderStateStatus {
    Uninitialized,
    Ready,
    InvalidStore,
    IoError,
};

class ProviderState {
public:
    void initialize(const std::filesystem::path& persistence_dir)
    {
        store_ = PermissionStore{};
        repository_.reset();
        status_ = ProviderStateStatus::Uninitialized;

        if (persistence_dir.empty())
            return;

        repository_.emplace(
            persistence_dir / "permissions.json");

        const PermissionLoadResult result =
            repository_->load();

        switch (result.status) {
        case PermissionLoadStatus::NotFound:
            status_ = ProviderStateStatus::Ready;
            break;

        case PermissionLoadStatus::Loaded:
            store_ = result.store;
            status_ = ProviderStateStatus::Ready;
            break;

        case PermissionLoadStatus::Invalid:
            status_ = ProviderStateStatus::InvalidStore;
            break;

        case PermissionLoadStatus::IoError:
            status_ = ProviderStateStatus::IoError;
            break;
        }
    }

    bool ready() const
    {
        return status_ == ProviderStateStatus::Ready;
    }

    ProviderStateStatus status() const
    {
        return status_;
    }

    const PermissionStore& permissions() const
    {
        return store_;
    }

    bool putPermission(const PermissionGrant& grant)
    {
        if (!ready() || !repository_.has_value())
            return false;

        PermissionStore candidate = store_;

        if (!candidate.put(grant))
            return false;

        if (!repository_->save(candidate))
            return false;

        store_ = std::move(candidate);
        return true;
    }

    bool revokePermission(
        const std::string& caller_key,
        const std::string& account_id)
    {
        if (!ready() || !repository_.has_value())
            return false;

        PermissionStore candidate = store_;

        if (!candidate.revoke(
                caller_key,
                account_id))
            return false;

        if (!repository_->save(candidate))
            return false;

        store_ = std::move(candidate);
        return true;
    }

private:
    ProviderStateStatus status_ =
        ProviderStateStatus::Uninitialized;

    PermissionStore store_;
    std::optional<PermissionRepository> repository_;
};

} // namespace field

#endif

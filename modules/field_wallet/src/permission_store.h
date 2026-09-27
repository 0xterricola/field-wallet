#ifndef FIELD_PERMISSION_STORE_H
#define FIELD_PERMISSION_STORE_H

#include "provider_permissions.h"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace field {

class PermissionStore {
public:
    bool put(PermissionGrant grant)
    {
        if (grant.caller_key.empty() || grant.account_id.empty())
            return false;

        const Key key{grant.caller_key, grant.account_id};
        grants_[key] = std::move(grant);
        return true;
    }

    bool grantCapability(
        const std::string& caller_key,
        const std::string& account_id,
        AccountKind account_kind,
        Capability capability)
    {
        if (caller_key.empty() || account_id.empty())
            return false;

        const Key key{caller_key, account_id};
        const auto it = grants_.find(key);

        if (it == grants_.end()) {
            PermissionGrant grant;
            grant.caller_key = caller_key;
            grant.account_id = account_id;
            grant.account_kind = account_kind;
            grant.capabilities.insert(capability);
            grant.revoked = false;

            grants_.emplace(key, std::move(grant));
            return true;
        }

        PermissionGrant& grant = it->second;

        // The same account ID must never silently change between
        // public and private semantics.
        if (grant.account_kind != account_kind)
            return false;

        grant.capabilities.insert(capability);

        // An explicit new approval re-enables the grant.
        grant.revoked = false;

        return true;
    }

    std::optional<PermissionGrant> find(
        const std::string& caller_key,
        const std::string& account_id) const
    {
        const auto it = grants_.find({caller_key, account_id});

        if (it == grants_.end())
            return std::nullopt;

        return it->second;
    }

    bool allows(
        const std::string& caller_key,
        const std::string& account_id,
        Capability capability) const
    {
        const auto grant = find(caller_key, account_id);

        return grant.has_value() && grant->allows(capability);
    }

    bool revoke(
        const std::string& caller_key,
        const std::string& account_id)
    {
        const auto it = grants_.find({caller_key, account_id});

        if (it == grants_.end())
            return false;

        it->second.revoked = true;
        return true;
    }

    std::size_t size() const
    {
        return grants_.size();
    }

    std::vector<PermissionGrant> all() const
    {
        std::vector<PermissionGrant> result;
        result.reserve(grants_.size());

        for (const auto& [key, grant] : grants_)
            result.push_back(grant);

        return result;
    }

private:
    using Key = std::pair<std::string, std::string>;

    std::map<Key, PermissionGrant> grants_;
};

} // namespace field

#endif

#ifndef FIELD_PROVIDER_ACCESS_H
#define FIELD_PROVIDER_ACCESS_H

#include "permission_store.h"
#include <vector>
#include "provider_authorization.h"
#include "provider_identity.h"

#include <logos_caller.h>

#include <optional>
#include <string>

namespace field {

inline std::optional<PermissionGrant> authorizedGrant(
    const logos::LogosCaller& caller,
    const PermissionStore& store,
    const std::string& account_id,
    Capability capability)
{
    if (!isDappCallerEligible(caller))
        return std::nullopt;

    const auto key = callerKey(caller);

    if (!key.has_value())
        return std::nullopt;

    const auto grant = store.find(*key, account_id);

    if (!grant.has_value())
        return std::nullopt;

    if (!grant->allows(capability))
        return std::nullopt;

    return grant;
}

inline std::vector<PermissionGrant> authorizedGrants(
    const logos::LogosCaller& caller,
    const PermissionStore& store,
    Capability capability)
{
    std::vector<PermissionGrant> result;

    if (!isDappCallerEligible(caller))
        return result;

    const auto caller_key = callerKey(caller);

    if (!caller_key.has_value())
        return result;

    for (const PermissionGrant& grant : store.all()) {
        if (grant.caller_key == *caller_key &&
            grant.allows(capability)) {
            result.push_back(grant);
        }
    }

    return result;
}

inline bool callerHasPermission(
    const logos::LogosCaller& caller,
    const PermissionStore& store,
    const std::string& account_id,
    Capability capability)
{
    return authorizedGrant(
        caller,
        store,
        account_id,
        capability).has_value();
}

} // namespace field

#endif

#ifndef FIELD_PROVIDER_ACCESS_H
#define FIELD_PROVIDER_ACCESS_H

#include "permission_store.h"
#include "provider_authorization.h"
#include "provider_identity.h"

#include <logos_caller.h>

#include <string>

namespace field {

inline bool callerHasPermission(
    const logos::LogosCaller& caller,
    const PermissionStore& store,
    const std::string& account_id,
    Capability capability)
{
    if (!isDappCallerEligible(caller))
        return false;

    const auto key = callerKey(caller);

    if (!key.has_value())
        return false;

    return store.allows(
        *key,
        account_id,
        capability);
}

} // namespace field

#endif

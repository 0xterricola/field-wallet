#ifndef FIELD_PROVIDER_PERMISSIONS_H
#define FIELD_PROVIDER_PERMISSIONS_H

#include <set>
#include <string>

namespace field {

enum class AccountKind {
    Public,
    Private,
};

enum class Capability {
    AccountIdentityRead,
    AccountBalanceRead,
    TransactionPropose,
};

struct PermissionGrant {
    std::string caller_key;
    std::string account_id;
    AccountKind account_kind = AccountKind::Public;
    std::set<Capability> capabilities;
    bool revoked = false;

    bool allows(Capability capability) const
    {
        return !revoked && capabilities.contains(capability);
    }
};

} // namespace field

#endif

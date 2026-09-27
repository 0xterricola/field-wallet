#ifndef FIELD_APPROVAL_AUTHORIZATION_H
#define FIELD_APPROVAL_AUTHORIZATION_H

#include <logos_caller.h>

namespace field {

// Field Wallet's approval surface is the authenticated Field Wallet UI module.
//
// Current Logos UI plugins authenticate outbound calls by module name. They do
// not receive an instanceId, so approval authority cannot currently be bound to
// a specific UI instance. Tighten this if Logos later exposes instance-bound UI
// identity.
inline bool isTrustedApprovalCaller(
    const logos::LogosCaller& caller)
{
    return caller.kind == logos::CallerKind::Module &&
           caller.name == "field_wallet_ui";
}

} // namespace field

#endif

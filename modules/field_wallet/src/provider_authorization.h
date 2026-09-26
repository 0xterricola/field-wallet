#ifndef FIELD_PROVIDER_AUTHORIZATION_H
#define FIELD_PROVIDER_AUTHORIZATION_H

#include <logos_caller.h>

namespace field {

inline bool isDappCallerEligible(const logos::LogosCaller& caller)
{
    return caller.kind == logos::CallerKind::Module &&
           !caller.name.empty();
}

} // namespace field

#endif

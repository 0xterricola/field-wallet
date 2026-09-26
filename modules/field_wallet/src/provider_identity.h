#ifndef FIELD_PROVIDER_IDENTITY_H
#define FIELD_PROVIDER_IDENTITY_H

#include <optional>
#include <string>

#include <logos_caller.h>

namespace field {

inline std::optional<std::string> callerKey(const logos::LogosCaller& caller)
{
    switch (caller.kind) {
    case logos::CallerKind::Module: {
        if (caller.name.empty())
            return std::nullopt;

        std::string key = "module:" + caller.name;

        if (!caller.instance.empty())
            key += ":instance:" + caller.instance;

        return key;
    }

    case logos::CallerKind::Derived:
        if (caller.parent.empty() || caller.leaf.empty())
            return std::nullopt;

        return "derived:" + caller.parent + ":leaf:" + caller.leaf;

    case logos::CallerKind::Operator:
        if (caller.name.empty())
            return std::nullopt;

        return "operator:" + caller.name;

    case logos::CallerKind::Host:
        return "host";

    case logos::CallerKind::Unknown:
    default:
        return std::nullopt;
    }
}

} // namespace field

#endif

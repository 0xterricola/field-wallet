#ifndef FIELD_PROVIDER_IDENTITY_H
#define FIELD_PROVIDER_IDENTITY_H

#include <optional>
#include <string>
#include <string_view>

#include <logos_caller.h>

namespace field {

inline std::string encodeIdentityPart(std::string_view value)
{
    return std::to_string(value.size()) + ":" + std::string(value);
}

inline std::optional<std::string> callerKey(const logos::LogosCaller& caller)
{
    switch (caller.kind) {
    case logos::CallerKind::Module:
        if (caller.name.empty())
            return std::nullopt;

        return "module|" +
               encodeIdentityPart(caller.name) + "|" +
               encodeIdentityPart(caller.instance);

    case logos::CallerKind::Derived:
        if (caller.parent.empty() || caller.leaf.empty())
            return std::nullopt;

        return "derived|" +
               encodeIdentityPart(caller.parent) + "|" +
               encodeIdentityPart(caller.leaf);

    case logos::CallerKind::Operator:
        if (caller.name.empty())
            return std::nullopt;

        return "operator|" + encodeIdentityPart(caller.name);

    case logos::CallerKind::Host:
        return "host";

    case logos::CallerKind::Unknown:
    default:
        return std::nullopt;
    }
}

} // namespace field

#endif

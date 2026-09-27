#ifndef FIELD_PROVIDER_VALIDATION_H
#define FIELD_PROVIDER_VALIDATION_H

#include <cstddef>
#include <string>

namespace field {

inline bool isHexOfLength(
    const std::string& value,
    std::size_t expected_length)
{
    if (value.size() != expected_length)
        return false;

    for (const char c : value) {
        const bool digit =
            c >= '0' && c <= '9';

        const bool lower =
            c >= 'a' && c <= 'f';

        const bool upper =
            c >= 'A' && c <= 'F';

        if (!digit && !lower && !upper)
            return false;
    }

    return true;
}

inline bool isAccountIdHex(
    const std::string& value)
{
    return isHexOfLength(value, 64);
}

inline bool isAmountLe16Hex(
    const std::string& value)
{
    return isHexOfLength(value, 32);
}

} // namespace field

#endif

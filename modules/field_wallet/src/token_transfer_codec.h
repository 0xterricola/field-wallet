#ifndef FIELD_TOKEN_TRANSFER_CODEC_H
#define FIELD_TOKEN_TRANSFER_CODEC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace field {

struct TokenTransferInstruction {
    std::array<std::uint8_t, 16> amount_le{};
};

inline std::optional<TokenTransferInstruction>
decodeTokenTransferInstruction(
    const std::vector<std::uint8_t>& instruction)
{
    // LEZ v0.2.5-rc2 Token instruction wire format:
    //
    //   byte 0      Borsh enum variant tag
    //   bytes 1-16  little-endian u128 amount
    //
    // Transfer is variant 0.
    constexpr std::size_t kTransferInstructionSize = 17;
    constexpr std::uint8_t kTransferVariant = 0;

    if (instruction.size() !=
        kTransferInstructionSize) {
        return std::nullopt;
    }

    if (instruction[0] !=
        kTransferVariant) {
        return std::nullopt;
    }

    TokenTransferInstruction decoded;

    for (std::size_t i = 0;
         i < decoded.amount_le.size();
         ++i) {
        decoded.amount_le[i] =
            instruction[i + 1];
    }

    return decoded;
}

inline std::string tokenAmountLeHex(
    const TokenTransferInstruction& instruction)
{
    static constexpr char kHex[] =
        "0123456789abcdef";

    std::string result;
    result.reserve(
        instruction.amount_le.size() * 2);

    for (const auto byte :
         instruction.amount_le) {
        result.push_back(
            kHex[(byte >> 4) & 0x0f]);
        result.push_back(
            kHex[byte & 0x0f]);
    }

    return result;
}

} // namespace field

#endif

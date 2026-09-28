#ifndef FIELD_TOKEN_HOLDING_CODEC_H
#define FIELD_TOKEN_HOLDING_CODEC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace field {

enum class TokenHoldingKind {
    Fungible,
    NftMaster,
    NftPrintedCopy
};

struct TokenHolding {
    TokenHoldingKind kind = TokenHoldingKind::Fungible;
    std::array<std::uint8_t, 32> definition_id{};
    std::array<std::uint8_t, 16> quantity_le{};
    bool owned = false;
};

inline std::optional<TokenHolding>
decodeTokenHolding(
    const std::vector<std::uint8_t>& data)
{
    constexpr std::size_t kDefinitionOffset = 1;
    constexpr std::size_t kDefinitionSize = 32;
    constexpr std::size_t kValueOffset =
        kDefinitionOffset + kDefinitionSize;

    TokenHolding decoded;

    switch (data.empty() ? 0xff : data[0]) {
    case 0:
        if (data.size() != 49) {
            return std::nullopt;
        }
        decoded.kind = TokenHoldingKind::Fungible;
        break;

    case 1:
        if (data.size() != 49) {
            return std::nullopt;
        }
        decoded.kind = TokenHoldingKind::NftMaster;
        break;

    case 2:
        if (data.size() != 34) {
            return std::nullopt;
        }
        decoded.kind = TokenHoldingKind::NftPrintedCopy;
        break;

    default:
        return std::nullopt;
    }

    for (std::size_t i = 0;
         i < decoded.definition_id.size();
         ++i) {
        decoded.definition_id[i] =
            data[kDefinitionOffset + i];
    }

    if (decoded.kind ==
        TokenHoldingKind::NftPrintedCopy) {
        const auto owned_byte = data[kValueOffset];
        if (owned_byte > 1) {
            return std::nullopt;
        }

        decoded.owned = owned_byte == 1;
        return decoded;
    }

    for (std::size_t i = 0;
         i < decoded.quantity_le.size();
         ++i) {
        decoded.quantity_le[i] =
            data[kValueOffset + i];
    }

    return decoded;
}

} // namespace field

#endif

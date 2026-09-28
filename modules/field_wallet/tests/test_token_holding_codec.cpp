#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <logos_test.h>

#include "token_holding_codec.h"

namespace {

std::vector<std::uint8_t>
bytesFromHex(std::string_view hex)
{
    if ((hex.size() % 2) != 0) {
        throw std::runtime_error(
            "hex input must have even length");
    }

    const auto nibble =
        [](char c) -> std::uint8_t {
            if (c >= '0' && c <= '9') {
                return static_cast<std::uint8_t>(c - '0');
            }
            if (c >= 'a' && c <= 'f') {
                return static_cast<std::uint8_t>(10 + c - 'a');
            }
            if (c >= 'A' && c <= 'F') {
                return static_cast<std::uint8_t>(10 + c - 'A');
            }
            throw std::runtime_error("invalid hex input");
        };

    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2);

    for (std::size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(
            (nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    }

    return bytes;
}

} // namespace

LOGOS_TEST(token_holding_codec_matches_rc2_fungible_vector) {
    const auto decoded =
        field::decodeTokenHolding(bytesFromHex(
            "00111111111111111111111111111111111111111111111111111111111111111100ffeeddccbbaa998877665544332211"));

    LOGOS_ASSERT_TRUE(decoded.has_value());
    LOGOS_ASSERT_TRUE(decoded->kind == field::TokenHoldingKind::Fungible);
    LOGOS_ASSERT_EQ(decoded->definition_id[0], std::uint8_t(0x11));
    LOGOS_ASSERT_EQ(decoded->quantity_le[0], std::uint8_t(0x00));
    LOGOS_ASSERT_EQ(decoded->quantity_le[15], std::uint8_t(0x11));
}

LOGOS_TEST(token_holding_codec_matches_rc2_nft_master_vector) {
    const auto decoded =
        field::decodeTokenHolding(bytesFromHex(
            "01222222222222222222222222222222222222222222222222222222222222222201000000000000000000000000000000"));

    LOGOS_ASSERT_TRUE(decoded.has_value());
    LOGOS_ASSERT_TRUE(decoded->kind == field::TokenHoldingKind::NftMaster);
    LOGOS_ASSERT_EQ(decoded->definition_id[0], std::uint8_t(0x22));
    LOGOS_ASSERT_EQ(decoded->quantity_le[0], std::uint8_t(1));
}

LOGOS_TEST(token_holding_codec_matches_rc2_nft_printed_vector) {
    const auto decoded =
        field::decodeTokenHolding(bytesFromHex(
            "02333333333333333333333333333333333333333333333333333333333333333301"));

    LOGOS_ASSERT_TRUE(decoded.has_value());
    LOGOS_ASSERT_TRUE(decoded->kind == field::TokenHoldingKind::NftPrintedCopy);
    LOGOS_ASSERT_EQ(decoded->definition_id[0], std::uint8_t(0x33));
    LOGOS_ASSERT_TRUE(decoded->owned);
}

LOGOS_TEST(token_holding_codec_rejects_wrong_sizes) {
    LOGOS_ASSERT_FALSE(field::decodeTokenHolding({}).has_value());

    LOGOS_ASSERT_FALSE(field::decodeTokenHolding(
        bytesFromHex("00")).has_value());
}

LOGOS_TEST(token_holding_codec_rejects_unknown_variant) {
    auto data = std::vector<std::uint8_t>(49, 0);
    data[0] = 3;
    LOGOS_ASSERT_FALSE(field::decodeTokenHolding(data).has_value());
}

LOGOS_TEST(token_holding_codec_rejects_invalid_bool) {
    auto data = std::vector<std::uint8_t>(34, 0);
    data[0] = 2;
    data[33] = 2;
    LOGOS_ASSERT_FALSE(field::decodeTokenHolding(data).has_value());
}

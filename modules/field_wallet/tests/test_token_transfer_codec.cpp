#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <logos_test.h>

#include "token_transfer_codec.h"

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
                return static_cast<std::uint8_t>(
                    c - '0');
            }

            if (c >= 'a' && c <= 'f') {
                return static_cast<std::uint8_t>(
                    10 + (c - 'a'));
            }

            if (c >= 'A' && c <= 'F') {
                return static_cast<std::uint8_t>(
                    10 + (c - 'A'));
            }

            throw std::runtime_error(
                "invalid hex input");
        };

    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2);

    for (std::size_t i = 0;
         i < hex.size();
         i += 2) {
        bytes.push_back(
            static_cast<std::uint8_t>(
                (nibble(hex[i]) << 4) |
                nibble(hex[i + 1])));
    }

    return bytes;
}

void assertTransferVector(
    std::string_view encoded_instruction,
    std::string_view expected_amount_le)
{
    const auto decoded =
        field::decodeTokenTransferInstruction(
            bytesFromHex(encoded_instruction));

    LOGOS_ASSERT_TRUE(
        decoded.has_value());

    LOGOS_ASSERT_EQ(
        field::tokenAmountLeHex(*decoded),
        std::string(expected_amount_le));
}

} // namespace

LOGOS_TEST(token_transfer_codec_matches_rc2_zero_vector) {
    assertTransferVector(
        "0000000000000000000000000000000000",
        "00000000000000000000000000000000");
}

LOGOS_TEST(token_transfer_codec_matches_rc2_one_vector) {
    assertTransferVector(
        "0001000000000000000000000000000000",
        "01000000000000000000000000000000");
}

LOGOS_TEST(token_transfer_codec_matches_rc2_500000_vector) {
    assertTransferVector(
        "0020a10700000000000000000000000000",
        "20a10700000000000000000000000000");
}

LOGOS_TEST(token_transfer_codec_matches_rc2_full_width_vector) {
    assertTransferVector(
        "0000ffeeddccbbaa998877665544332211",
        "00ffeeddccbbaa998877665544332211");
}

LOGOS_TEST(token_transfer_codec_rejects_wrong_size) {
    LOGOS_ASSERT_FALSE(
        field::decodeTokenTransferInstruction(
            bytesFromHex(
                "00010000000000000000000000000000"))
            .has_value());

    LOGOS_ASSERT_FALSE(
        field::decodeTokenTransferInstruction(
            bytesFromHex(
                "000100000000000000000000000000000000"))
            .has_value());
}

LOGOS_TEST(token_transfer_codec_rejects_non_transfer_variant) {
    LOGOS_ASSERT_FALSE(
        field::decodeTokenTransferInstruction(
            bytesFromHex(
                "0101000000000000000000000000000000"))
            .has_value());

    LOGOS_ASSERT_FALSE(
        field::decodeTokenTransferInstruction(
            bytesFromHex(
                "ff01000000000000000000000000000000"))
            .has_value());
}

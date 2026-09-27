#include <logos_test.h>

#include "provider_validation.h"

LOGOS_TEST(provider_validation_accepts_valid_account_id_hex) {
    LOGOS_ASSERT_TRUE(
        field::isAccountIdHex(
            std::string(64, 'a')));

    LOGOS_ASSERT_TRUE(
        field::isAccountIdHex(
            std::string(64, 'F')));
}

LOGOS_TEST(provider_validation_rejects_invalid_account_id_length) {
    LOGOS_ASSERT_FALSE(
        field::isAccountIdHex(
            std::string(63, 'a')));

    LOGOS_ASSERT_FALSE(
        field::isAccountIdHex(
            std::string(65, 'a')));
}

LOGOS_TEST(provider_validation_rejects_non_hex_account_id) {
    std::string value(64, 'a');
    value[10] = 'z';

    LOGOS_ASSERT_FALSE(
        field::isAccountIdHex(value));
}

LOGOS_TEST(provider_validation_checks_le16_amount_hex) {
    LOGOS_ASSERT_TRUE(
        field::isAmountLe16Hex(
            std::string(32, '1')));

    LOGOS_ASSERT_FALSE(
        field::isAmountLe16Hex(
            std::string(31, '1')));

    std::string invalid(32, '1');
    invalid[5] = 'x';

    LOGOS_ASSERT_FALSE(
        field::isAmountLe16Hex(invalid));
}

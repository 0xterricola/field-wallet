#include <logos_test.h>

#include "lez_transfer_result.h"

LOGOS_TEST(lez_transfer_result_accepts_success) {
    const auto result =
        field::parseLezTransferResult(
            R"({"success":true,"tx_hash":"abcd","error":""})");

    LOGOS_ASSERT_TRUE(result.has_value());
    LOGOS_ASSERT_TRUE(
        field::isSuccessfulLezTransferResult(*result));

    LOGOS_ASSERT_EQ(
        result->tx_hash,
        std::string("abcd"));
}

LOGOS_TEST(lez_transfer_result_parses_explicit_failure) {
    const auto result =
        field::parseLezTransferResult(
            R"({"success":false,"tx_hash":"","error":"wallet FFI error"})");

    LOGOS_ASSERT_TRUE(result.has_value());

    LOGOS_ASSERT_FALSE(
        field::isSuccessfulLezTransferResult(*result));

    LOGOS_ASSERT_EQ(
        result->error,
        std::string("wallet FFI error"));
}

LOGOS_TEST(lez_transfer_result_rejects_malformed_payload) {
    LOGOS_ASSERT_FALSE(
        field::parseLezTransferResult(
            "not-json").has_value());

    LOGOS_ASSERT_FALSE(
        field::parseLezTransferResult(
            R"({"success":true})").has_value());

    const auto missing_hash =
        field::parseLezTransferResult(
            R"({"success":true,"tx_hash":"","error":""})");

    LOGOS_ASSERT_TRUE(missing_hash.has_value());

    LOGOS_ASSERT_FALSE(
        field::isSuccessfulLezTransferResult(
            *missing_hash));
}

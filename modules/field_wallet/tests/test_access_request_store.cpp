#include <logos_test.h>

#include "access_request_store.h"

LOGOS_TEST(access_request_store_creates_request) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_EQ(
        store.size(),
        static_cast<std::size_t>(1));

    const auto request =
        store.find("module|5:app_a|0:");

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->capabilities.contains(
            field::Capability::AccountIdentityRead));
}

LOGOS_TEST(access_request_store_merges_capabilities) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountBalanceRead));

    const auto request =
        store.find("module|5:app_a|0:");

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->capabilities.contains(
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        request->capabilities.contains(
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_EQ(
        store.size(),
        static_cast<std::size_t>(1));
}

LOGOS_TEST(access_request_store_isolates_callers) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_b|0:",
            "app_b",
            "",
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_EQ(
        store.size(),
        static_cast<std::size_t>(2));
}

LOGOS_TEST(access_request_store_rejects_identity_mismatch) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        store.requestCapability(
            "module|5:app_a|0:",
            "different_app",
            "",
            field::Capability::AccountBalanceRead));

    const auto request =
        store.find("module|5:app_a|0:");

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_FALSE(
        request->capabilities.contains(
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(access_request_store_removes_request) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.remove(
            "module|5:app_a|0:"));

    LOGOS_ASSERT_FALSE(
        store.find(
            "module|5:app_a|0:").has_value());
}

LOGOS_TEST(access_request_store_rejects_empty_identity) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_FALSE(
        store.requestCapability(
            "",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        store.requestCapability(
            "module|0:|0:",
            "",
            "",
            field::Capability::AccountIdentityRead));
}

LOGOS_TEST(access_request_store_removes_one_capability) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_TRUE(
        store.removeCapability(
            "module|5:app_a|0:",
            field::Capability::AccountIdentityRead));

    const auto request =
        store.find("module|5:app_a|0:");

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_FALSE(
        request->capabilities.contains(
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        request->capabilities.contains(
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(access_request_store_removes_request_after_last_capability) {
    field::AccessRequestStore store;

    LOGOS_ASSERT_TRUE(
        store.requestCapability(
            "module|5:app_a|0:",
            "app_a",
            "",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.removeCapability(
            "module|5:app_a|0:",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        store.find(
            "module|5:app_a|0:").has_value());

    LOGOS_ASSERT_EQ(
        store.size(),
        static_cast<std::size_t>(0));
}

#include <logos_test.h>

#include "owned_account.h"

#include <nlohmann/json.hpp>

LOGOS_TEST(owned_account_parses_public_and_private_accounts) {
    const std::string public_id(64, 'a');
    const std::string private_id(64, 'b');

    const nlohmann::json input = {
        {
            {"account_id", public_id},
            {"is_public", true},
        },
        {
            {"account_id", private_id},
            {"is_public", false},
        },
    };

    const auto accounts =
        field::parseOwnedAccounts(input);

    LOGOS_ASSERT_TRUE(accounts.has_value());

    LOGOS_ASSERT_EQ(
        accounts->size(),
        static_cast<std::size_t>(2));

    LOGOS_ASSERT_TRUE(
        (*accounts)[0].account_kind ==
        field::AccountKind::Public);

    LOGOS_ASSERT_TRUE(
        (*accounts)[1].account_kind ==
        field::AccountKind::Private);
}

LOGOS_TEST(owned_account_rejects_invalid_container) {
    const nlohmann::json input = {
        {"account_id", std::string(64, 'a')},
        {"is_public", true},
    };

    LOGOS_ASSERT_FALSE(
        field::parseOwnedAccounts(input)
            .has_value());
}

LOGOS_TEST(owned_account_rejects_invalid_account_id) {
    const nlohmann::json input = {
        {
            {"account_id", "not-an-account-id"},
            {"is_public", true},
        },
    };

    LOGOS_ASSERT_FALSE(
        field::parseOwnedAccounts(input)
            .has_value());
}

LOGOS_TEST(owned_account_finds_account_and_preserves_kind) {
    const std::string public_id(64, 'a');
    const std::string private_id(64, 'b');

    const nlohmann::json input = {
        {
            {"account_id", public_id},
            {"is_public", true},
        },
        {
            {"account_id", private_id},
            {"is_public", false},
        },
    };

    const auto accounts =
        field::parseOwnedAccounts(input);

    LOGOS_ASSERT_TRUE(accounts.has_value());

    const auto found =
        field::findOwnedAccount(
            *accounts,
            private_id);

    LOGOS_ASSERT_TRUE(found.has_value());

    LOGOS_ASSERT_TRUE(
        found->account_kind ==
        field::AccountKind::Private);

    LOGOS_ASSERT_FALSE(
        field::findOwnedAccount(
            *accounts,
            std::string(64, 'c'))
            .has_value());
}

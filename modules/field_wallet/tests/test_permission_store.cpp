#include <logos_test.h>

#include "permission_store.h"

LOGOS_TEST(permission_store_round_trip) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module:app_a";
    grant.account_id = "account_a";
    grant.capabilities.insert(field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_TRUE(store.put(grant));

    const auto stored = store.find("module:app_a", "account_a");

    LOGOS_ASSERT_TRUE(stored.has_value());
    LOGOS_ASSERT_TRUE(
        stored->allows(field::Capability::AccountIdentityRead));
}

LOGOS_TEST(permission_store_isolates_callers) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module:app_a";
    grant.account_id = "account_a";
    grant.capabilities.insert(field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(store.put(grant));

    LOGOS_ASSERT_TRUE(store.allows(
        "module:app_a",
        "account_a",
        field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_FALSE(store.allows(
        "module:app_b",
        "account_a",
        field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_isolates_accounts) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module:app_a";
    grant.account_id = "account_a";
    grant.capabilities.insert(field::Capability::TransactionPropose);

    LOGOS_ASSERT_TRUE(store.put(grant));

    LOGOS_ASSERT_FALSE(store.allows(
        "module:app_a",
        "account_b",
        field::Capability::TransactionPropose));
}

LOGOS_TEST(permission_store_revocation_blocks_access) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module:app_a";
    grant.account_id = "account_a";
    grant.capabilities.insert(field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(store.put(grant));
    LOGOS_ASSERT_TRUE(store.revoke("module:app_a", "account_a"));

    LOGOS_ASSERT_FALSE(store.allows(
        "module:app_a",
        "account_a",
        field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_rejects_incomplete_grants) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.account_id = "account_a";

    LOGOS_ASSERT_FALSE(store.put(grant));

    grant.caller_key = "module:app_a";
    grant.account_id.clear();

    LOGOS_ASSERT_FALSE(store.put(grant));

    LOGOS_ASSERT_EQ(store.size(), static_cast<std::size_t>(0));
}

LOGOS_TEST(permission_store_grant_capability_creates_grant) {
    field::PermissionStore store;

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountIdentityRead));
}

LOGOS_TEST(permission_store_grant_capability_merges_capabilities) {
    field::PermissionStore store;

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_TRUE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_grant_capability_rejects_account_kind_change) {
    field::PermissionStore store;

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Private,
            field::Capability::AccountBalanceRead));

    const auto grant =
        store.find(
            "module|5:app_a|0:",
            "account-a");

    LOGOS_ASSERT_TRUE(grant.has_value());

    LOGOS_ASSERT_TRUE(
        grant->account_kind ==
        field::AccountKind::Public);

    LOGOS_ASSERT_FALSE(
        grant->allows(
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_explicit_reapproval_unrevokes_grant) {
    field::PermissionStore store;

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.revoke(
            "module|5:app_a|0:",
            "account-a"));

    LOGOS_ASSERT_FALSE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            "module|5:app_a|0:",
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_TRUE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountBalanceRead));
}

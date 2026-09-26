#include <logos_test.h>

#include "provider_permissions.h"

#include <string>

LOGOS_TEST(permission_grant_allows_explicit_capability) {
    field::PermissionGrant grant;
    grant.caller_key = "module:example_app";
    grant.account_id = "account-1";
    grant.capabilities.insert(field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_TRUE(
        grant.allows(field::Capability::AccountIdentityRead));
}

LOGOS_TEST(permission_grant_denies_ungranted_capability) {
    field::PermissionGrant grant;
    grant.capabilities.insert(field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::TransactionPropose));
}

LOGOS_TEST(revoked_grant_denies_all_capabilities) {
    field::PermissionGrant grant;
    grant.capabilities.insert(field::Capability::AccountIdentityRead);
    grant.capabilities.insert(field::Capability::AccountBalanceRead);
    grant.capabilities.insert(field::Capability::TransactionPropose);

    grant.revoked = true;

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::AccountBalanceRead));

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::TransactionPropose));
}

LOGOS_TEST(private_account_does_not_imply_balance_permission) {
    field::PermissionGrant grant;
    grant.account_kind = field::AccountKind::Private;
    grant.capabilities.insert(field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_TRUE(
        grant.allows(field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_FALSE(
        grant.allows(field::Capability::AccountBalanceRead));
}

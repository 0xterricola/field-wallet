#include <logos_test.h>

#include "provider_access.h"

namespace {

logos::LogosCaller moduleCaller(
    const std::string& name)
{
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = name;
    return caller;
}

field::PermissionStore grantedStore()
{
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_a|0:";
    grant.account_id = "account-a";
    grant.capabilities.insert(
        field::Capability::AccountBalanceRead);

    store.put(grant);
    return store;
}

} // namespace

LOGOS_TEST(provider_access_allows_matching_grant) {
    const auto store = grantedStore();

    LOGOS_ASSERT_TRUE(
        field::callerHasPermission(
            moduleCaller("app_a"),
            store,
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_access_denies_different_caller) {
    const auto store = grantedStore();

    LOGOS_ASSERT_FALSE(
        field::callerHasPermission(
            moduleCaller("app_b"),
            store,
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_access_denies_different_account) {
    const auto store = grantedStore();

    LOGOS_ASSERT_FALSE(
        field::callerHasPermission(
            moduleCaller("app_a"),
            store,
            "account-b",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_access_denies_ungranted_capability) {
    const auto store = grantedStore();

    LOGOS_ASSERT_FALSE(
        field::callerHasPermission(
            moduleCaller("app_a"),
            store,
            "account-a",
            field::Capability::TransactionPropose));
}

LOGOS_TEST(provider_access_denies_non_dapp_callers) {
    auto store = grantedStore();

    field::PermissionGrant hostGrant;
    hostGrant.caller_key = "host";
    hostGrant.account_id = "account-a";
    hostGrant.capabilities.insert(
        field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(store.put(hostGrant));

    logos::LogosCaller host;
    host.kind = logos::CallerKind::Host;

    LOGOS_ASSERT_FALSE(
        field::callerHasPermission(
            host,
            store,
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_access_returns_authorized_grant) {
    const auto store = grantedStore();

    const auto grant = field::authorizedGrant(
        moduleCaller("app_a"),
        store,
        "account-a",
        field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(grant.has_value());

    LOGOS_ASSERT_TRUE(
        grant->account_kind == field::AccountKind::Public);
}

LOGOS_TEST(provider_access_preserves_private_account_kind) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_a|0:";
    grant.account_id = "private-account";
    grant.account_kind = field::AccountKind::Private;
    grant.capabilities.insert(
        field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(store.put(grant));

    const auto authorized = field::authorizedGrant(
        moduleCaller("app_a"),
        store,
        "private-account",
        field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(authorized.has_value());

    LOGOS_ASSERT_TRUE(
        authorized->account_kind ==
        field::AccountKind::Private);
}

LOGOS_TEST(provider_access_revoked_grant_is_not_authorized) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_a|0:";
    grant.account_id = "account-a";
    grant.capabilities.insert(
        field::Capability::AccountBalanceRead);
    grant.revoked = true;

    LOGOS_ASSERT_TRUE(store.put(grant));

    LOGOS_ASSERT_FALSE(
        field::authorizedGrant(
            moduleCaller("app_a"),
            store,
            "account-a",
            field::Capability::AccountBalanceRead)
            .has_value());
}

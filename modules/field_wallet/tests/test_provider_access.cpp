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

LOGOS_TEST(authorized_grants_returns_only_matching_caller_and_capability) {
    field::PermissionStore store;

    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "app_a";

    const auto caller_key =
        field::callerKey(caller);

    LOGOS_ASSERT_TRUE(caller_key.has_value());

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            *caller_key,
            "account-a",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            *caller_key,
            "account-b",
            field::AccountKind::Private,
            field::Capability::AccountBalanceRead));

    logos::LogosCaller other;
    other.kind = logos::CallerKind::Module;
    other.name = "app_b";

    const auto other_key =
        field::callerKey(other);

    LOGOS_ASSERT_TRUE(other_key.has_value());

    LOGOS_ASSERT_TRUE(
        store.grantCapability(
            *other_key,
            "account-c",
            field::AccountKind::Public,
            field::Capability::AccountIdentityRead));

    const auto grants =
        field::authorizedGrants(
            caller,
            store,
            field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_EQ(
        grants.size(),
        static_cast<std::size_t>(1));

    LOGOS_ASSERT_EQ(
        grants.front().account_id,
        std::string("account-a"));
}

LOGOS_TEST(authorized_grants_rejects_non_dapp_caller) {
    field::PermissionStore store;

    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Host;

    const auto grants =
        field::authorizedGrants(
            caller,
            store,
            field::Capability::AccountIdentityRead);

    LOGOS_ASSERT_TRUE(grants.empty());
}

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

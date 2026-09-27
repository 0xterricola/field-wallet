#include <logos_test.h>

#include "provider_state.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path stateTestDir(const std::string& name)
{
    const auto path =
        std::filesystem::temp_directory_path() /
        "field_wallet_state_tests" /
        name;

    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    std::filesystem::create_directories(path);

    return path;
}

field::PermissionGrant stateTestGrant()
{
    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_a|0:";
    grant.account_id = "account-a";
    grant.account_kind = field::AccountKind::Private;
    grant.capabilities.insert(
        field::Capability::AccountBalanceRead);

    return grant;
}

} // namespace

LOGOS_TEST(provider_state_without_path_is_not_ready) {
    field::ProviderState state;

    state.initialize({});

    LOGOS_ASSERT_FALSE(state.ready());

    LOGOS_ASSERT_TRUE(
        state.status() ==
        field::ProviderStateStatus::Uninitialized);
}

LOGOS_TEST(provider_state_missing_store_starts_ready_and_empty) {
    field::ProviderState state;

    state.initialize(
        stateTestDir("missing-store"));

    LOGOS_ASSERT_TRUE(state.ready());

    LOGOS_ASSERT_EQ(
        state.permissions().size(),
        static_cast<std::size_t>(0));
}

LOGOS_TEST(provider_state_loads_existing_permissions) {
    const auto dir =
        stateTestDir("existing-store");

    field::PermissionStore store;

    LOGOS_ASSERT_TRUE(
        store.put(stateTestGrant()));

    field::PermissionRepository repository(
        dir / "permissions.json");

    LOGOS_ASSERT_TRUE(repository.save(store));

    field::ProviderState state;
    state.initialize(dir);

    LOGOS_ASSERT_TRUE(state.ready());

    LOGOS_ASSERT_TRUE(
        state.permissions().allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_state_corrupt_store_fails_closed) {
    const auto dir =
        stateTestDir("corrupt-store");

    const auto path =
        dir / "permissions.json";

    {
        std::ofstream output(path);
        output << "{ corrupt json";
    }

    field::ProviderState state;
    state.initialize(dir);

    LOGOS_ASSERT_FALSE(state.ready());

    LOGOS_ASSERT_TRUE(
        state.status() ==
        field::ProviderStateStatus::InvalidStore);

    LOGOS_ASSERT_EQ(
        state.permissions().size(),
        static_cast<std::size_t>(0));

    LOGOS_ASSERT_FALSE(
        state.putPermission(
            stateTestGrant()));
}

LOGOS_TEST(provider_state_put_permission_persists_transactionally) {
    const auto dir =
        stateTestDir("transactional-put");

    field::ProviderState first;
    first.initialize(dir);

    LOGOS_ASSERT_TRUE(first.ready());

    LOGOS_ASSERT_TRUE(
        first.putPermission(
            stateTestGrant()));

    field::ProviderState second;
    second.initialize(dir);

    LOGOS_ASSERT_TRUE(second.ready());

    LOGOS_ASSERT_TRUE(
        second.permissions().allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_state_revoke_permission_persists_transactionally) {
    const auto dir =
        stateTestDir("transactional-revoke");

    field::ProviderState first;
    first.initialize(dir);

    LOGOS_ASSERT_TRUE(
        first.putPermission(
            stateTestGrant()));

    LOGOS_ASSERT_TRUE(
        first.revokePermission(
            "module|5:app_a|0:",
            "account-a"));

    field::ProviderState second;
    second.initialize(dir);

    LOGOS_ASSERT_TRUE(second.ready());

    LOGOS_ASSERT_FALSE(
        second.permissions().allows(
            "module|5:app_a|0:",
            "account-a",
            field::Capability::AccountBalanceRead));
}

LOGOS_TEST(provider_state_failed_persistence_does_not_change_memory) {
    const auto root =
        stateTestDir("failed-persistence");

    const auto dir =
        root / "state";

    std::filesystem::create_directories(dir);

    field::ProviderState state;
    state.initialize(dir);

    LOGOS_ASSERT_TRUE(state.ready());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

    {
        std::ofstream blocker(dir);
        blocker << "not a directory";
    }

    LOGOS_ASSERT_FALSE(
        state.putPermission(
            stateTestGrant()));

    LOGOS_ASSERT_EQ(
        state.permissions().size(),
        static_cast<std::size_t>(0));
}

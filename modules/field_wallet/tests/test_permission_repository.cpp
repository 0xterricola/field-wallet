#include <logos_test.h>

#include "permission_repository.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path testPath(const std::string& name)
{
    const auto root =
        std::filesystem::temp_directory_path() /
        "field_wallet_tests";

    std::filesystem::create_directories(root);

    const auto path = root / name;

    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(
        std::filesystem::path(path.string() + ".tmp"),
        ec);

    return path;
}

field::PermissionGrant exampleGrant()
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

LOGOS_TEST(permission_repository_missing_file_is_not_found) {
    const auto path = testPath("missing.json");

    field::PermissionRepository repository(path);

    const auto result = repository.load();

    LOGOS_ASSERT_TRUE(
        result.status == field::PermissionLoadStatus::NotFound);

    LOGOS_ASSERT_EQ(
        result.store.size(),
        static_cast<std::size_t>(0));
}

LOGOS_TEST(permission_repository_save_and_load) {
    const auto path = testPath("round-trip.json");

    field::PermissionStore store;
    LOGOS_ASSERT_TRUE(store.put(exampleGrant()));

    field::PermissionRepository repository(path);

    LOGOS_ASSERT_TRUE(repository.save(store));

    const auto result = repository.load();

    LOGOS_ASSERT_TRUE(
        result.status == field::PermissionLoadStatus::Loaded);

    LOGOS_ASSERT_TRUE(result.store.allows(
        "module|5:app_a|0:",
        "account-a",
        field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_repository_replaces_existing_store) {
    const auto path = testPath("replace.json");

    field::PermissionRepository repository(path);

    field::PermissionStore first;
    LOGOS_ASSERT_TRUE(first.put(exampleGrant()));
    LOGOS_ASSERT_TRUE(repository.save(first));

    field::PermissionStore second;

    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_b|0:";
    grant.account_id = "account-b";
    grant.capabilities.insert(
        field::Capability::TransactionPropose);

    LOGOS_ASSERT_TRUE(second.put(grant));
    LOGOS_ASSERT_TRUE(repository.save(second));

    const auto result = repository.load();

    LOGOS_ASSERT_TRUE(
        result.status == field::PermissionLoadStatus::Loaded);

    LOGOS_ASSERT_EQ(
        result.store.size(),
        static_cast<std::size_t>(1));

    LOGOS_ASSERT_FALSE(result.store.find(
        "module|5:app_a|0:",
        "account-a").has_value());

    LOGOS_ASSERT_TRUE(result.store.allows(
        "module|5:app_b|0:",
        "account-b",
        field::Capability::TransactionPropose));
}

LOGOS_TEST(permission_repository_rejects_corrupt_store) {
    const auto path = testPath("corrupt.json");

    {
        std::ofstream output(path);
        output << "{ definitely not valid json";
    }

    field::PermissionRepository repository(path);

    const auto result = repository.load();

    LOGOS_ASSERT_TRUE(
        result.status == field::PermissionLoadStatus::Invalid);

    LOGOS_ASSERT_EQ(
        result.store.size(),
        static_cast<std::size_t>(0));
}

LOGOS_TEST(permission_repository_leaves_no_temp_file_after_save) {
    const auto path = testPath("atomic.json");

    field::PermissionStore store;
    LOGOS_ASSERT_TRUE(store.put(exampleGrant()));

    field::PermissionRepository repository(path);

    LOGOS_ASSERT_TRUE(repository.save(store));

    const std::filesystem::path temporary =
        path.string() + ".tmp";

    LOGOS_ASSERT_FALSE(
        std::filesystem::exists(temporary));
}

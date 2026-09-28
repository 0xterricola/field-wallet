#include <logos_test.h>

#include "transaction_request_repository.h"
#include "transaction_state.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path cleanDir(
    const std::string& name)
{
    const auto dir =
        std::filesystem::temp_directory_path() /
        name;

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

    return dir;
}

} // namespace

LOGOS_TEST(transaction_repository_round_trips_requests) {
    const auto dir =
        cleanDir(
            "field_transaction_repo_round_trip");

    field::TransactionRequestStore store;

    const auto id =
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01");

    LOGOS_ASSERT_TRUE(id.has_value());
    LOGOS_ASSERT_TRUE(
        store.beginExecution(*id));
    LOGOS_ASSERT_TRUE(
        store.markIndeterminate(
            *id,
            "unknown"));

    field::TransactionRequestRepository repo(
        dir / "transactions.json");

    LOGOS_ASSERT_TRUE(repo.save(store));

    const auto loaded = repo.load();

    LOGOS_ASSERT_TRUE(
        loaded.status ==
        field::TransactionLoadStatus::Loaded);

    const auto request =
        loaded.store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->status ==
        field::TransactionRequestStatus::
            Indeterminate);

    LOGOS_ASSERT_EQ(
        request->result,
        std::string("unknown"));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

LOGOS_TEST(transaction_state_persists_requests_and_ids) {
    const auto dir =
        cleanDir(
            "field_transaction_state_ids");

    field::TransactionState first;
    first.initialize(dir);

    const auto first_id =
        first.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01");

    LOGOS_ASSERT_TRUE(first_id.has_value());
    LOGOS_ASSERT_EQ(
        *first_id,
        static_cast<std::uint64_t>(1));

    field::TransactionState second;
    second.initialize(dir);

    LOGOS_ASSERT_TRUE(
        second.find(*first_id).has_value());

    const auto second_id =
        second.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-c",
            "02");

    LOGOS_ASSERT_TRUE(second_id.has_value());
    LOGOS_ASSERT_EQ(
        *second_id,
        static_cast<std::uint64_t>(2));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

LOGOS_TEST(transaction_state_recovers_executing_as_indeterminate) {
    const auto dir =
        cleanDir(
            "field_transaction_state_recovery");

    field::TransactionState first;
    first.initialize(dir);

    const auto id =
        first.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01");

    LOGOS_ASSERT_TRUE(id.has_value());
    LOGOS_ASSERT_TRUE(
        first.beginExecution(*id));

    field::TransactionState second;
    second.initialize(dir);

    const auto recovered =
        second.find(*id);

    LOGOS_ASSERT_TRUE(
        recovered.has_value());

    LOGOS_ASSERT_TRUE(
        recovered->status ==
        field::TransactionRequestStatus::
            Indeterminate);

    LOGOS_ASSERT_EQ(
        recovered->result,
        std::string(
            "recovered_after_restart"));

    // Recovery itself must also have been persisted.
    field::TransactionState third;
    third.initialize(dir);

    const auto persisted =
        third.find(*id);

    LOGOS_ASSERT_TRUE(
        persisted.has_value());

    LOGOS_ASSERT_TRUE(
        persisted->status ==
        field::TransactionRequestStatus::
            Indeterminate);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

LOGOS_TEST(transaction_state_fails_closed_on_corrupt_store) {
    const auto dir =
        cleanDir(
            "field_transaction_state_corrupt");

    std::filesystem::create_directories(dir);

    {
        std::ofstream output(
            dir / "transactions.json");
        output << "{not-json";
    }

    field::TransactionState state;
    state.initialize(dir);

    LOGOS_ASSERT_TRUE(
        state.status() ==
        field::TransactionStateStatus::
            InvalidStore);

    LOGOS_ASSERT_FALSE(
        state.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01").has_value());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

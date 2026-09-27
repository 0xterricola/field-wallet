#include <logos_test.h>

#include "transaction_request_store.h"

LOGOS_TEST(transaction_request_store_creates_public_transfer) {
    field::TransactionRequestStore store;

    const auto id =
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01000000000000000000000000000000");

    LOGOS_ASSERT_TRUE(id.has_value());

    const auto request =
        store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_EQ(
        request->account_id,
        std::string("account-a"));

    LOGOS_ASSERT_EQ(
        request->destination_account_id,
        std::string("account-b"));

    LOGOS_ASSERT_TRUE(
        request->account_kind ==
        field::AccountKind::Public);
}

LOGOS_TEST(transaction_request_store_assigns_unique_ids) {
    field::TransactionRequestStore store;

    const auto first =
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01");

    const auto second =
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "account-c",
            "02");

    LOGOS_ASSERT_TRUE(first.has_value());
    LOGOS_ASSERT_TRUE(second.has_value());

    LOGOS_ASSERT_FALSE(*first == *second);
}

LOGOS_TEST(transaction_request_store_rejects_missing_fields) {
    field::TransactionRequestStore store;

    LOGOS_ASSERT_FALSE(
        store.createPublicTransfer(
            "",
            "app_a",
            "",
            "account-a",
            "account-b",
            "01").has_value());

    LOGOS_ASSERT_FALSE(
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "",
            "account-b",
            "01").has_value());

    LOGOS_ASSERT_FALSE(
        store.createPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "account-a",
            "",
            "01").has_value());
}

LOGOS_TEST(transaction_request_store_removes_request) {
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
    LOGOS_ASSERT_TRUE(store.remove(*id));
    LOGOS_ASSERT_FALSE(store.find(*id).has_value());
}

LOGOS_TEST(transaction_request_store_marks_request_succeeded) {
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
        store.markSucceeded(
            *id,
            "lez-result"));

    const auto request =
        store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->status ==
        field::TransactionRequestStatus::Succeeded);

    LOGOS_ASSERT_EQ(
        request->result,
        std::string("lez-result"));
}

LOGOS_TEST(transaction_request_store_rejects_pending_request) {
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
    LOGOS_ASSERT_TRUE(store.reject(*id));

    const auto request =
        store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->status ==
        field::TransactionRequestStatus::Rejected);
}

LOGOS_TEST(transaction_request_store_terminal_request_cannot_change_again) {
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
        store.markSucceeded(*id, "done"));

    LOGOS_ASSERT_FALSE(store.reject(*id));

    LOGOS_ASSERT_FALSE(
        store.markSucceeded(*id, "again"));
}

LOGOS_TEST(transaction_request_store_creates_private_to_public_transfer) {
    field::TransactionRequestStore store;

    const auto id =
        store.createPrivateToPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "private-account",
            "public-account",
            "01");

    LOGOS_ASSERT_TRUE(id.has_value());

    const auto request =
        store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->kind ==
        field::TransactionRequestKind::
            PrivateToPublicNativeTransfer);

    LOGOS_ASSERT_TRUE(
        request->account_kind ==
        field::AccountKind::Private);

    LOGOS_ASSERT_EQ(
        request->account_id,
        std::string("private-account"));

    LOGOS_ASSERT_EQ(
        request->destination_account_id,
        std::string("public-account"));
}

LOGOS_TEST(transaction_request_store_rejects_invalid_private_to_public_request) {
    field::TransactionRequestStore store;

    LOGOS_ASSERT_FALSE(
        store.createPrivateToPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "",
            "public-account",
            "01").has_value());

    LOGOS_ASSERT_FALSE(
        store.createPrivateToPublicTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "private-account",
            "",
            "01").has_value());
}

LOGOS_TEST(transaction_request_store_creates_public_to_owned_private_transfer) {
    field::TransactionRequestStore store;

    const auto id =
        store.createPublicToOwnedPrivateTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "public-account",
            "private-account",
            "01");

    LOGOS_ASSERT_TRUE(id.has_value());

    const auto request =
        store.find(*id);

    LOGOS_ASSERT_TRUE(request.has_value());

    LOGOS_ASSERT_TRUE(
        request->kind ==
        field::TransactionRequestKind::
            PublicToOwnedPrivateNativeTransfer);

    LOGOS_ASSERT_TRUE(
        request->account_kind ==
        field::AccountKind::Public);

    LOGOS_ASSERT_EQ(
        request->destination_account_id,
        std::string("private-account"));
}

LOGOS_TEST(transaction_request_store_rejects_invalid_public_to_owned_private_request) {
    field::TransactionRequestStore store;

    LOGOS_ASSERT_FALSE(
        store.createPublicToOwnedPrivateTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "",
            "private-account",
            "01").has_value());

    LOGOS_ASSERT_FALSE(
        store.createPublicToOwnedPrivateTransfer(
            "module|5:app_a|0:",
            "app_a",
            "",
            "public-account",
            "",
            "01").has_value());
}

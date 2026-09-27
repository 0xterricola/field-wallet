#include <logos_test.h>

#include "approval_authorization.h"

LOGOS_TEST(field_wallet_ui_is_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "field_wallet_ui";

    LOGOS_ASSERT_TRUE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(third_party_module_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "some_dapp";

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(similar_module_name_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "field_wallet_ui_fake";

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(host_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Host;

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(operator_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Operator;
    caller.name = "field_wallet_ui";

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(derived_caller_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Derived;
    caller.parent = "field_wallet_ui";
    caller.leaf = "approval";

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

LOGOS_TEST(unknown_caller_is_not_trusted_for_approvals) {
    logos::LogosCaller caller;

    LOGOS_ASSERT_FALSE(
        field::isTrustedApprovalCaller(caller));
}

#include <logos_test.h>

#include "provider_authorization.h"

LOGOS_TEST(module_is_eligible_dapp_caller) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "example_app";

    LOGOS_ASSERT_TRUE(field::isDappCallerEligible(caller));
}

LOGOS_TEST(module_without_name_is_not_eligible) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;

    LOGOS_ASSERT_FALSE(field::isDappCallerEligible(caller));
}

LOGOS_TEST(derived_is_not_eligible_by_default) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Derived;
    caller.parent = "example_app";
    caller.leaf = "window_1";

    LOGOS_ASSERT_FALSE(field::isDappCallerEligible(caller));
}

LOGOS_TEST(operator_and_host_are_not_dapp_callers) {
    logos::LogosCaller caller;

    caller.kind = logos::CallerKind::Operator;
    caller.name = "operator";
    LOGOS_ASSERT_FALSE(field::isDappCallerEligible(caller));

    caller = {};
    caller.kind = logos::CallerKind::Host;
    LOGOS_ASSERT_FALSE(field::isDappCallerEligible(caller));
}

LOGOS_TEST(unknown_is_not_eligible) {
    logos::LogosCaller caller;

    LOGOS_ASSERT_FALSE(field::isDappCallerEligible(caller));
}

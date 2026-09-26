#include <logos_test.h>

#include "provider_identity.h"

#include <string>

LOGOS_TEST(module_caller_key_without_instance) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "example_app";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(*key, std::string("module:example_app"));
}

LOGOS_TEST(module_caller_key_with_instance) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "example_app";
    caller.instance = "abc123";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(
        *key,
        std::string("module:example_app:instance:abc123"));
}

LOGOS_TEST(derived_caller_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Derived;
    caller.parent = "example_app";
    caller.leaf = "window_1";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(
        *key,
        std::string("derived:example_app:leaf:window_1"));
}

LOGOS_TEST(operator_caller_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Operator;
    caller.name = "alice";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(*key, std::string("operator:alice"));
}

LOGOS_TEST(host_caller_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Host;

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(*key, std::string("host"));
}

LOGOS_TEST(unknown_caller_has_no_key) {
    logos::LogosCaller caller;

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT_FALSE(key.has_value());
}

LOGOS_TEST(module_without_name_has_no_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;

    LOGOS_ASSERT_FALSE(field::callerKey(caller).has_value());
}

LOGOS_TEST(derived_without_required_fields_has_no_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Derived;
    caller.parent = "example_app";

    LOGOS_ASSERT_FALSE(field::callerKey(caller).has_value());

    caller.parent.clear();
    caller.leaf = "window_1";

    LOGOS_ASSERT_FALSE(field::callerKey(caller).has_value());
}

LOGOS_TEST(operator_without_name_has_no_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Operator;

    LOGOS_ASSERT_FALSE(field::callerKey(caller).has_value());
}

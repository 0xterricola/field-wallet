#include <logos_test.h>

#include "provider_identity.h"

#include <string>

LOGOS_TEST(module_caller_key_without_instance) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Module;
    caller.name = "example_app";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(*key, std::string("module|11:example_app|0:"));
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
        std::string("module|11:example_app|6:abc123"));
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
        std::string("derived|11:example_app|8:window_1"));
}

LOGOS_TEST(operator_caller_key) {
    logos::LogosCaller caller;
    caller.kind = logos::CallerKind::Operator;
    caller.name = "alice";

    const auto key = field::callerKey(caller);

    LOGOS_ASSERT(key.has_value());
    LOGOS_ASSERT_EQ(*key, std::string("operator|5:alice"));
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


LOGOS_TEST(module_caller_key_encoding_is_unambiguous) {
    logos::LogosCaller first;
    first.kind = logos::CallerKind::Module;
    first.name = "a:instance:b";

    logos::LogosCaller second;
    second.kind = logos::CallerKind::Module;
    second.name = "a";
    second.instance = "b";

    const auto first_key = field::callerKey(first);
    const auto second_key = field::callerKey(second);

    LOGOS_ASSERT(first_key.has_value());
    LOGOS_ASSERT(second_key.has_value());
    LOGOS_ASSERT(*first_key != *second_key);
}

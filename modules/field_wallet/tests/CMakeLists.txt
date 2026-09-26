cmake_minimum_required(VERSION 3.14)
project(FieldWalletTests LANGUAGES CXX)

include(LogosTest)

logos_test(
    NAME field_wallet_tests
    TEST_SOURCES
        main.cpp
        test_provider_identity.cpp
        test_provider_permissions.cpp
        test_provider_authorization.cpp
        test_permission_store.cpp
    EXTRA_INCLUDES
        ../src
)

set_target_properties(
    field_wallet_tests
    PROPERTIES CXX_STANDARD 20
)

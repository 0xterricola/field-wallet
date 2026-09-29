cmake_minimum_required(VERSION 3.21)
project(FieldWalletAccountTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects Test)
enable_testing()
add_executable(field_wallet_account_tests
    test_accounts.cpp
    ../../src/FieldWalletBackend.cpp
    ../../src/FieldWalletBackend.h)
qt6_add_repc_sources(field_wallet_account_tests ../../src/FieldWalletBackend.rep)
target_include_directories(field_wallet_account_tests PRIVATE stubs ../../src "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(field_wallet_account_tests PRIVATE Qt6::Core Qt6::RemoteObjects Qt6::Test)
add_test(NAME field_wallet_account_tests COMMAND field_wallet_account_tests)

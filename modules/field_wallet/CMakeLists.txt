cmake_minimum_required(VERSION 3.14)

project(FieldWallet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

logos_module(
    NAME field_wallet
    SOURCES
        src/field_wallet_module.h
        src/field_wallet_module.cpp
        src/provider_identity.h
        src/provider_permissions.h
)

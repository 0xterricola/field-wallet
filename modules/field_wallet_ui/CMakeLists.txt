cmake_minimum_required(VERSION 3.14)
project(FieldWalletUIPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

logos_module(
    NAME field_wallet_ui
    REP_FILE src/FieldWalletBackend.rep
    SOURCES
        src/FieldWalletPluginInterface.h
        src/FieldWalletPlugin.h
        src/FieldWalletPlugin.cpp
        src/FieldWalletBackend.h
        src/FieldWalletBackend.cpp
)

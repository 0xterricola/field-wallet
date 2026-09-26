#include "field_wallet_module.h"

FieldWalletModule::FieldWalletModule() = default;

FieldWalletModule::~FieldWalletModule() = default;

std::string FieldWalletModule::name() const {
    return "field_wallet";
}

std::string FieldWalletModule::version() const {
    return "0.1.0";
}

std::string FieldWalletModule::lez_core_version() {
    return modules().lez_core.version();
}

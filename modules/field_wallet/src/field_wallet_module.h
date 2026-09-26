#ifndef FIELD_WALLET_MODULE_H
#define FIELD_WALLET_MODULE_H

#include <string>

#include <logos_module_context.h>
#include "logos_sdk.h"

class FieldWalletModule : public LogosModuleContext {
public:
    FieldWalletModule();
    ~FieldWalletModule();

    std::string name() const;
    std::string version() const;

    /// Returns the version reported by the underlying LEZ Core module.
    std::string lez_core_version();
};

#endif // FIELD_WALLET_MODULE_H

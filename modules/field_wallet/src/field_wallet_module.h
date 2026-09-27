#ifndef FIELD_WALLET_MODULE_H
#define FIELD_WALLET_MODULE_H

#include <string>

#include <logos_module_context.h>
#include "logos_sdk.h"
#include "provider_state.h"

class FieldWalletModule : public LogosModuleContext {
public:
    FieldWalletModule();
    ~FieldWalletModule();

    std::string name() const;
    std::string version() const;

    /// Returns the version reported by the underlying LEZ Core module.
    std::string lez_core_version();

    /// Diagnostic representation of the authenticated Logos caller.
    std::string caller_identity();

    /// Read an approved account balance for the authenticated dApp caller.
    std::string provider_get_balance(const std::string& account_id);

    /// Grant one capability after approval by Field Wallet.
    std::string approval_grant_capability(
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id,
        const std::string& account_kind,
        const std::string& capability);

    /// Revoke all capabilities for one approved dApp account binding.
    std::string approval_revoke(
        const std::string& module_name,
        const std::string& module_instance,
        const std::string& account_id);

protected:
    void onContextReady() override;

private:
    field::ProviderState provider_state_;
};

#endif // FIELD_WALLET_MODULE_H

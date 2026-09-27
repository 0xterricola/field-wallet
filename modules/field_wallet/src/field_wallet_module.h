#ifndef FIELD_WALLET_MODULE_H
#define FIELD_WALLET_MODULE_H

#include <string>

#include <logos_module_context.h>
#include "logos_sdk.h"
#include "provider_state.h"
#include "access_request_store.h"
#include "transaction_request_store.h"

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

    /// Request one wallet capability for the authenticated dApp caller.
    std::string provider_request_capability(const std::string& capability);

    /// Return account identities explicitly approved for the authenticated dApp.
    std::string provider_get_accounts();

    /// Read an approved account balance for the authenticated dApp caller.
    std::string provider_get_balance(const std::string& account_id);

    /// Propose a public native transfer for later wallet approval.
    std::string provider_propose_public_transfer(
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex);

    /// List pending dApp capability requests for Field Wallet UI.
    std::string approval_list_requests();

    /// Grant one capability from an authenticated pending dApp request.
    std::string approval_grant_request(
        const std::string& caller_key,
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
    field::AccessRequestStore access_requests_;
    field::TransactionRequestStore transaction_requests_;
};

#endif // FIELD_WALLET_MODULE_H

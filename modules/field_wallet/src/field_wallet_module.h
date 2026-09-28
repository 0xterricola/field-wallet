#ifndef FIELD_WALLET_MODULE_H
#define FIELD_WALLET_MODULE_H

#include <cstdint>
#include <string>

#include <logos_module_context.h>
#include "logos_sdk.h"
#include "provider_state.h"
#include "access_request_store.h"
#include "transaction_state.h"

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


    /// Return Field's trusted wallet lifecycle state.
    std::string wallet_status();

    /// Create Field's default LEZ wallet and return its recovery mnemonic.
    std::string wallet_create(const std::string& password, const std::string& sequencer_addr);

    /// Open an existing LEZ wallet for Field.
    std::string wallet_open(const std::string& config_path, const std::string& storage_path);

    /// List accounts owned by the underlying LEZ wallet for Field Wallet UI.
    std::string wallet_list_accounts();

    /// Read the balance of one account owned by this wallet for Field Wallet UI.
    std::string wallet_get_balance(
        const std::string& account_id);

    /// Create one public account in the underlying LEZ wallet.
    std::string wallet_create_public_account();

    /// Create one private account in the underlying LEZ wallet.
    std::string wallet_create_private_account();

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

    /// Propose a private-to-public native transfer for later wallet approval.
    std::string provider_propose_private_to_public_transfer(
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex);

    /// Propose a public-to-owned-private native transfer for later wallet approval.
    std::string provider_propose_public_to_private_transfer(
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex);

    /// Propose a private-to-owned-private native transfer for later wallet approval.
    std::string provider_propose_private_to_private_transfer(
        const std::string& account_id,
        const std::string& destination_account_id,
        const std::string& amount_le16_hex);

    /// Return the status of a transaction request owned by the authenticated dApp.
    std::string provider_get_transaction_status(
        uint64_t request_id);

    /// List pending dApp capability requests for Field Wallet UI.
    std::string approval_list_requests();

    /// List pending transaction proposals for Field Wallet UI.
    std::string approval_list_transaction_requests();

    /// Reject one pending transaction proposal.
    std::string approval_reject_transaction(
        uint64_t request_id);

    /// Execute one approved pending transaction proposal.
    std::string approval_execute_transaction(
        uint64_t request_id);

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
    bool wallet_open_ = false;

    field::ProviderState provider_state_;
    field::AccessRequestStore access_requests_;
    field::TransactionState transaction_state_;
};

#endif // FIELD_WALLET_MODULE_H

#include "field_wallet_module.h"
#include "provider_identity.h"
#include "provider_access.h"
#include "provider_validation.h"
#include "lez_transfer_result.h"
#include "owned_account.h"
#include "provider_authorization.h"
#include "provider_persistence.h"
#include "approval_authorization.h"

#include <logos_caller.h>
#include <nlohmann/json.hpp>

FieldWalletModule::FieldWalletModule() = default;

FieldWalletModule::~FieldWalletModule() = default;


namespace {

std::optional<std::vector<field::OwnedAccount>>
loadOwnedAccounts(
    LogosModules& logos_modules,
    logos::CallError& error)
{
    const auto raw =
        logos_modules.lez_core.list_accounts(
            &error);

    if (!error.ok())
        return std::nullopt;

    return field::parseOwnedAccounts(raw);
}

} // namespace

std::string FieldWalletModule::name() const {
    return "field_wallet";
}

std::string FieldWalletModule::version() const {
    return "0.1.0";
}

std::string FieldWalletModule::lez_core_version() {
    return modules().lez_core.version();
}


std::string FieldWalletModule::caller_identity() {
    const logos::LogosCaller caller = logos::currentCaller();

    nlohmann::json out;

    if (const auto key = field::callerKey(caller))
        out["key"] = *key;

    switch (caller.kind) {
    case logos::CallerKind::Module:
        out["kind"] = "module";
        out["name"] = caller.name;
        if (!caller.instance.empty())
            out["instance"] = caller.instance;
        break;

    case logos::CallerKind::Derived:
        out["kind"] = "derived";
        out["parent"] = caller.parent;
        out["leaf"] = caller.leaf;
        break;

    case logos::CallerKind::Operator:
        out["kind"] = "operator";
        out["name"] = caller.name;
        break;

    case logos::CallerKind::Host:
        out["kind"] = "host";
        break;

    default:
        out["kind"] = "unknown";
        break;
    }

    return out.dump();
}

void FieldWalletModule::onContextReady() {
    if (!isContextReady() ||
        instancePersistencePath().empty()) {
        return;
    }

    provider_state_.initialize(
        instancePersistencePath());

    transaction_state_.initialize(
        instancePersistencePath());
}

std::string FieldWalletModule::wallet_list_accounts()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    logos::CallError error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            error);

    if (!error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!owned_accounts.has_value()) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    nlohmann::json accounts =
        nlohmann::json::array();

    for (const auto& account :
         *owned_accounts) {
        accounts.push_back({
            {"accountId",
             account.account_id},
            {"accountKind",
             field::accountKindName(
                 account.account_kind)},
        });
    }

    out["ok"] = true;
    out["accounts"] =
        std::move(accounts);

    return out.dump();
}

std::string FieldWalletModule::wallet_get_balance(
    const std::string& account_id)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (!field::isAccountIdHex(account_id)) {
        out["ok"] = false;
        out["code"] = "invalid_account_id";
        return out.dump();
    }

    logos::CallError error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            error);

    if (!error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!owned_accounts.has_value()) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    const auto account =
        field::findOwnedAccount(
            *owned_accounts,
            account_id);

    if (!account.has_value()) {
        out["ok"] = false;
        out["code"] = "account_not_owned";
        return out.dump();
    }

    const bool is_public =
        account->account_kind ==
        field::AccountKind::Public;

    const std::string balance =
        modules().lez_core.get_balance(
            account_id,
            is_public,
            &error);

    if (!error.ok() || balance.empty()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    out["ok"] = true;
    out["accountId"] = account_id;
    out["accountKind"] =
        field::accountKindName(
            account->account_kind);
    out["balance"] = balance;

    return out.dump();
}

std::string FieldWalletModule::wallet_create_public_account()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    logos::CallError error;

    const std::string account_id =
        modules().lez_core.create_account_public(
            &error);

    if (!error.ok() ||
        account_id.empty()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!field::isAccountIdHex(
            account_id)) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    out["ok"] = true;
    out["accountId"] = account_id;
    out["accountKind"] = "public";

    return out.dump();
}

std::string FieldWalletModule::wallet_create_private_account()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    logos::CallError error;

    const std::string account_id =
        modules().lez_core.create_account_private(
            &error);

    if (!error.ok() ||
        account_id.empty()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!field::isAccountIdHex(
            account_id)) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    out["ok"] = true;
    out["accountId"] = account_id;
    out["accountKind"] = "private";

    return out.dump();
}

std::string FieldWalletModule::provider_request_capability(
    const std::string& capability)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isDappCallerEligible(caller)) {
        out["ok"] = false;
        out["code"] = "caller_not_eligible";
        return out.dump();
    }

    const auto caller_key =
        field::callerKey(caller);

    if (!caller_key.has_value()) {
        out["ok"] = false;
        out["code"] = "invalid_caller";
        return out.dump();
    }

    const auto parsed_capability =
        field::parseCapability(capability);

    if (!parsed_capability.has_value()) {
        out["ok"] = false;
        out["code"] = "invalid_capability";
        return out.dump();
    }

    if (!access_requests_.requestCapability(
            *caller_key,
            caller.name,
            caller.instance,
            *parsed_capability)) {
        out["ok"] = false;
        out["code"] = "request_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["capability"] =
        field::capabilityName(*parsed_capability);

    return out.dump();
}

std::string FieldWalletModule::provider_get_accounts()
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isDappCallerEligible(caller)) {
        out["ok"] = false;
        out["code"] = "caller_not_eligible";
        return out.dump();
    }

    const auto grants =
        field::authorizedGrants(
            caller,
            provider_state_.permissions(),
            field::Capability::AccountIdentityRead);

    nlohmann::json accounts =
        nlohmann::json::array();

    for (const auto& grant : grants) {
        accounts.push_back({
            {"accountId", grant.account_id},
            {"accountKind",
             field::accountKindName(grant.account_kind)},
        });
    }

    out["ok"] = true;
    out["accounts"] = std::move(accounts);

    return out.dump();
}

std::string FieldWalletModule::provider_get_balance(
    const std::string& account_id)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    const auto grant = field::authorizedGrant(
        caller,
        provider_state_.permissions(),
        account_id,
        field::Capability::AccountBalanceRead);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    const bool is_public =
        grant->account_kind ==
        field::AccountKind::Public;

    logos::CallError error;

    const std::string balance =
        modules().lez_core.get_balance(
            account_id,
            is_public,
            &error);

    if (!error.ok() || balance.empty()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    out["ok"] = true;
    out["balance"] = balance;

    return out.dump();
}

std::string FieldWalletModule::provider_propose_public_transfer(
    const std::string& account_id,
    const std::string& destination_account_id,
    const std::string& amount_le16_hex)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    const auto grant = field::authorizedGrant(
        caller,
        provider_state_.permissions(),
        account_id,
        field::Capability::TransactionPropose);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    if (grant->account_kind != field::AccountKind::Public) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!field::isAccountIdHex(account_id) ||
        !field::isAccountIdHex(destination_account_id) ||
        !field::isAmountLe16Hex(amount_le16_hex)) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    const auto request_id =
        transaction_state_.createPublicTransfer(
            grant->caller_key,
            caller.name,
            caller.instance,
            account_id,
            destination_account_id,
            amount_le16_hex);

    if (!request_id.has_value()) {
        out["ok"] = false;
        out["code"] = "request_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = *request_id;

    return out.dump();
}

std::string FieldWalletModule::provider_propose_private_to_public_transfer(
    const std::string& account_id,
    const std::string& destination_account_id,
    const std::string& amount_le16_hex)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    const auto grant = field::authorizedGrant(
        caller,
        provider_state_.permissions(),
        account_id,
        field::Capability::TransactionPropose);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    if (grant->account_kind != field::AccountKind::Private) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!field::isAccountIdHex(account_id) ||
        !field::isAccountIdHex(destination_account_id) ||
        !field::isAmountLe16Hex(amount_le16_hex)) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    const auto request_id =
        transaction_state_.createPrivateToPublicTransfer(
            grant->caller_key,
            caller.name,
            caller.instance,
            account_id,
            destination_account_id,
            amount_le16_hex);

    if (!request_id.has_value()) {
        out["ok"] = false;
        out["code"] = "request_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = *request_id;

    return out.dump();
}

std::string FieldWalletModule::provider_propose_public_to_private_transfer(
    const std::string& account_id,
    const std::string& destination_account_id,
    const std::string& amount_le16_hex)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    const auto grant = field::authorizedGrant(
        caller,
        provider_state_.permissions(),
        account_id,
        field::Capability::TransactionPropose);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    if (grant->account_kind != field::AccountKind::Public) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!field::isAccountIdHex(account_id) ||
        !field::isAccountIdHex(destination_account_id) ||
        !field::isAmountLe16Hex(amount_le16_hex)) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    logos::CallError inventory_error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            inventory_error);

    if (!inventory_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!owned_accounts.has_value()) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    const auto destination =
        field::findOwnedAccount(
            *owned_accounts,
            destination_account_id);

    // Keep the dApp-facing failure generic so this
    // endpoint is not an account-inventory oracle.
    if (!destination.has_value() ||
        destination->account_kind !=
            field::AccountKind::Private) {
        out["ok"] = false;
        out["code"] = "invalid_destination";
        return out.dump();
    }

    const auto request_id =
        transaction_state_.createPublicToOwnedPrivateTransfer(
            grant->caller_key,
            caller.name,
            caller.instance,
            account_id,
            destination_account_id,
            amount_le16_hex);

    if (!request_id.has_value()) {
        out["ok"] = false;
        out["code"] = "request_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = *request_id;

    return out.dump();
}

std::string FieldWalletModule::provider_propose_private_to_private_transfer(
    const std::string& account_id,
    const std::string& destination_account_id,
    const std::string& amount_le16_hex)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    const auto grant =
        field::authorizedGrant(
            caller,
            provider_state_.permissions(),
            account_id,
            field::Capability::TransactionPropose);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    if (grant->account_kind !=
        field::AccountKind::Private) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!field::isAccountIdHex(account_id) ||
        !field::isAccountIdHex(
            destination_account_id) ||
        !field::isAmountLe16Hex(
            amount_le16_hex)) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    logos::CallError inventory_error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            inventory_error);

    if (!inventory_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!owned_accounts.has_value()) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    const auto destination =
        field::findOwnedAccount(
            *owned_accounts,
            destination_account_id);

    if (!destination.has_value() ||
        destination->account_kind !=
            field::AccountKind::Private) {
        out["ok"] = false;
        out["code"] = "invalid_destination";
        return out.dump();
    }

    const auto request_id =
        transaction_state_.
            createPrivateToOwnedPrivateTransfer(
                grant->caller_key,
                caller.name,
                caller.instance,
                account_id,
                destination_account_id,
                amount_le16_hex);

    if (!request_id.has_value()) {
        out["ok"] = false;
        out["code"] = "request_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = *request_id;

    return out.dump();
}

std::string FieldWalletModule::provider_get_transaction_status(
    uint64_t request_id)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isDappCallerEligible(caller)) {
        out["ok"] = false;
        out["code"] = "caller_not_eligible";
        return out.dump();
    }

    const auto caller_key =
        field::callerKey(caller);

    if (!caller_key.has_value()) {
        out["ok"] = false;
        out["code"] = "caller_not_eligible";
        return out.dump();
    }

    const auto request =
        transaction_state_.find(request_id);

    // Do not reveal whether another dApp's request ID exists.
    if (!request.has_value() ||
        request->caller_key != *caller_key) {
        out["ok"] = false;
        out["code"] = "request_not_found";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = request_id;

    switch (request->status) {
    case field::TransactionRequestStatus::Pending:
        out["status"] = "pending";
        break;

    case field::TransactionRequestStatus::Executing:
        out["status"] = "executing";
        break;

    case field::TransactionRequestStatus::Succeeded:
        out["status"] = "succeeded";
        out["lezResult"] = request->result;
        break;

    case field::TransactionRequestStatus::Rejected:
        out["status"] = "rejected";
        break;

    case field::TransactionRequestStatus::ExecutionFailed:
        out["status"] = "execution_failed";
        out["lezResult"] = request->result;
        break;

    case field::TransactionRequestStatus::Indeterminate:
        out["status"] = "indeterminate";
        out["lezResult"] = request->result;
        break;
    }

    return out.dump();
}

std::string FieldWalletModule::approval_list_requests()
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    nlohmann::json requests =
        nlohmann::json::array();

    for (const auto& request : access_requests_.all()) {
        nlohmann::json capabilities =
            nlohmann::json::array();

        for (const auto capability : request.capabilities)
            capabilities.push_back(
                field::capabilityName(capability));

        requests.push_back({
            {"callerKey", request.caller_key},
            {"moduleName", request.module_name},
            {"moduleInstance", request.module_instance},
            {"capabilities", std::move(capabilities)},
        });
    }

    out["ok"] = true;
    out["requests"] = std::move(requests);

    return out.dump();
}

std::string FieldWalletModule::approval_list_transaction_requests()
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    nlohmann::json requests =
        nlohmann::json::array();

    for (const auto& request : transaction_state_.all()) {
        if (request.status !=
            field::TransactionRequestStatus::Pending) {
            continue;
        }

        std::string type;

        switch (request.kind) {
        case field::TransactionRequestKind::PublicNativeTransfer:
            type = "public_native_transfer";
            break;

        case field::TransactionRequestKind::PrivateToPublicNativeTransfer:
            type = "private_to_public_native_transfer";
            break;

        case field::TransactionRequestKind::PublicToOwnedPrivateNativeTransfer:
            type = "public_to_private_native_transfer";
            break;

        case field::TransactionRequestKind::PrivateToOwnedPrivateNativeTransfer:
            type = "private_to_private_native_transfer";
            break;
        }

        requests.push_back({
            {"requestId", request.id},
            {"type", type},
            {"callerKey", request.caller_key},
            {"moduleName", request.module_name},
            {"moduleInstance", request.module_instance},
            {"accountId", request.account_id},
            {"accountKind",
             field::accountKindName(request.account_kind)},
            {"destinationAccountId",
             request.destination_account_id},
            {"amountLe16Hex",
             request.amount_le16_hex},
        });
    }

    out["ok"] = true;
    out["requests"] = std::move(requests);

    return out.dump();
}

std::string FieldWalletModule::approval_reject_transaction(
    uint64_t request_id)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    const auto request =
        transaction_state_.find(request_id);

    if (!request.has_value()) {
        out["ok"] = false;
        out["code"] = "request_not_found";
        return out.dump();
    }

    if (request->status !=
        field::TransactionRequestStatus::Pending) {
        out["ok"] = false;
        out["code"] = "request_not_pending";
        return out.dump();
    }

    if (!transaction_state_.reject(request_id)) {
        out["ok"] = false;
        out["code"] = "request_state_failed";
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = request_id;
    out["status"] = "rejected";

    return out.dump();
}

std::string FieldWalletModule::approval_execute_transaction(
    uint64_t request_id)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    const auto request =
        transaction_state_.find(request_id);

    if (!request.has_value()) {
        out["ok"] = false;
        out["code"] = "request_not_found";
        return out.dump();
    }

    if (request->status !=
        field::TransactionRequestStatus::Pending) {
        out["ok"] = false;
        out["code"] = "request_not_pending";
        return out.dump();
    }

    logos::LogosCaller target;
    target.kind = logos::CallerKind::Module;
    target.name = request->module_name;
    target.instance = request->module_instance;

    const auto target_key =
        field::callerKey(target);

    if (!target_key.has_value() ||
        *target_key != request->caller_key) {
        out["ok"] = false;
        out["code"] = "request_identity_mismatch";
        return out.dump();
    }

    const auto grant =
        field::authorizedGrant(
            target,
            provider_state_.permissions(),
            request->account_id,
            field::Capability::TransactionPropose);

    if (!grant.has_value()) {
        out["ok"] = false;
        out["code"] = "permission_denied";
        return out.dump();
    }

    if (grant->account_kind != request->account_kind) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!transaction_state_.beginExecution(
            request_id)) {
        out["ok"] = false;
        out["code"] = "request_not_pending";
        return out.dump();
    }

    logos::CallError error;
    std::string lez_result;

    switch (request->kind) {
    case field::TransactionRequestKind::PublicNativeTransfer:
        lez_result =
            modules().lez_core.transfer_public(
                request->account_id,
                request->destination_account_id,
                request->amount_le16_hex,
                &error);
        break;

    case field::TransactionRequestKind::PrivateToPublicNativeTransfer:
        lez_result =
            modules().lez_core.transfer_deshielded(
                request->account_id,
                request->destination_account_id,
                request->amount_le16_hex,
                &error);
        break;

    case field::TransactionRequestKind::PublicToOwnedPrivateNativeTransfer:
        lez_result =
            modules().lez_core.transfer_shielded_owned(
                request->account_id,
                request->destination_account_id,
                request->amount_le16_hex,
                &error);
        break;

    case field::TransactionRequestKind::PrivateToOwnedPrivateNativeTransfer:
        lez_result =
            modules().lez_core.transfer_private_owned(
                request->account_id,
                request->destination_account_id,
                request->amount_le16_hex,
                &error);
        break;
    }

    if (!error.ok() || lez_result.empty()) {
        transaction_state_.markIndeterminate(
            request_id,
            lez_result);

        out["ok"] = false;
        out["code"] = "execution_indeterminate";
        out["requestId"] = request_id;
        out["status"] = "indeterminate";
        return out.dump();
    }

    const auto parsed =
        field::parseLezTransferResult(
            lez_result);

    if (!parsed.has_value()) {
        transaction_state_.markIndeterminate(
            request_id,
            lez_result);

        out["ok"] = false;
        out["code"] = "execution_indeterminate";
        out["requestId"] = request_id;
        out["status"] = "indeterminate";
        out["lezResult"] = lez_result;
        return out.dump();
    }

    if (!field::isSuccessfulLezTransferResult(
            *parsed)) {
        transaction_state_.markExecutionFailed(
            request_id,
            lez_result);

        out["ok"] = false;
        out["code"] = "lez_transaction_failed";
        out["requestId"] = request_id;
        out["status"] = "execution_failed";
        out["lezResult"] = lez_result;
        return out.dump();
    }

    if (!transaction_state_.markSucceeded(
            request_id,
            lez_result)) {
        out["ok"] = false;
        out["code"] = "request_state_failed";
        out["requestId"] = request_id;
        return out.dump();
    }

    out["ok"] = true;
    out["requestId"] = request_id;
    out["status"] = "succeeded";
    out["txHash"] = parsed->tx_hash;
    out["lezResult"] = lez_result;

    return out.dump();
}

std::string FieldWalletModule::approval_grant_request(
    const std::string& caller_key,
    const std::string& account_id,
    const std::string& account_kind,
    const std::string& capability)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (caller_key.empty() || account_id.empty()) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    const auto kind =
        field::parseAccountKind(account_kind);

    if (!kind.has_value()) {
        out["ok"] = false;
        out["code"] = "invalid_account_kind";
        return out.dump();
    }

    const auto parsed_capability =
        field::parseCapability(capability);

    if (!parsed_capability.has_value()) {
        out["ok"] = false;
        out["code"] = "invalid_capability";
        return out.dump();
    }

    const auto request =
        access_requests_.find(caller_key);

    if (!request.has_value()) {
        out["ok"] = false;
        out["code"] = "request_not_found";
        return out.dump();
    }

    if (!request->capabilities.contains(
            *parsed_capability)) {
        out["ok"] = false;
        out["code"] = "capability_not_requested";
        return out.dump();
    }

    field::AccessRequestStore candidate_requests =
        access_requests_;

    if (!candidate_requests.removeCapability(
            caller_key,
            *parsed_capability)) {
        out["ok"] = false;
        out["code"] = "request_consume_failed";
        return out.dump();
    }

    logos::CallError inventory_error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            inventory_error);

    if (!inventory_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!owned_accounts.has_value()) {
        out["ok"] = false;
        out["code"] = "lez_result_invalid";
        return out.dump();
    }

    const auto owned_account =
        field::findOwnedAccount(
            *owned_accounts,
            account_id);

    if (!owned_account.has_value()) {
        out["ok"] = false;
        out["code"] = "account_not_owned";
        return out.dump();
    }

    const auto requested_kind =
        field::parseAccountKind(
            account_kind);

    if (!requested_kind.has_value() ||
        owned_account->account_kind !=
            *requested_kind) {
        out["ok"] = false;
        out["code"] = "account_kind_mismatch";
        return out.dump();
    }

    if (!provider_state_.grantCapability(
            caller_key,
            account_id,
            *kind,
            *parsed_capability)) {
        out["ok"] = false;
        out["code"] = "permission_update_failed";
        return out.dump();
    }

    access_requests_ =
        std::move(candidate_requests);

    out["ok"] = true;
    out["moduleName"] = request->module_name;
    out["moduleInstance"] = request->module_instance;
    out["accountId"] = account_id;
    out["accountKind"] =
        field::accountKindName(*kind);
    out["capability"] =
        field::capabilityName(*parsed_capability);

    return out.dump();
}

std::string FieldWalletModule::approval_revoke(
    const std::string& module_name,
    const std::string& module_instance,
    const std::string& account_id)
{
    nlohmann::json out;

    if (!provider_state_.ready()) {
        out["ok"] = false;
        out["code"] = "provider_not_ready";
        return out.dump();
    }

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (module_name.empty() || account_id.empty()) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    logos::LogosCaller target;
    target.kind = logos::CallerKind::Module;
    target.name = module_name;
    target.instance = module_instance;

    if (!field::isDappCallerEligible(target)) {
        out["ok"] = false;
        out["code"] = "invalid_target";
        return out.dump();
    }

    const auto target_key =
        field::callerKey(target);

    if (!target_key.has_value()) {
        out["ok"] = false;
        out["code"] = "invalid_target";
        return out.dump();
    }

    if (!provider_state_.revokePermission(
            *target_key,
            account_id)) {
        out["ok"] = false;
        out["code"] = "permission_update_failed";
        return out.dump();
    }

    out["ok"] = true;
    return out.dump();
}

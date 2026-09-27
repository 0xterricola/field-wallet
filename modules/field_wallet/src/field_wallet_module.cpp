#include "field_wallet_module.h"
#include "provider_identity.h"
#include "provider_access.h"
#include "provider_authorization.h"
#include "provider_persistence.h"
#include "approval_authorization.h"

#include <logos_caller.h>
#include <nlohmann/json.hpp>

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

    if (destination_account_id.empty() ||
        amount_le16_hex.empty()) {
        out["ok"] = false;
        out["code"] = "invalid_request";
        return out.dump();
    }

    const auto request_id =
        transaction_requests_.createPublicTransfer(
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

    for (const auto& request : transaction_requests_.all()) {
        requests.push_back({
            {"requestId", request.id},
            {"type", "public_native_transfer"},
            {"callerKey", request.caller_key},
            {"moduleName", request.module_name},
            {"moduleInstance", request.module_instance},
            {"accountId", request.account_id},
            {"accountKind", "public"},
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

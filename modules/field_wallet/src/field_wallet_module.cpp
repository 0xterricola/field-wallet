#include "field_wallet_module.h"
#include "provider_identity.h"
#include "provider_access.h"

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

    if (!error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    out["ok"] = true;
    out["accountId"] = account_id;
    out["accountKind"] =
        is_public ? "public" : "private";
    out["balance"] = balance;

    return out.dump();
}

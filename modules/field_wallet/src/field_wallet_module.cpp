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
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

extern "C" {
#include <wallet_ffi.h>
}


FieldWalletModule::FieldWalletModule() = default;

FieldWalletModule::~FieldWalletModule() = default;


namespace {

std::optional<std::vector<field::OwnedAccount>>
loadOwnedAccounts(
    LogosModules& logos_modules,
    logos::CallError& error,
    const std::string& keycard_account_id)
{
    const auto raw =
        logos_modules.lez_core.list_accounts(
            &error);

    if (!error.ok())
        return std::nullopt;

    auto accounts =
        field::parseOwnedAccounts(raw);

    if (!accounts.has_value())
        return std::nullopt;

    if (!keycard_account_id.empty()) {
        if (!field::isAccountIdHex(
                keycard_account_id)) {
            return std::nullopt;
        }

        if (!field::findOwnedAccount(
                 *accounts,
                 keycard_account_id)
                 .has_value()) {
            field::OwnedAccount keycard_account;
            keycard_account.account_id =
                keycard_account_id;
            keycard_account.account_kind =
                field::AccountKind::Public;

            accounts->push_back(
                std::move(keycard_account));
        }
    }

    return accounts;
}


constexpr int64_t kWalletFfiSuccess = 0;

std::string ffiBytes32ToHex(const FfiBytes32& bytes)
{
    static constexpr char kHex[] = "0123456789abcdef";

    std::string result(64, '0');

    for (size_t i = 0; i < 32; ++i) {
        result[i * 2] = kHex[(bytes.data[i] >> 4) & 0x0f];
        result[i * 2 + 1] = kHex[bytes.data[i] & 0x0f];
    }

    return result;
}

constexpr const char* kDefaultSequencer =
    "https://testnet.lez.logos.co";

struct WalletPaths {
    std::string config;
    std::string storage;
    std::string statistics;
};

bool lezDependencyReady(
    LogosModules& logos_modules,
    logos::CallError& error)
{
    const std::string version =
        logos_modules.lez_core.version(&error);

    return error.ok() && !version.empty();
}

std::optional<WalletPaths> defaultWalletPaths(
    LogosModules& logos_modules,
    logos::CallError& error)
{
    const std::string wallet_dir =
        logos_modules.lez_core.wallet_dir(&error);

    if (!error.ok() ||
        wallet_dir.empty() ||
        wallet_dir == "-") {
        return std::nullopt;
    }

    const std::filesystem::path root(wallet_dir);

    WalletPaths paths;
    paths.config =
        (root / "config.json").string();
    paths.storage =
        (root / "storage.json").string();
    paths.statistics =
        (root / "statistics.json").string();

    return paths;
}

WalletPaths walletPathsInDirectory(
    const std::filesystem::path& root)
{
    WalletPaths paths;

    paths.config =
        (root / "config.json").string();

    paths.storage =
        (root / "storage.json").string();

    paths.statistics =
        (root / "statistics.json").string();

    return paths;
}

std::filesystem::path managedWalletsRoot(
    const WalletPaths& default_paths)
{
    return std::filesystem::path(
        default_paths.config
    ).parent_path() / "wallets";
}

bool isSafeWalletName(
    const std::string& name)
{
    if (name.empty() ||
        name.size() > 48 ||
        name == "default") {
        return false;
    }

    for (const unsigned char value : name) {
        const bool alpha =
            (value >= 'a' && value <= 'z') ||
            (value >= 'A' && value <= 'Z');

        const bool digit =
            value >= '0' && value <= '9';

        if (!alpha &&
            !digit &&
            value != '-' &&
            value != '_') {
            return false;
        }
    }

    return true;
}

WalletPaths namedWalletPaths(
    const WalletPaths& default_paths,
    const std::string& wallet_name)
{
    return walletPathsInDirectory(
        managedWalletsRoot(
            default_paths
        ) / wallet_name
    );
}

std::string statisticsPathFor(
    const std::string& storage_path)
{
    return (
        std::filesystem::path(storage_path)
            .parent_path() /
        "statistics.json"
    ).string();
}

bool writeWalletConfig(
    const std::string& config_path,
    const std::string& sequencer_addr)
{
    nlohmann::json config =
        nlohmann::json::object();

    {
        std::ifstream input(config_path);

        if (input) {
            const nlohmann::json parsed =
                nlohmann::json::parse(
                    input,
                    nullptr,
                    false);

            if (parsed.is_object())
                config = parsed;
        }
    }

    if (config.empty()) {
        config["seq_poll_timeout"] = "30s";
        config["seq_tx_poll_max_blocks"] = 15;
        config["seq_poll_max_retries"] = 10;
        config["seq_block_poll_max_amount"] = 100;
    }

    config["sequencers"] =
        nlohmann::json::array({
            {
                {
                    "sequencer_addr",
                    sequencer_addr.empty()
                        ? kDefaultSequencer
                        : sequencer_addr
                },
                {"basic_auth", nullptr}
            }
        });

    std::error_code ec;

    std::filesystem::create_directories(
        std::filesystem::path(config_path)
            .parent_path(),
        ec);

    if (ec)
        return false;

    std::ofstream output(
        config_path,
        std::ios::trunc);

    if (!output)
        return false;

    output << config.dump(2) << '\n';
    output.flush();

    return output.good();
}

constexpr const char* kRecoveryBackupPendingFile =
    ".field-recovery-backup-pending";

std::filesystem::path recoveryBackupPendingPath(
    const std::string& storage_path)
{
    return std::filesystem::path(storage_path)
        .parent_path() /
        kRecoveryBackupPendingFile;
}

bool writeRecoveryBackupPendingMarker(
    const std::string& storage_path)
{
    const std::filesystem::path marker =
        recoveryBackupPendingPath(storage_path);

    std::ofstream output(
        marker,
        std::ios::trunc);

    if (!output)
        return false;

    output << "pending\n";
    output.flush();

    return output.good();
}

bool recoveryBackupPending(
    const std::string& storage_path)
{
    return std::filesystem::exists(
        recoveryBackupPendingPath(storage_path));
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

bool FieldWalletModule::keycard_available() {
    const bool available = wallet_ffi_keycard_available();

    if (!available) {
        keycard_account_id_.clear();
        keycard_key_path_.clear();
    }

    return available;
}

std::string FieldWalletModule::keycard_get_public_account(
    const std::string& pin,
    const std::string& key_path)
{
    keycard_account_id_.clear();
    keycard_key_path_.clear();

    if (pin.empty() || key_path.empty())
        return {};

    FfiBytes32 account_id{};

    const WalletFfiError error =
        wallet_ffi_keycard_get_public_account(
            pin.c_str(),
            key_path.c_str(),
            &account_id);

    if (static_cast<int64_t>(error) != kWalletFfiSuccess) {
        fprintf(
            stderr,
            "keycard_get_public_account: wallet FFI error %d\n",
            static_cast<int>(error));
        return {};
    }

    keycard_account_id_ =
        ffiBytes32ToHex(account_id);
    keycard_key_path_ = key_path;

    return keycard_account_id_;
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

std::string FieldWalletModule::wallet_status()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError path_error;

    const auto paths =
        defaultWalletPaths(
            modules(),
            path_error);

    if (!path_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!paths.has_value()) {
        out["ok"] = false;
        out["code"] = "wallet_dir_unavailable";
        return out.dump();
    }

    const bool config_exists =
        std::filesystem::exists(
            paths->config);

    const bool storage_exists =
        std::filesystem::exists(
            paths->storage);

    bool managed_wallet_exists = false;

    const std::filesystem::path wallets_root =
        managedWalletsRoot(*paths);

    std::error_code wallets_ec;

    if (std::filesystem::exists(
            wallets_root,
            wallets_ec) &&
        !wallets_ec) {
        std::filesystem::directory_iterator iterator(
            wallets_root,
            wallets_ec);

        const std::filesystem::directory_iterator end;

        while (!wallets_ec && iterator != end) {
            const std::filesystem::directory_entry entry =
                *iterator;

            std::error_code status_error;

            const std::filesystem::file_status status =
                entry.symlink_status(status_error);

            if (!status_error &&
                std::filesystem::is_directory(status) &&
                !std::filesystem::is_symlink(status)) {
                const std::string wallet_name =
                    entry.path().filename().string();

                if (isSafeWalletName(wallet_name)) {
                    const WalletPaths wallet_paths =
                        walletPathsInDirectory(
                            entry.path());

                    if (std::filesystem::exists(
                            wallet_paths.config) &&
                        std::filesystem::exists(
                            wallet_paths.storage)) {
                        managed_wallet_exists = true;
                        break;
                    }
                }
            }

            iterator.increment(wallets_ec);
        }

        if (wallets_ec) {
            out["ok"] = false;
            out["code"] = "wallet_directory_read_failed";
            return out.dump();
        }
    }

    out["ok"] = true;

    if (wallet_open_ &&
        !current_config_path_.empty() &&
        !current_storage_path_.empty()) {
        out["configPath"] = current_config_path_;
        out["storagePath"] = current_storage_path_;
    } else {
        out["configPath"] = paths->config;
        out["storagePath"] = paths->storage;
    }

    const bool recovery_pending =
        config_exists &&
        storage_exists &&
        recoveryBackupPending(
            paths->storage);

    if (recovery_pending) {
        out["state"] = "recovery_pending";
    } else if (wallet_open_) {
        out["state"] = "open";
    } else if ((config_exists &&
                storage_exists) ||
               managed_wallet_exists) {
        out["state"] = "configured";
    } else if (!config_exists &&
               !storage_exists) {
        out["state"] = "uninitialized";
    } else {
        out["state"] = "incomplete";
    }

    return out.dump();
}

std::string FieldWalletModule::wallet_list_saved()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError path_error;

    const auto paths =
        defaultWalletPaths(
            modules(),
            path_error);

    if (!path_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!paths.has_value()) {
        out["ok"] = false;
        out["code"] = "wallet_dir_unavailable";
        return out.dump();
    }

    nlohmann::json wallets =
        nlohmann::json::array();

    if (std::filesystem::exists(paths->config) &&
        std::filesystem::exists(paths->storage)) {
        wallets.push_back({
            {"id", "default"},
            {"name", "LEZ Wallet"},
            {"configPath", paths->config},
            {"storagePath", paths->storage}
        });
    }

    const std::filesystem::path wallets_root =
        managedWalletsRoot(*paths);

    std::error_code ec;

    if (std::filesystem::exists(wallets_root, ec) &&
        !ec) {
        std::vector<std::filesystem::path> directories;

        std::filesystem::directory_iterator iterator(
            wallets_root,
            ec);

        const std::filesystem::directory_iterator end;

        while (!ec && iterator != end) {
            const std::filesystem::directory_entry entry =
                *iterator;

            std::error_code status_error;

            const std::filesystem::file_status status =
                entry.symlink_status(status_error);

            if (!status_error &&
                std::filesystem::is_directory(status) &&
                !std::filesystem::is_symlink(status)) {
                directories.push_back(entry.path());
            }

            iterator.increment(ec);
        }

        if (ec) {
            out["ok"] = false;
            out["code"] = "wallet_directory_read_failed";
            return out.dump();
        }

        std::sort(
            directories.begin(),
            directories.end());

        for (const auto& directory : directories) {
            const std::string wallet_name =
                directory.filename().string();

            if (!isSafeWalletName(wallet_name)) {
                continue;
            }

            const WalletPaths wallet_paths =
                walletPathsInDirectory(directory);

            if (!std::filesystem::exists(
                    wallet_paths.config) ||
                !std::filesystem::exists(
                    wallet_paths.storage)) {
                continue;
            }

            wallets.push_back({
                {"id", wallet_name},
                {"name", wallet_name},
                {"configPath", wallet_paths.config},
                {"storagePath", wallet_paths.storage}
            });
        }
    }

    out["ok"] = true;
    out["wallets"] = wallets;

    return out.dump();
}

std::string FieldWalletModule::wallet_create_named(
    const std::string& wallet_name,
    const std::string& password,
    const std::string& sequencer_addr)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (!isSafeWalletName(wallet_name)) {
        out["ok"] = false;
        out["code"] = "invalid_wallet_name";
        return out.dump();
    }

    if (password.empty()) {
        out["ok"] = false;
        out["code"] = "password_required";
        return out.dump();
    }

    // LEZ 0.3 exposes one live wallet handle and no close/reset operation.
    // Do not let Field clear its own state while LEZ still owns that handle.
    if (wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_create_requires_restart";
        out["restartRequired"] = true;
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError path_error;

    const auto default_paths =
        defaultWalletPaths(
            modules(),
            path_error);

    if (!path_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!default_paths.has_value()) {
        out["ok"] = false;
        out["code"] = "wallet_dir_unavailable";
        return out.dump();
    }

    const WalletPaths paths =
        namedWalletPaths(
            *default_paths,
            wallet_name);

    if (std::filesystem::exists(paths.config) ||
        std::filesystem::exists(paths.storage)) {
        out["ok"] = false;
        out["code"] = "wallet_already_exists";
        out["walletId"] = wallet_name;
        out["storagePath"] = paths.storage;
        return out.dump();
    }

    if (!writeWalletConfig(
            paths.config,
            sequencer_addr)) {
        out["ok"] = false;
        out["code"] = "config_write_failed";
        return out.dump();
    }

    if (!writeRecoveryBackupPendingMarker(
            paths.storage)) {
        std::error_code ignored;

        std::filesystem::remove(
            paths.config,
            ignored);

        out["ok"] = false;
        out["code"] =
            "recovery_marker_write_failed";
        return out.dump();
    }

    const bool had_previous_wallet =
        wallet_open_;

    const std::string previous_config =
        current_config_path_;

    const std::string previous_storage =
        current_storage_path_;

    if (wallet_open_) {
        logos::CallError save_error;

        const int64_t save_result =
            modules().lez_core.save(
                &save_error);

        if (!save_error.ok() ||
            save_result != kWalletFfiSuccess) {
            std::error_code ignored;
            std::filesystem::remove(
                paths.config,
                ignored);

            out["ok"] = false;
            out["code"] =
                "current_wallet_save_failed";
            return out.dump();
        }

        wallet_open_ = false;
        current_config_path_.clear();
        current_storage_path_.clear();
    }

    logos::CallError create_error;

    const std::string mnemonic =
        modules().lez_core.create_new(
            paths.config,
            paths.storage,
            paths.statistics,
            password,
            &create_error,
            120000);

    if (!create_error.ok() ||
        mnemonic.empty()) {
        {
            std::error_code ignored;

            std::filesystem::remove(
                recoveryBackupPendingPath(
                    paths.storage),
                ignored);
        }


        std::error_code ignored;

        std::filesystem::remove(
            paths.config,
            ignored);

        std::filesystem::remove(
            paths.storage,
            ignored);

        std::filesystem::remove(
            paths.statistics,
            ignored);

        bool recovered = false;

        if (had_previous_wallet &&
            !previous_config.empty() &&
            !previous_storage.empty()) {
            logos::CallError recovery_error;

            const int64_t recovery_result =
                modules().lez_core.open(
                    previous_config,
                    previous_storage,
                    statisticsPathFor(
                        previous_storage),
                    &recovery_error);

            if (recovery_error.ok() &&
                recovery_result ==
                    kWalletFfiSuccess) {
                wallet_open_ = true;
                current_config_path_ =
                    previous_config;
                current_storage_path_ =
                    previous_storage;
                recovered = true;
            }
        }

        out["ok"] = false;
        out["code"] = "wallet_create_failed";
        out["recoveredPreviousWallet"] =
            recovered;
        return out.dump();
    }

    wallet_open_ = true;
    current_config_path_ = paths.config;
    current_storage_path_ = paths.storage;

    logos::CallError save_error;

    const int64_t save_result =
        modules().lez_core.save(
            &save_error);

    if (!save_error.ok() ||
        save_result != kWalletFfiSuccess ||
        !std::filesystem::exists(
            paths.storage)) {
        out["ok"] = false;
        out["code"] = "wallet_save_failed";
        out["walletOpen"] = true;
        out["walletId"] = wallet_name;
        out["configPath"] = paths.config;
        out["storagePath"] = paths.storage;

        // Creation already succeeded. Never discard
        // recovery material just because persistence
        // verification failed afterward.
        out["mnemonic"] = mnemonic;

        return out.dump();
    }

    out["ok"] = true;
    out["state"] = "open";
    out["walletId"] = wallet_name;
    out["mnemonic"] = mnemonic;
    out["configPath"] = paths.config;
    out["storagePath"] = paths.storage;

    return out.dump();
}

std::string FieldWalletModule::wallet_restore_named(
    const std::string& wallet_name,
    const std::string& mnemonic,
    const std::string& password,
    const std::string& sequencer_addr)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (!isSafeWalletName(wallet_name)) {
        out["ok"] = false;
        out["code"] = "invalid_wallet_name";
        return out.dump();
    }

    if (mnemonic.empty()) {
        out["ok"] = false;
        out["code"] = "mnemonic_required";
        return out.dump();
    }

    if (password.empty()) {
        out["ok"] = false;
        out["code"] = "password_required";
        return out.dump();
    }

    // The current LEZ lifecycle API has one live wallet handle and does
    // not expose a close operation. Never overwrite an already-open wallet.
    if (wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_already_open";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError path_error;

    const auto default_paths =
        defaultWalletPaths(
            modules(),
            path_error);

    if (!path_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!default_paths.has_value()) {
        out["ok"] = false;
        out["code"] = "wallet_dir_unavailable";
        return out.dump();
    }

    const WalletPaths paths =
        namedWalletPaths(
            *default_paths,
            wallet_name);

    if (std::filesystem::exists(paths.config) ||
        std::filesystem::exists(paths.storage)) {
        out["ok"] = false;
        out["code"] = "wallet_already_exists";
        out["walletId"] = wallet_name;
        out["storagePath"] = paths.storage;
        return out.dump();
    }

    if (!writeWalletConfig(
            paths.config,
            sequencer_addr)) {
        out["ok"] = false;
        out["code"] = "config_write_failed";
        return out.dump();
    }

    logos::CallError create_error;

    // restore_storage() operates on an existing wallet handle, so create
    // the destination vault first. Its generated mnemonic is intentionally
    // discarded after the supplied recovery phrase replaces the keychain.
    const std::string generated_mnemonic =
        modules().lez_core.create_new(
            paths.config,
            paths.storage,
            paths.statistics,
            password,
            &create_error,
            120000);

    if (!create_error.ok() ||
        generated_mnemonic.empty()) {
        std::error_code ignored;

        std::filesystem::remove(
            paths.config,
            ignored);
        std::filesystem::remove(
            paths.storage,
            ignored);
        std::filesystem::remove(
            paths.statistics,
            ignored);
        std::filesystem::remove(
            std::filesystem::path(paths.config)
                .parent_path(),
            ignored);

        out["ok"] = false;
        out["code"] = "wallet_create_failed";
        return out.dump();
    }

    logos::CallError restore_error;

    constexpr uint32_t kRestoreDepth = 5;

    const int64_t restore_result =
        modules().lez_core.restore_storage(
            mnemonic,
            password,
            kRestoreDepth,
            &restore_error,
            120000);

    if (!restore_error.ok() ||
        restore_result != kWalletFfiSuccess) {
        // LEZ currently provides no close operation. Remove the failed
        // vault from disk and require a process restart before retrying,
        // because the temporary wallet handle still exists in LEZ.
        std::error_code ignored;

        std::filesystem::remove(
            paths.config,
            ignored);
        std::filesystem::remove(
            paths.storage,
            ignored);
        std::filesystem::remove(
            paths.statistics,
            ignored);
        std::filesystem::remove(
            std::filesystem::path(paths.config)
                .parent_path(),
            ignored);

        out["ok"] = false;
        out["code"] = "wallet_restore_failed";
        out["ffiCode"] = restore_result;
        out["restartRequired"] = true;
        return out.dump();
    }

    wallet_open_ = true;
    current_config_path_ = paths.config;
    current_storage_path_ = paths.storage;

    // Match the upstream recovery test: after rebuilding keys, sync the
    // restored wallet against current chain state so balances can recover.
    bool sync_complete = false;

    logos::CallError height_error;

    const int64_t current_height =
        modules().lez_core.get_current_block_height(
            &height_error,
            120000);

    if (height_error.ok()) {
        logos::CallError sync_error;

        const int64_t sync_result =
            modules().lez_core.sync_to_block(
                current_height,
                &sync_error,
                120000);

        sync_complete =
            sync_error.ok() &&
            sync_result == kWalletFfiSuccess;
    }

    logos::CallError save_error;

    const int64_t save_result =
        modules().lez_core.save(
            &save_error);

    if (!save_error.ok() ||
        save_result != kWalletFfiSuccess ||
        !std::filesystem::exists(paths.storage)) {
        out["ok"] = false;
        out["code"] = "wallet_save_failed";
        out["walletOpen"] = true;
        out["walletId"] = wallet_name;
        out["configPath"] = paths.config;
        out["storagePath"] = paths.storage;
        return out.dump();
    }

    out["ok"] = true;
    out["state"] = "open";
    out["walletId"] = wallet_name;
    out["configPath"] = paths.config;
    out["storagePath"] = paths.storage;
    out["syncComplete"] = sync_complete;

    return out.dump();
}


std::string FieldWalletModule::wallet_create(
    const std::string& password,
    const std::string& sequencer_addr)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_already_open";
        return out.dump();
    }

    if (password.empty()) {
        out["ok"] = false;
        out["code"] = "password_required";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError path_error;

    const auto paths =
        defaultWalletPaths(
            modules(),
            path_error);

    if (!path_error.ok()) {
        out["ok"] = false;
        out["code"] = "lez_error";
        return out.dump();
    }

    if (!paths.has_value()) {
        out["ok"] = false;
        out["code"] = "wallet_dir_unavailable";
        return out.dump();
    }

    if (std::filesystem::exists(
            paths->storage)) {
        out["ok"] = false;
        out["code"] = "wallet_already_exists";
        out["storagePath"] =
            paths->storage;
        return out.dump();
    }

    const bool config_existed =
        std::filesystem::exists(
            paths->config);

    if (!writeWalletConfig(
            paths->config,
            sequencer_addr)) {
        out["ok"] = false;
        out["code"] = "config_write_failed";
        return out.dump();
    }

    if (!writeRecoveryBackupPendingMarker(
            paths->storage)) {
        if (!config_existed) {
            std::error_code ignored;
            std::filesystem::remove(
                paths->config,
                ignored);
        }

        out["ok"] = false;
        out["code"] =
            "recovery_marker_write_failed";
        return out.dump();
    }

    logos::CallError create_error;

    const std::string mnemonic =
        modules().lez_core.create_new(
            paths->config,
            paths->storage,
            paths->statistics,
            password,
            &create_error,
            120000);

    if (!create_error.ok() ||
        mnemonic.empty()) {
        {
            std::error_code ignored;
            std::filesystem::remove(
                recoveryBackupPendingPath(
                    paths->storage),
                ignored);
        }

        if (!config_existed) {
            std::error_code ignored;
            std::filesystem::remove(
                paths->config,
                ignored);
        }

        out["ok"] = false;
        out["code"] = "wallet_create_failed";
        return out.dump();
    }

    wallet_open_ = true;
    current_config_path_ = paths->config;
    current_storage_path_ = paths->storage;

    logos::CallError save_error;

    const int64_t save_result =
        modules().lez_core.save(
            &save_error);

    if (!save_error.ok() ||
        save_result != kWalletFfiSuccess ||
        !std::filesystem::exists(
            paths->storage)) {
        out["ok"] = false;
        out["code"] = "wallet_save_failed";
        out["walletOpen"] = true;

        // Creation already happened. Never discard the recovery material.
        out["mnemonic"] = mnemonic;

        return out.dump();
    }

    out["ok"] = true;
    out["state"] = "open";
    out["mnemonic"] = mnemonic;
    out["configPath"] = paths->config;
    out["storagePath"] = paths->storage;

    return out.dump();
}

std::string FieldWalletModule::wallet_confirm_recovery_backup()
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    std::string storage_path;

    // After create/create_named the live wallet is authoritative.
    // This is essential for named wallets under wallets/<name>/.
    if (wallet_open_ &&
        !current_storage_path_.empty()) {
        storage_path =
            current_storage_path_;
    } else {
        // Preserve the cold-start/default-wallet recovery path.
        logos::CallError path_error;

        const auto paths =
            defaultWalletPaths(
                modules(),
                path_error);

        if (!path_error.ok()) {
            out["ok"] = false;
            out["code"] = "lez_error";
            return out.dump();
        }

        if (!paths.has_value()) {
            out["ok"] = false;
            out["code"] =
                "wallet_dir_unavailable";
            return out.dump();
        }

        storage_path =
            paths->storage;
    }

    if (!std::filesystem::exists(
            storage_path)) {
        out["ok"] = false;
        out["code"] = "wallet_not_found";
        out["storagePath"] =
            storage_path;
        return out.dump();
    }

    std::error_code ec;

    const bool removed =
        std::filesystem::remove(
            recoveryBackupPendingPath(
                storage_path),
            ec);

    if (ec) {
        out["ok"] = false;
        out["code"] =
            "recovery_marker_remove_failed";
        out["storagePath"] =
            storage_path;
        return out.dump();
    }

    out["ok"] = true;
    out["cleared"] = removed;
    out["storagePath"] =
        storage_path;

    return out.dump();
}

std::string FieldWalletModule::wallet_open(
    const std::string& config_path,
    const std::string& storage_path)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (wallet_open_) {
        if (config_path == current_config_path_ &&
            storage_path == current_storage_path_) {
            out["ok"] = true;
            out["state"] = "open";
            out["configPath"] = current_config_path_;
            out["storagePath"] = current_storage_path_;
            return out.dump();
        }

        out["ok"] = false;
        out["code"] = "wallet_open_requires_restart";
        out["restartRequired"] = true;
        out["configPath"] = current_config_path_;
        out["storagePath"] = current_storage_path_;
        return out.dump();
    }

    if (config_path.empty() ||
        storage_path.empty()) {
        out["ok"] = false;
        out["code"] = "wallet_paths_required";
        return out.dump();
    }

    if (!std::filesystem::exists(
            config_path)) {
        out["ok"] = false;
        out["code"] = "config_not_found";
        return out.dump();
    }

    if (!std::filesystem::exists(
            storage_path)) {
        out["ok"] = false;
        out["code"] = "storage_not_found";
        return out.dump();
    }

    if (recoveryBackupPending(
            storage_path)) {
        out["ok"] = false;
        out["code"] =
            "recovery_backup_pending";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    logos::CallError open_error;

    const int64_t result =
        modules().lez_core.open(
            config_path,
            storage_path,
            statisticsPathFor(
                storage_path),
            &open_error);

    if (!open_error.ok() ||
        result != kWalletFfiSuccess) {
        out["ok"] = false;
        out["code"] = "wallet_open_failed";
        out["ffiCode"] = result;
        return out.dump();
    }

    wallet_open_ = true;
    current_config_path_ = config_path;
    current_storage_path_ = storage_path;

    out["ok"] = true;
    out["state"] = "open";
    out["configPath"] = config_path;
    out["storagePath"] = storage_path;

    return out.dump();
}

std::string FieldWalletModule::wallet_switch(
    const std::string& config_path,
    const std::string& storage_path)
{
    nlohmann::json out;

    const logos::LogosCaller caller =
        logos::currentCaller();

    if (!field::isTrustedApprovalCaller(caller)) {
        out["ok"] = false;
        out["code"] = "approval_not_authorized";
        return out.dump();
    }

    if (config_path.empty() ||
        storage_path.empty()) {
        out["ok"] = false;
        out["code"] = "wallet_paths_required";
        return out.dump();
    }

    if (!std::filesystem::exists(config_path)) {
        out["ok"] = false;
        out["code"] = "config_not_found";
        return out.dump();
    }

    if (!std::filesystem::exists(storage_path)) {
        out["ok"] = false;
        out["code"] = "storage_not_found";
        return out.dump();
    }

    logos::CallError ready_error;

    if (!lezDependencyReady(
            modules(),
            ready_error)) {
        out["ok"] = false;
        out["code"] = "dependency_not_ready";
        return out.dump();
    }

    if (wallet_open_ &&
        config_path == current_config_path_ &&
        storage_path == current_storage_path_) {
        out["ok"] = true;
        out["state"] = "open";
        out["configPath"] = current_config_path_;
        out["storagePath"] = current_storage_path_;
        return out.dump();
    }

    // LEZ 0.3 does not currently expose a close/reset operation for its
    // single live wallet handle. Never pretend the handle was closed by
    // clearing only Field's bookkeeping.
    if (wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_switch_requires_restart";
        out["restartRequired"] = true;
        out["configPath"] = current_config_path_;
        out["storagePath"] = current_storage_path_;
        return out.dump();
    }

    const std::string previous_config =
        current_config_path_;
    const std::string previous_storage =
        current_storage_path_;

    if (wallet_open_) {
        logos::CallError save_error;

        const int64_t save_result =
            modules().lez_core.save(
                &save_error);

        if (!save_error.ok() ||
            save_result != kWalletFfiSuccess) {
            out["ok"] = false;
            out["code"] = "wallet_save_failed";
            return out.dump();
        }

        wallet_open_ = false;
        current_config_path_.clear();
        current_storage_path_.clear();
    }

    logos::CallError open_error;

    const int64_t open_result =
        modules().lez_core.open(
            config_path,
            storage_path,
            statisticsPathFor(storage_path),
            &open_error);

    if (!open_error.ok() ||
        open_result != kWalletFfiSuccess) {
        bool recovered = false;

        if (!previous_config.empty() &&
            !previous_storage.empty()) {
            logos::CallError recovery_error;

            const int64_t recovery_result =
                modules().lez_core.open(
                    previous_config,
                    previous_storage,
                    statisticsPathFor(previous_storage),
                    &recovery_error);

            if (recovery_error.ok() &&
                recovery_result == kWalletFfiSuccess) {
                wallet_open_ = true;
                current_config_path_ = previous_config;
                current_storage_path_ = previous_storage;
                recovered = true;
            }
        }

        out["ok"] = false;
        out["code"] = "wallet_switch_open_failed";
        out["recoveredPreviousWallet"] = recovered;
        return out.dump();
    }

    wallet_open_ = true;
    current_config_path_ = config_path;
    current_storage_path_ = storage_path;

    out["ok"] = true;
    out["state"] = "open";
    out["configPath"] = config_path;
    out["storagePath"] = storage_path;

    return out.dump();
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

    if (!wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_not_open";
        return out.dump();
    }

    logos::CallError error;

    const auto owned_accounts =
        loadOwnedAccounts(
            modules(),
            error,
            keycard_account_id_);

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

    if (!wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_not_open";
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
            error,
            keycard_account_id_);

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

    if (!wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_not_open";
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

    if (!wallet_open_) {
        out["ok"] = false;
        out["code"] = "wallet_not_open";
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
            inventory_error,
            keycard_account_id_);

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
            inventory_error,
            keycard_account_id_);

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
    return approval_execute_transaction_impl(
        request_id,
        nullptr);
}

std::string FieldWalletModule::approval_execute_transaction_keycard(
    uint64_t request_id,
    const std::string& pin)
{
    // The generated RPC/QML layers may make additional string copies, so this
    // is best-effort lifetime reduction rather than an absolute zeroization
    // guarantee. The Rust FFI wraps its own PIN copy in Zeroizing<String>.
    std::string pin_copy = pin;

    const std::string result =
        approval_execute_transaction_impl(
            request_id,
            &pin_copy);

    std::fill(
        pin_copy.begin(),
        pin_copy.end(),
        '\0');
    pin_copy.clear();

    return result;
}

std::string FieldWalletModule::approval_execute_transaction_impl(
    uint64_t request_id,
    const std::string* keycard_pin)
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

    const bool connected_keycard_request =
        !keycard_account_id_.empty() &&
        request->account_id == keycard_account_id_;

    if (keycard_pin == nullptr) {
        // A disconnected Keycard must not become eligible for
        // software signing merely because its active session was
        // invalidated. Require membership in LEZ Core's native
        // account inventory, without Field Wallet's Keycard append.
        logos::CallError software_inventory_error;

        const auto software_accounts_raw =
            modules().lez_core.list_accounts(
                &software_inventory_error);

        if (!software_inventory_error.ok()) {
            out["ok"] = false;
            out["code"] = "software_account_check_failed";
            return out.dump();
        }

        const auto software_accounts =
            field::parseOwnedAccounts(
                software_accounts_raw);

        if (!software_accounts.has_value() ||
            !field::findOwnedAccount(
                *software_accounts,
                request->account_id).has_value()) {
            out["ok"] = false;
            out["code"] = "software_account_not_owned";
            return out.dump();
        }

        if (connected_keycard_request) {
            out["ok"] = false;
            out["code"] = "keycard_pin_required";
            return out.dump();
        }
    } else {
        if (keycard_pin->empty()) {
            out["ok"] = false;
            out["code"] = "keycard_pin_required";
            return out.dump();
        }

        if (keycard_account_id_.empty() ||
            keycard_key_path_.empty()) {
            out["ok"] = false;
            out["code"] = "keycard_not_connected";
            return out.dump();
        }

        if (request->kind !=
                field::TransactionRequestKind::
                    PublicNativeTransfer ||
            request->account_kind !=
                field::AccountKind::Public) {
            out["ok"] = false;
            out["code"] =
                "keycard_transaction_not_supported";
            return out.dump();
        }

        if (request->account_id !=
            keycard_account_id_) {
            out["ok"] = false;
            out["code"] = "keycard_account_mismatch";
            return out.dump();
        }

        FfiBytes32 derived_account{};

        const WalletFfiError verify_error =
            wallet_ffi_keycard_get_public_account(
                keycard_pin->c_str(),
                keycard_key_path_.c_str(),
                &derived_account);

        if (static_cast<int64_t>(verify_error) !=
            kWalletFfiSuccess) {
            out["ok"] = false;
            out["code"] =
                "keycard_verification_failed";
            return out.dump();
        }

        if (ffiBytes32ToHex(derived_account) !=
            request->account_id) {
            out["ok"] = false;
            out["code"] = "keycard_account_mismatch";
            return out.dump();
        }
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
        if (keycard_pin != nullptr) {
            lez_result =
                modules().lez_core.transfer_public_keycard(
                    request->account_id,
                    request->destination_account_id,
                    request->amount_le16_hex,
                    *keycard_pin,
                    keycard_key_path_,
                    &error);
        } else {
            lez_result =
                modules().lez_core.transfer_public(
                    request->account_id,
                    request->destination_account_id,
                    request->amount_le16_hex,
                    &error);
        }
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
            inventory_error,
            keycard_account_id_);

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

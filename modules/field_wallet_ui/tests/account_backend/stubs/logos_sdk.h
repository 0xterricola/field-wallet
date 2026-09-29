#pragma once
#include "logos_api.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <functional>
#include <string>

struct Timeout { explicit Timeout(int) {} };
namespace logos {
template<class T> struct AsyncResult {
    T value;
    struct { std::string message; } error;
    bool ok() const { return error.message.empty(); }
};
}

namespace account_test {
inline QString encode(const QJsonObject& value) {
    return QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact));
}
inline QJsonObject account(const QString& id, const QString& kind) {
    return {{"accountId", id}, {"accountKind", kind}};
}
struct State {
    QString storage = "/test/vault-a/storage.json";
    QMap<QString, QJsonArray> inventories;
    QString inventoryReply;
    QString balanceReply;
    QString creationReply;
    bool creationTransportFailure = false;
    int creates = 0;
    int switches = 0;
    QString balanceAccount;
    std::function<void()> pending;
};
inline State state;
inline void finish() {
    auto callback = std::move(state.pending);
    state.pending = {};
    if (callback) callback();
}
}

struct TestFieldWallet {
    QString wallet_status() {
        return account_test::encode({{"ok", true}, {"state", "open"},
            {"configPath", account_test::state.storage + ".config"},
            {"storagePath", account_test::state.storage}});
    }
    QString wallet_list_saved() { return QStringLiteral("{\"ok\":true,\"wallets\":[]}"); }
    QString wallet_list_accounts() {
        auto& s = account_test::state;
        if (!s.inventoryReply.isEmpty()) return s.inventoryReply;
        return account_test::encode({{"ok", true}, {"accounts", s.inventories.value(s.storage)}});
    }
    QString wallet_get_balance(const QString& id) {
        auto& s = account_test::state;
        s.balanceAccount = id;
        if (!s.balanceReply.isEmpty()) return s.balanceReply;
        return account_test::encode({{"ok", true}, {"balance", id.endsWith('b') ? "22" : "11"}});
    }
    QString wallet_open(const QString&, const QString& storage) {
        account_test::state.storage = storage;
        return wallet_status();
    }
    template<class Callback> void wallet_switchAsyncResult(const QString&, const QString& storage, Callback cb, Timeout) {
        ++account_test::state.switches;
        account_test::state.storage = storage;
        cb(logos::AsyncResult<QString>{wallet_status(), {}});
    }
    template<class Callback> void wallet_createAsyncResult(const QString&, const QString&, Callback cb, Timeout) {
        cb(logos::AsyncResult<QString>{QStringLiteral("{\"ok\":false,\"code\":\"not_used_in_test\"}"), {}});
    }
    template<class Callback> void wallet_create_namedAsyncResult(const QString&, const QString&, const QString&, Callback cb, Timeout t) {
        wallet_createAsyncResult({}, {}, cb, t);
    }
    template<class Callback> void create(const QString& kind, Callback cb) {
        auto& s = account_test::state;
        ++s.creates;
        const QString storage = s.storage;
        s.pending = [kind, storage, cb] {
            auto& s = account_test::state;
            if (s.creationTransportFailure) {
                cb(logos::AsyncResult<QString>{{}, {"timeout"}});
                return;
            }
            if (!s.creationReply.isEmpty()) {
                cb(logos::AsyncResult<QString>{s.creationReply, {}});
                return;
            }
            const QString id(64, kind == "private" ? 'b' : 'c');
            s.inventories[storage].append(account_test::account(id, kind));
            cb(logos::AsyncResult<QString>{account_test::encode(
                {{"ok", true}, {"accountId", id}, {"accountKind", kind}}), {}});
        };
    }
    template<class Callback> void wallet_create_private_accountAsyncResult(Callback cb, Timeout) { create("private", cb); }
    template<class Callback> void wallet_create_public_accountAsyncResult(Callback cb, Timeout) { create("public", cb); }
};
struct LogosModules {
    explicit LogosModules(LogosAPI*) {}
    TestFieldWallet field_wallet;
};

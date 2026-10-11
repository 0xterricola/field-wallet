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

    bool keycardAvailable = false;
    QString keycardAccountId;

    QString approvalListReply;
    QString approvalRejectReply;
    QString approvalExecuteReply;
    QString approvalKeycardExecuteReply;

    int approvalRejects = 0;
    int approvalExecutes = 0;
    int approvalKeycardExecutes = 0;

    qulonglong lastApprovalRequestId = 0;
    qulonglong lastKeycardRequestId = 0;

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

    template<class Callback>
    void wallet_restore_namedAsyncResult(
        const QString& walletName,
        const QString&,
        const QString&,
        const QString&,
        Callback cb,
        Timeout)
    {
        auto& s = account_test::state;
        s.storage =
            QStringLiteral("/test/") +
            walletName +
            QStringLiteral("/storage.json");

        cb(logos::AsyncResult<QString>{
            account_test::encode({
                {"ok", true},
                {"configPath", s.storage + ".config"},
                {"storagePath", s.storage}
            }),
            {}
        });
    }

    QString wallet_confirm_recovery_backup()
    {
        return account_test::encode({
            {"ok", true}
        });
    }

    template<class Callback>
    void keycard_availableAsyncResult(
        Callback cb,
        Timeout)
    {
        cb(logos::AsyncResult<bool>{
            account_test::state.keycardAvailable,
            {}
        });
    }

    template<class Callback>
    void keycard_get_public_accountAsyncResult(
        const QString&,
        const QString&,
        Callback cb,
        Timeout)
    {
        cb(logos::AsyncResult<QString>{
            account_test::state.keycardAccountId,
            {}
        });
    }

    template<class Callback>
    void approval_list_transaction_requestsAsyncResult(
        Callback cb,
        Timeout)
    {
        auto& s = account_test::state;

        const QString reply =
            s.approvalListReply.isEmpty()
                ? account_test::encode({
                    {"ok", true},
                    {"requests", QJsonArray{}}
                })
                : s.approvalListReply;

        cb(logos::AsyncResult<QString>{reply, {}});
    }

    template<class Callback>
    void approval_reject_transactionAsyncResult(
        qulonglong requestId,
        Callback cb,
        Timeout)
    {
        auto& s = account_test::state;
        ++s.approvalRejects;
        s.lastApprovalRequestId = requestId;

        const QString reply =
            s.approvalRejectReply.isEmpty()
                ? account_test::encode({
                    {"ok", true},
                    {"requestId", static_cast<qint64>(requestId)},
                    {"status", "rejected"}
                })
                : s.approvalRejectReply;

        cb(logos::AsyncResult<QString>{reply, {}});
    }

    template<class Callback>
    void approval_execute_transactionAsyncResult(
        qulonglong requestId,
        Callback cb,
        Timeout)
    {
        auto& s = account_test::state;
        ++s.approvalExecutes;
        s.lastApprovalRequestId = requestId;

        const QString reply =
            s.approvalExecuteReply.isEmpty()
                ? account_test::encode({
                    {"ok", true},
                    {"requestId", static_cast<qint64>(requestId)},
                    {"status", "executed"}
                })
                : s.approvalExecuteReply;

        cb(logos::AsyncResult<QString>{reply, {}});
    }

    template<class Callback>
    void approval_execute_transaction_keycardAsyncResult(
        qulonglong requestId,
        const QString&,
        Callback cb,
        Timeout)
    {
        auto& s = account_test::state;
        ++s.approvalKeycardExecutes;
        s.lastKeycardRequestId = requestId;

        const QString reply =
            s.approvalKeycardExecuteReply.isEmpty()
                ? account_test::encode({
                    {"ok", true},
                    {"requestId", static_cast<qint64>(requestId)},
                    {"status", "executed"}
                })
                : s.approvalKeycardExecuteReply;

        cb(logos::AsyncResult<QString>{reply, {}});
    }
};
struct LogosModules {
    explicit LogosModules(LogosAPI*) {}
    TestFieldWallet field_wallet;
};

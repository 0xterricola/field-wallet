#include "FieldWalletBackend.h"
#include "logos_sdk.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QTest>

class AccountTests : public QObject {
    Q_OBJECT
    QTemporaryDir settingsDir;
    const QString publicId = QString(64, 'a');
    const QString privateId = QString(64, 'b');
    const QString otherId = QString(64, 'd');
private slots:
    void initTestCase() {
        QVERIFY(settingsDir.isValid());
        QCoreApplication::setOrganizationName("FieldAccountTests");
        QCoreApplication::setApplicationName("IsolatedSettings");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    }
    void init() {
        QSettings().clear();
        account_test::state = {};
        auto& s = account_test::state;
        s.inventories[s.storage] = {account_test::account(publicId, "public")};
        s.inventories["/test/vault-b/storage.json"] = {account_test::account(otherId, "public")};
    }
    void createsPrivateAlongsideExistingPublicAndSelectsIt() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        QCOMPARE(backend.accountId(), publicId);
        QVERIFY(backend.accountsLoaded());
        backend.createPrivateAccount();
        QVERIFY(backend.accountBusy());
        backend.createPrivateAccount();
        QCOMPARE(account_test::state.creates, 1);
        backend.switchWallet("ignored", "/test/vault-b/storage.json");
        QCOMPARE(account_test::state.switches, 0);
        account_test::finish();
        QVERIFY(!backend.accountBusy());
        QCOMPARE(backend.accountId(), privateId);
        QCOMPARE(backend.accountKind(), QString("private"));
        QCOMPARE(backend.balanceRaw(), QString("22"));
        QCOMPARE(account_test::state.balanceAccount, privateId);
        QCOMPARE(QJsonDocument::fromJson(backend.accountsJson().toUtf8()).array().size(), 2);
        backend.selectAccount(publicId);
        QCOMPARE(backend.accountId(), publicId);
        QCOMPARE(backend.balanceRaw(), QString("11"));
        backend.selectAccount(privateId);
        backend.refreshWallet();
        QCOMPARE(backend.accountId(), privateId);
    }
    void selectionSurvivesBackendRecreationAndWalletSwitching() {
        account_test::state.inventories[account_test::state.storage].append(account_test::account(privateId, "private"));
        {
            FieldWalletBackend backend;
            backend.refreshWallet();
            backend.selectAccount(privateId);
            backend.switchWallet("ignored", "/test/vault-b/storage.json");
            QCOMPARE(backend.accountId(), otherId);
            backend.switchWallet("ignored", "/test/vault-a/storage.json");
            QCOMPARE(backend.accountId(), privateId);
        }
        FieldWalletBackend reopened;
        reopened.refreshWallet();
        QCOMPARE(reopened.accountId(), privateId);
        QCOMPARE(reopened.accountKind(), QString("private"));
    }
    void accountNamesPersistAndStayScopedToTheirWallet() {
        account_test::state.inventories[account_test::state.storage].append(account_test::account(privateId, "private"));
        // Even the same account ID in another vault must have independent metadata.
        account_test::state.inventories["/test/vault-b/storage.json"] = {account_test::account(privateId, "private")};
        {
            FieldWalletBackend backend;
            backend.refreshWallet();
            backend.renameAccount(privateId, "Private savings");
            auto accounts = QJsonDocument::fromJson(backend.accountsJson().toUtf8()).array();
            QCOMPARE(accounts.at(1).toObject().value("name").toString(), QString("Private savings"));
            backend.switchWallet("ignored", "/test/vault-b/storage.json");
            accounts = QJsonDocument::fromJson(backend.accountsJson().toUtf8()).array();
            QCOMPARE(accounts.first().toObject().value("name").toString(), QString("Private account 1"));
            backend.renameAccount(privateId, "Separate wallet");
            backend.switchWallet("ignored", "/test/vault-a/storage.json");
        }
        FieldWalletBackend reopened;
        reopened.refreshWallet();
        const auto accounts = QJsonDocument::fromJson(reopened.accountsJson().toUtf8()).array();
        QCOMPARE(accounts.at(1).toObject().value("name").toString(), QString("Private savings"));
    }
    void invalidRenameDoesNotChangeNames() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        const QString before = backend.accountsJson();
        backend.renameAccount(publicId, "  ");
        QCOMPARE(backend.accountsJson(), before);
        QVERIFY(!backend.walletError().isEmpty());
        backend.renameAccount(publicId, QString(65, 'x'));
        QCOMPARE(backend.accountsJson(), before);
        backend.renameAccount(otherId, "Not owned");
        QCOMPARE(backend.accountsJson(), before);
        QVERIFY(!backend.walletError().isEmpty());
    }
    void rejectsAccountOutsideCurrentWallet() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        backend.selectAccount(otherId);
        QCOMPARE(backend.accountId(), publicId);
        QCOMPARE(account_test::state.balanceAccount, publicId);
        QVERIFY(!backend.walletError().isEmpty());
    }
    void inventoryFailuresAreNotEmptyWallets_data() {
        QTest::addColumn<QString>("reply");
        QTest::newRow("transport-shaped-error") << QString("{\"ok\":false,\"code\":\"lez_error\"}");
        QTest::newRow("invalid-json") << QString("bad");
        QTest::newRow("missing-inventory") << QString("{\"ok\":true}");
        QTest::newRow("invalid-account") << QString("{\"ok\":true,\"accounts\":[{\"accountId\":\"a\",\"accountKind\":\"unknown\"}]}");
    }
    void inventoryFailuresAreNotEmptyWallets() {
        QFETCH(QString, reply);
        FieldWalletBackend backend;
        backend.refreshWallet();
        account_test::state.inventoryReply = reply;
        backend.refreshWallet();
        QVERIFY(!backend.accountsLoaded());
        QVERIFY(backend.accountId().isEmpty());
        QVERIFY(backend.balanceRaw().isEmpty());
        QVERIFY(!backend.walletError().isEmpty());
        backend.createPrivateAccount();
        QCOMPARE(account_test::state.creates, 0);
        account_test::state.inventoryReply.clear();
        backend.refreshWallet();
        QCOMPARE(backend.accountId(), publicId);
    }
    void createsFirstAccountInActuallyEmptyWallet() {
        account_test::state.inventories[account_test::state.storage] = {};
        FieldWalletBackend backend;
        backend.refreshWallet();
        QVERIFY(backend.accountsLoaded());
        QVERIFY(backend.accountId().isEmpty());
        backend.createPrivateAccount();
        account_test::finish();
        QCOMPARE(backend.accountId(), privateId);
    }
    void failedCreationDoesNotSelectOrDuplicateAccount() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        account_test::state.creationReply = "{\"ok\":false,\"code\":\"lez_error\"}";
        backend.createPrivateAccount();
        account_test::finish();
        QVERIFY(!backend.accountBusy());
        QCOMPARE(backend.accountId(), publicId);
        QVERIFY(backend.walletError().contains("lez_error"));
        QCOMPARE(QJsonDocument::fromJson(backend.accountsJson().toUtf8()).array().size(), 1);
    }
    void transportFailureReportsUnknownOutcome() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        account_test::state.creationTransportFailure = true;
        backend.createPrivateAccount();
        account_test::finish();
        QVERIFY(!backend.accountBusy());
        QCOMPARE(backend.accountId(), publicId);
        QVERIFY(backend.walletError().contains("unknown"));
    }
    void balanceFailureKeepsPrivateAccountAndClearsPreviousBalance() {
        FieldWalletBackend backend;
        backend.refreshWallet();
        account_test::state.balanceReply = "{\"ok\":false,\"code\":\"lez_error\"}";
        backend.createPrivateAccount();
        account_test::finish();
        QCOMPARE(backend.accountId(), privateId);
        QVERIFY(backend.balanceRaw().isEmpty());
        QVERIFY(backend.walletError().contains("balance:"));
    }
    void missingRememberedAccountFallsBackToOwnedAccount() {
        account_test::state.inventories[account_test::state.storage].append(account_test::account(privateId, "private"));
        FieldWalletBackend backend;
        backend.refreshWallet();
        backend.selectAccount(privateId);
        account_test::state.inventories[account_test::state.storage] = {account_test::account(publicId, "public")};
        backend.refreshWallet();
        QCOMPARE(backend.accountId(), publicId);
    }
    void approvalListLoadsPendingTransaction() {
        auto& state = account_test::state;

        state.approvalListReply =
            account_test::encode({
                {"ok", true},
                {"requests", QJsonArray{
                    QJsonObject{
                        {"requestId", static_cast<qint64>(41)},
                        {"type", "public_native_transfer"},
                        {"callerKey", "module|4:test|0:"},
                        {"moduleName", "test"},
                        {"moduleInstance", ""},
                        {"accountId", publicId},
                        {"accountKind", "public"},
                        {"destinationAccountId", otherId},
                        {"amountLe16Hex",
                         "01000000000000000000000000000000"}
                    }
                }}
            });

        FieldWalletBackend backend;

        backend.refreshApprovals();

        QVERIFY(!backend.approvalBusy());
        QVERIFY(backend.approvalError().isEmpty());

        const auto requests =
            QJsonDocument::fromJson(
                backend.approvalTransactionsJson().toUtf8())
                .array();

        QCOMPARE(requests.size(), 1);

        const auto request =
            requests.first().toObject();

        QCOMPARE(
            request.value("requestId").toInteger(),
            qint64(41));

        QCOMPARE(
            request.value("accountId").toString(),
            publicId);

        QCOMPARE(
            request.value("destinationAccountId").toString(),
            otherId);

        QCOMPARE(
            request.value("type").toString(),
            QString("public_native_transfer"));
    }

    void rejectDispatchesOnlyRejectPath() {
        FieldWalletBackend backend;

        backend.rejectTransaction(42);

        QCOMPARE(
            account_test::state.approvalRejects,
            1);

        QCOMPARE(
            account_test::state.approvalExecutes,
            0);

        QCOMPARE(
            account_test::state.approvalKeycardExecutes,
            0);

        QCOMPARE(
            account_test::state.lastApprovalRequestId,
            qulonglong(42));

        QVERIFY(!backend.approvalBusy());

        QVERIFY(
            backend.approvalResult()
                .contains("rejected"));
    }

    void softwareExecutionUsesRegularSignerPath() {
        FieldWalletBackend backend;

        backend.executeTransaction(43);

        QCOMPARE(
            account_test::state.approvalExecutes,
            1);

        QCOMPARE(
            account_test::state.approvalKeycardExecutes,
            0);

        QCOMPARE(
            account_test::state.lastApprovalRequestId,
            qulonglong(43));

        QVERIFY(!backend.approvalBusy());
        QVERIFY(backend.approvalError().isEmpty());

        QVERIFY(
            backend.approvalResult()
                .contains("executed"));
    }

    void keycardExecutionRequiresPinAndUsesHardwarePath() {
        FieldWalletBackend backend;

        backend.executeKeycardTransaction(
            44,
            QString());

        QCOMPARE(
            account_test::state.approvalKeycardExecutes,
            0);

        QVERIFY(!backend.approvalError().isEmpty());

        backend.executeKeycardTransaction(
            44,
            QStringLiteral("123456"));

        QCOMPARE(
            account_test::state.approvalKeycardExecutes,
            1);

        QCOMPARE(
            account_test::state.approvalExecutes,
            0);

        QCOMPARE(
            account_test::state.lastKeycardRequestId,
            qulonglong(44));

        QVERIFY(!backend.approvalBusy());
        QVERIFY(backend.approvalError().isEmpty());

        QVERIFY(
            backend.approvalResult()
                .contains("executed"));
    }

};
QTEST_GUILESS_MAIN(AccountTests)
#include "test_accounts.moc"

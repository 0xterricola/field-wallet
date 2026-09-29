#include "FieldWalletBackend.h"

#include "logos_api.h"
#include "logos_sdk.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QSettings>

namespace {

bool parseObject(
    const QString& raw,
    QJsonObject& object)
{
    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            raw.toUtf8(),
            &error);

    if (error.error != QJsonParseError::NoError ||
        !document.isObject()) {
        return false;
    }

    object = document.object();
    return true;
}

QString walletMetadataKey(
    const QString& storagePath)
{
    const QByteArray digest =
        QCryptographicHash::hash(
            storagePath.toUtf8(),
            QCryptographicHash::Sha256)
            .toHex();

    return QStringLiteral(
        "walletDisplay/%1/")
        .arg(
            QString::fromLatin1(digest));
}

QString accountNameKey(const QString& storagePath, const QString& accountId)
{
    return walletMetadataKey(storagePath) + QStringLiteral("accountNames/") +
        QString::fromLatin1(QCryptographicHash::hash(
            accountId.toUtf8(), QCryptographicHash::Sha256).toHex());
}

void rememberWalletAccount(
    const QString& storagePath,
    const QString& accountId,
    const QString& accountKind)
{
    if (storagePath.isEmpty() ||
        accountId.isEmpty()) {
        return;
    }

    QSettings settings;

    const QString prefix =
        walletMetadataKey(storagePath);

    settings.setValue(
        prefix + QStringLiteral("accountId"),
        accountId);

    settings.setValue(
        prefix + QStringLiteral("accountKind"),
        accountKind);
}

QString resultCode(
    const QJsonObject& object,
    const QString& fallback)
{
    const QString code =
        object.value(
            QStringLiteral("code"))
            .toString();

    return code.isEmpty()
        ? fallback
        : code;
}

}

FieldWalletBackend::FieldWalletBackend(
    LogosAPI* logosAPI,
    QObject* parent)
    : FieldWalletBackendSimpleSource(parent),
      m_logosAPI(
          logosAPI
              ? logosAPI
              : new LogosAPI(
                    "field_wallet_ui",
                    this)),
      m_logos(new LogosModules(m_logosAPI))
{
    setReady(true);
    setBuildLabel(
        QStringLiteral("live-core"));

    setWalletState(
        QStringLiteral("loading"));

    setConfigPath(QString());
    setStoragePath(QString());
    setRecoveryPhrase(QString());
    setSavedWalletsJson(QStringLiteral("[]"));

    clearAccountData();
    setAccountBusy(false);
    setWalletError(QString());
    setWalletBusy(false);
    setWalletSwitchBusy(false);
    setWalletFolderSelection(QString());
    setWalletFolderHasConfig(false);
    setWalletFolderHasStorage(false);
}

FieldWalletBackend::~FieldWalletBackend()
{
    delete m_logos;
}

QString FieldWalletBackend::ping()
{
    return QStringLiteral(
        "field-wallet-ui");
}

void FieldWalletBackend::clearAccountData()
{
    setAccountsLoaded(false);
    setAccountsJson(QStringLiteral("[]"));
    setAccountId(QString());
    setAccountKind(QString());
    setBalanceRaw(QString());
}

void FieldWalletBackend::chooseWalletFolder()
{
    // Clear the previous value so choosing the same folder twice
    // still produces a property update.
    setWalletFolderSelection(QString());
    setWalletFolderHasConfig(false);
    setWalletFolderHasStorage(false);

#ifdef Q_OS_MACOS
    auto* process = new QProcess(this);

    connect(
        process,
        qOverload<int, QProcess::ExitStatus>(
            &QProcess::finished),
        this,
        [this, process](
            int exitCode,
            QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::NormalExit &&
                exitCode == 0) {
                QString path =
                    QString::fromUtf8(
                        process->readAllStandardOutput())
                        .trimmed();

                while (path.length() > 1 &&
                       path.endsWith('/')) {
                    path.chop(1);
                }

                if (!path.isEmpty()) {
                    const QDir folder(path);

                    const QFileInfo configFile(
                        folder.filePath(
                            QStringLiteral("config.json")));

                    const QFileInfo storageFile(
                        folder.filePath(
                            QStringLiteral("storage.json")));

                    setWalletFolderSelection(path);

                    setWalletFolderHasConfig(
                        configFile.exists() &&
                        configFile.isFile());

                    setWalletFolderHasStorage(
                        storageFile.exists() &&
                        storageFile.isFile());
                }
            }

            process->deleteLater();
        });

    process->start(
        QStringLiteral("/usr/bin/osascript"),
        {
            QStringLiteral("-e"),
            QStringLiteral(
                "POSIX path of "
                "(choose folder with prompt "
                "\"Choose Field wallet folder\")")
        });
#else
    setWalletError(
        QStringLiteral(
            "folder picker: unsupported platform"));
#endif
}

void FieldWalletBackend::refreshWallet()
{
    if (accountBusy())
        return;

    setWalletError(QString());

    loadSavedWallets();

    const QString statusRaw =
        m_logos
            ->field_wallet
            .wallet_status();

    QJsonObject status;

    if (!parseObject(
            statusRaw,
            status)) {
        setWalletState(
            QStringLiteral("error"));

        setWalletError(
            QStringLiteral(
                "wallet status: invalid response"));
        return;
    }

    if (!status
             .value(QStringLiteral("ok"))
             .toBool()) {
        setWalletState(
            QStringLiteral("error"));

        setWalletError(
            QStringLiteral("wallet status: ") +
            resultCode(
                status,
                QStringLiteral(
                    "unknown error")));

        clearAccountData();
        return;
    }

    const QString state =
        status.value(
            QStringLiteral("state"))
            .toString();

    setWalletState(
        state.isEmpty()
            ? QStringLiteral("error")
            : state);

    setConfigPath(
        status.value(
            QStringLiteral("configPath"))
            .toString());

    setStoragePath(
        status.value(
            QStringLiteral("storagePath"))
            .toString());

    if (state != QStringLiteral("open")) {
        clearAccountData();
        return;
    }

    loadAccountsAndBalance();
}

void FieldWalletBackend::loadSavedWallets()
{
    const QString raw =
        m_logos
            ->field_wallet
            .wallet_list_saved();

    QJsonObject result;

    if (!parseObject(raw, result)) {
        setSavedWalletsJson(
            QStringLiteral("[]"));
        return;
    }

    if (!result
             .value(QStringLiteral("ok"))
             .toBool()) {
        setSavedWalletsJson(
            QStringLiteral("[]"));
        return;
    }

    const QJsonArray sourceWallets =
        result.value(
            QStringLiteral("wallets"))
            .toArray();

    QJsonArray wallets;
    QSettings settings;

    for (const QJsonValue& value :
         sourceWallets) {
        QJsonObject wallet =
            value.toObject();

        const QString storagePath =
            wallet.value(
                QStringLiteral("storagePath"))
                .toString();

        if (!storagePath.isEmpty()) {
            const QString prefix =
                walletMetadataKey(
                    storagePath);

            const QString accountId =
                settings.value(
                    prefix +
                    QStringLiteral("accountId"))
                    .toString();

            const QString accountKind =
                settings.value(
                    prefix +
                    QStringLiteral("accountKind"))
                    .toString();

            const QString walletId =
                wallet.value(
                    QStringLiteral("id"))
                    .toString();

            QString displayName =
                settings.value(
                    prefix +
                    QStringLiteral("displayName"))
                    .toString()
                    .trimmed();

            // Existing wallets automatically start with their
            // immutable wallet ID as their Field display name.
            if (displayName.isEmpty()) {
                displayName = walletId;

                if (displayName.isEmpty()) {
                    displayName =
                        wallet.value(
                            QStringLiteral("name"))
                            .toString();
                }

                if (!displayName.isEmpty()) {
                    settings.setValue(
                        prefix +
                        QStringLiteral("displayName"),
                        displayName);
                }
            }

            if (!walletId.isEmpty()) {
                wallet[
                    QStringLiteral(
                        "walletId")] =
                    walletId;
            }

            if (!displayName.isEmpty()) {
                wallet[
                    QStringLiteral(
                        "name")] =
                    displayName;
            }

            if (!accountId.isEmpty())
                wallet[
                    QStringLiteral(
                        "accountId")] =
                    accountId;

            if (!accountKind.isEmpty())
                wallet[
                    QStringLiteral(
                        "accountKind")] =
                    accountKind;
        }

        wallets.append(wallet);
    }

    setSavedWalletsJson(
        QString::fromUtf8(
            QJsonDocument(wallets)
                .toJson(
                    QJsonDocument::Compact)));
}

void FieldWalletBackend::loadAccountsAndBalance(
    const QString& preferredAccountId)
{
    clearAccountData();

    const QString accountsRaw =
        m_logos
            ->field_wallet
            .wallet_list_accounts();

    QJsonObject accountsResult;

    if (!parseObject(
            accountsRaw,
            accountsResult)) {
        setWalletError(
            QStringLiteral(
                "accounts: invalid response"));
        return;
    }

    if (!accountsResult
             .value(QStringLiteral("ok"))
             .toBool()) {
        setWalletError(
            QStringLiteral("accounts: ") +
            resultCode(
                accountsResult,
                QStringLiteral(
                    "unknown error")));
        return;
    }

    if (!accountsResult.value(QStringLiteral("accounts")).isArray()) {
        setWalletError(QStringLiteral("accounts: invalid inventory"));
        return;
    }

    const QJsonArray accounts =
        accountsResult.value(QStringLiteral("accounts")).toArray();

    // Validate the entire inventory before allowing selection or creation.
    for (const QJsonValue& value : accounts) {
        const QJsonObject account = value.toObject();
        const QString id = account.value(QStringLiteral("accountId")).toString();
        const QString kind = account.value(QStringLiteral("accountKind")).toString();
        if (id.isEmpty() || (kind != QStringLiteral("public") &&
                             kind != QStringLiteral("private"))) {
            setWalletError(QStringLiteral("accounts: malformed account"));
            return;
        }
    }

    QSettings settings;
    QJsonArray namedAccounts;
    int publicCount = 0;
    int privateCount = 0;
    for (const QJsonValue& value : accounts) {
        QJsonObject account = value.toObject();
        const QString id = account.value(QStringLiteral("accountId")).toString();
        const bool isPrivate = account.value(QStringLiteral("accountKind")).toString() == QStringLiteral("private");
        const int number = isPrivate ? ++privateCount : ++publicCount;
        const QString key = accountNameKey(storagePath(), id);
        QString name = settings.value(key).toString().trimmed();
        if (name.isEmpty()) {
            name = (isPrivate ? QStringLiteral("Private account %1") : QStringLiteral("Public account %1")).arg(number);
            settings.setValue(key, name);
        }
        account[QStringLiteral("name")] = name;
        namedAccounts.append(account);
    }
    setAccountsJson(QString::fromUtf8(
        QJsonDocument(namedAccounts).toJson(QJsonDocument::Compact)));
    setAccountsLoaded(true);

    if (accounts.isEmpty()) {
        setWalletError(preferredAccountId.isEmpty()
            ? QString()
            : QStringLiteral("Created account is not in the wallet inventory yet. Refresh accounts before trying again."));
        return;
    }

    const QString rememberedId = settings.value(
        walletMetadataKey(storagePath()) + QStringLiteral("accountId")).toString();
    const QString wantedId = preferredAccountId.isEmpty()
        ? rememberedId : preferredAccountId;

    QJsonObject account = accounts.first().toObject();
    bool preferredFound = false;
    for (const QJsonValue& value : accounts) {
        if (value.toObject().value(QStringLiteral("accountId")).toString() == wantedId) {
            account = value.toObject();
            preferredFound = true;
            break;
        }
    }

    // Do not silently display another account after an explicit selection/create.
    if (!preferredAccountId.isEmpty() && !preferredFound) {
        setWalletError(QStringLiteral("Requested account is not in the wallet inventory. Refresh accounts before trying again."));
        return;
    }

    const QString accountId = account.value(QStringLiteral("accountId")).toString();
    const QString accountKind = account.value(QStringLiteral("accountKind")).toString();

    setAccountId(accountId);
    setAccountKind(accountKind);

    rememberWalletAccount(
        storagePath(),
        accountId,
        accountKind);

    loadSavedWallets();

    const QString balanceRaw =
        m_logos
            ->field_wallet
            .wallet_get_balance(
                accountId);

    QJsonObject balanceResult;

    if (!parseObject(
            balanceRaw,
            balanceResult)) {
        setWalletError(
            QStringLiteral(
                "balance: invalid response"));
        return;
    }

    if (!balanceResult
             .value(QStringLiteral("ok"))
             .toBool()) {
        setWalletError(
            QStringLiteral("balance: ") +
            resultCode(
                balanceResult,
                QStringLiteral(
                    "unknown error")));
        return;
    }

    setBalanceRaw(
        balanceResult.value(
            QStringLiteral("balance"))
            .toString());

    setWalletError(QString());
}

void FieldWalletBackend::createWallet(
    QString password,
    QString sequencerAddr)
{
    if (accountBusy())
        return;

    setWalletError(QString());
    setWalletBusy(true);

    m_logos
        ->field_wallet
        .wallet_createAsyncResult(
            password,
            sequencerAddr.trimmed(),
            [this](logos::AsyncResult<QString> outcome) {
                if (!outcome.ok()) {
                    setWalletError(
                        QStringLiteral("create transport: ") +
                        QString::fromStdString(
                            outcome.error.message));
                    setWalletBusy(false);
                    return;
                }

                QJsonObject result;

                if (!parseObject(
                        outcome.value,
                        result)) {
                    setWalletError(
                        QStringLiteral(
                            "create: invalid response"));
                    setWalletBusy(false);
                    return;
                }

                const QString mnemonic =
                    result.value(
                        QStringLiteral("mnemonic"))
                        .toString();

                if (!result
                         .value(QStringLiteral("ok"))
                         .toBool()) {
                    if (!mnemonic.isEmpty())
                        setRecoveryPhrase(mnemonic);

                    setWalletError(
                        QStringLiteral("create: ") +
                        resultCode(
                            result,
                            QStringLiteral(
                                "unknown error")));

                    setWalletBusy(false);
                    return;
                }

                setWalletState(
                    QStringLiteral("open"));

                setConfigPath(
                    result.value(
                        QStringLiteral("configPath"))
                        .toString());

                setStoragePath(
                    result.value(
                        QStringLiteral("storagePath"))
                        .toString());

                setRecoveryPhrase(mnemonic);

                loadSavedWallets();
                clearAccountData();

                setWalletBusy(false);
            },
            Timeout(120000));
}

void FieldWalletBackend::createNamedWallet(
    QString walletName,
    QString password,
    QString sequencerAddr)
{
    if (accountBusy())
        return;

    setWalletError(QString());
    setWalletBusy(true);

    m_logos
        ->field_wallet
        .wallet_create_namedAsyncResult(
            walletName.trimmed(),
            password,
            sequencerAddr.trimmed(),
            [this](logos::AsyncResult<QString> outcome) {
                if (!outcome.ok()) {
                    setWalletError(
                        QStringLiteral("create transport: ") +
                        QString::fromStdString(
                            outcome.error.message));
                    setWalletBusy(false);
                    return;
                }

                QJsonObject result;

                if (!parseObject(
                        outcome.value,
                        result)) {
                    setWalletError(
                        QStringLiteral(
                            "create: invalid response"));
                    setWalletBusy(false);
                    return;
                }

                const QString mnemonic =
                    result.value(
                        QStringLiteral("mnemonic"))
                        .toString();

                if (!result
                         .value(QStringLiteral("ok"))
                         .toBool()) {
                    if (!mnemonic.isEmpty())
                        setRecoveryPhrase(mnemonic);

                    setWalletError(
                        QStringLiteral("create: ") +
                        resultCode(
                            result,
                            QStringLiteral(
                                "unknown error")));

                    setWalletBusy(false);
                    return;
                }

                setWalletState(
                    QStringLiteral("open"));

                setConfigPath(
                    result.value(
                        QStringLiteral("configPath"))
                        .toString());

                setStoragePath(
                    result.value(
                        QStringLiteral("storagePath"))
                        .toString());

                setRecoveryPhrase(mnemonic);

                loadSavedWallets();
                clearAccountData();

                setWalletBusy(false);
            },
            Timeout(120000));
}



void FieldWalletBackend::renameWallet(
    QString storagePath,
    QString displayName)
{
    storagePath = storagePath.trimmed();
    displayName = displayName.trimmed();

    if (storagePath.isEmpty() ||
        displayName.isEmpty() ||
        displayName.size() > 64) {
        return;
    }

    QSettings settings;

    const QString prefix =
        walletMetadataKey(storagePath);

    settings.setValue(
        prefix + QStringLiteral("displayName"),
        displayName);

    settings.sync();

    loadSavedWallets();
}

void FieldWalletBackend::openWallet(
    QString configPath,
    QString storagePath)
{
    if (accountBusy())
        return;

    setWalletError(QString());

    const QString raw =
        m_logos
            ->field_wallet
            .wallet_open(
                configPath.trimmed(),
                storagePath.trimmed());

    QJsonObject result;

    if (!parseObject(raw, result)) {
        setWalletError(
            QStringLiteral(
                "open: invalid response"));
        return;
    }

    if (!result
             .value(QStringLiteral("ok"))
             .toBool()) {
        setWalletError(
            QStringLiteral("open: ") +
            resultCode(
                result,
                QStringLiteral(
                    "unknown error")));
        return;
    }

    setWalletState(
        QStringLiteral("open"));

    setConfigPath(
        result.value(
            QStringLiteral("configPath"))
            .toString());

    setStoragePath(
        result.value(
            QStringLiteral("storagePath"))
            .toString());

    loadSavedWallets();
    loadAccountsAndBalance();
}

void FieldWalletBackend::switchWallet(
    QString configPath,
    QString storagePath)
{
    if (accountBusy())
        return;

    setWalletError(QString());
    setWalletSwitchBusy(true);

    m_logos
        ->field_wallet
        .wallet_switchAsyncResult(
            configPath,
            storagePath,
            [this](logos::AsyncResult<QString> outcome) {
                if (!outcome.ok()) {
                    refreshWallet();

                    setWalletError(
                        QStringLiteral("switch transport: ") +
                        QString::fromStdString(
                            outcome.error.message));

                    setWalletSwitchBusy(false);
                    return;
                }

                QJsonObject result;

                if (!parseObject(
                        outcome.value,
                        result)) {
                    refreshWallet();

                    setWalletError(
                        QStringLiteral(
                            "switch: invalid response"));

                    setWalletSwitchBusy(false);
                    return;
                }

                if (!result
                         .value(QStringLiteral("ok"))
                         .toBool()) {
                    refreshWallet();

                    setWalletError(
                        QStringLiteral("switch: ") +
                        resultCode(
                            result,
                            QStringLiteral(
                                "unknown error")));

                    setWalletSwitchBusy(false);
                    return;
                }

                setWalletState(
                    QStringLiteral("open"));

                setConfigPath(
                    result.value(
                        QStringLiteral("configPath"))
                        .toString());

                setStoragePath(
                    result.value(
                        QStringLiteral("storagePath"))
                        .toString());

                clearAccountData();

                // Keep the loader visible through all wallet-specific
                // data refreshes so the old wallet never flashes through.
                loadSavedWallets();
                loadAccountsAndBalance();

                setWalletSwitchBusy(false);
            },
            Timeout(120000));
}

void FieldWalletBackend::clearRecoveryPhrase()
{
    setRecoveryPhrase(QString());
    refreshWallet();
}

void FieldWalletBackend::selectAccount(QString selectedAccountId)
{
    if (accountBusy() || walletBusy() || walletSwitchBusy() ||
        walletState() != QStringLiteral("open") || !accountsLoaded()) {
        return;
    }

    const QJsonArray accounts = QJsonDocument::fromJson(
        accountsJson().toUtf8()).array();
    bool owned = false;
    for (const QJsonValue& value : accounts) {
        if (value.toObject().value(QStringLiteral("accountId")).toString() == selectedAccountId) {
            owned = true;
            break;
        }
    }
    if (!owned) {
        setWalletError(QStringLiteral("Account is not in the current wallet."));
        return;
    }

    setAccountBusy(true);
    setWalletError(QString());
    loadAccountsAndBalance(selectedAccountId);
    setAccountBusy(false);
}

void FieldWalletBackend::renameAccount(QString selectedAccountId, QString displayName)
{
    if (accountBusy() || walletBusy() || walletSwitchBusy() ||
        walletState() != QStringLiteral("open") || !accountsLoaded()) {
        return;
    }
    displayName = displayName.trimmed();
    if (displayName.isEmpty() || displayName.size() > 64) {
        setWalletError(QStringLiteral("Account names must contain 1 to 64 characters."));
        return;
    }
    QJsonArray accounts = QJsonDocument::fromJson(accountsJson().toUtf8()).array();
    for (qsizetype i = 0; i < accounts.size(); ++i) {
        QJsonObject account = accounts.at(i).toObject();
        if (account.value(QStringLiteral("accountId")).toString() != selectedAccountId)
            continue;
        QSettings settings;
        settings.setValue(accountNameKey(storagePath(), selectedAccountId), displayName);
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            setWalletError(QStringLiteral("Could not save the account name."));
            return;
        }
        account[QStringLiteral("name")] = displayName;
        accounts.replace(i, account);
        setAccountsJson(QString::fromUtf8(QJsonDocument(accounts).toJson(QJsonDocument::Compact)));
        setWalletError(QString());
        return;
    }
    setWalletError(QStringLiteral("Account is not in the current wallet."));
}

void FieldWalletBackend::createPublicAccount()
{
    createAccount(false);
}

void FieldWalletBackend::createPrivateAccount()
{
    createAccount(true);
}

void FieldWalletBackend::createAccount(bool isPrivate)
{
    if (accountBusy() || walletBusy() || walletSwitchBusy() ||
        walletState() != QStringLiteral("open") || !accountsLoaded()) {
        return;
    }

    setWalletError(QString());
    setAccountBusy(true);
    const QString expectedStorage = storagePath();
    const QString kind = isPrivate ? QStringLiteral("private") : QStringLiteral("public");

    auto completed = [this, expectedStorage, kind](logos::AsyncResult<QString> outcome) {
        if (storagePath() != expectedStorage) {
            clearAccountData();
            setWalletError(QStringLiteral("Wallet changed during account creation. Refresh accounts before continuing."));
            setAccountBusy(false);
            return;
        }
        if (!outcome.ok()) {
            setWalletError(kind + QStringLiteral(" account: request outcome is unknown. Refresh accounts before retrying. ") +
                QString::fromStdString(outcome.error.message));
            setAccountBusy(false);
            return;
        }

        QJsonObject result;
        if (!parseObject(outcome.value, result)) {
            setWalletError(QStringLiteral("Account creation returned an invalid response. Refresh accounts before retrying."));
        } else if (!result.value(QStringLiteral("ok")).toBool()) {
            setWalletError(kind + QStringLiteral(" account: ") +
                resultCode(result, QStringLiteral("unknown error")));
        } else {
            const QString createdId = result.value(QStringLiteral("accountId")).toString();
            if (createdId.isEmpty() || result.value(QStringLiteral("accountKind")).toString() != kind) {
                setWalletError(QStringLiteral("Account creation returned an invalid account. Refresh accounts before retrying."));
            } else {
                // Remember success even if the following inventory refresh fails.
                rememberWalletAccount(storagePath(), createdId, kind);
                // Select the returned account, not the first pre-existing public account.
                loadAccountsAndBalance(createdId);
            }
        }
        setAccountBusy(false);
    };

    if (isPrivate) {
        m_logos->field_wallet.wallet_create_private_accountAsyncResult(
            completed, Timeout(120000));
    } else {
        m_logos->field_wallet.wallet_create_public_accountAsyncResult(
            completed, Timeout(120000));
    }
}

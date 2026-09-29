#ifndef FIELD_WALLET_BACKEND_H
#define FIELD_WALLET_BACKEND_H

#include <QObject>
#include <QString>

#include "rep_FieldWalletBackend_source.h"

class LogosAPI;
struct LogosModules;

class FieldWalletBackend : public FieldWalletBackendSimpleSource
{
    Q_OBJECT

public:
    explicit FieldWalletBackend(
        LogosAPI* logosAPI = nullptr,
        QObject* parent = nullptr);

    ~FieldWalletBackend() override;

public slots:
    QString ping() override;

    void refreshWallet() override;
    void chooseWalletFolder() override;

    void createWallet(
        QString password,
        QString sequencerAddr) override;

    void createNamedWallet(
        QString walletName,
        QString password,
        QString sequencerAddr) override;

    void openWallet(
        QString configPath,
        QString storagePath) override;

    void switchWallet(
        QString configPath,
        QString storagePath) override;

    void renameWallet(
        QString storagePath,
        QString displayName) override;

    void clearRecoveryPhrase() override;

    void createPublicAccount() override;
    void createPrivateAccount() override;

private:
    void clearAccountData();
    void loadAccountsAndBalance();
    void loadSavedWallets();

    LogosAPI* m_logosAPI;
    LogosModules* m_logos;
};

#endif // FIELD_WALLET_BACKEND_H

#ifndef FIELD_WALLET_BACKEND_H
#define FIELD_WALLET_BACKEND_H

#include <QObject>
#include <QString>

#include "rep_FieldWalletBackend_source.h"

class LogosAPI;

class FieldWalletBackend : public FieldWalletBackendSimpleSource
{
    Q_OBJECT

public:
    explicit FieldWalletBackend(
        LogosAPI* logosAPI = nullptr,
        QObject* parent = nullptr);

public slots:
    QString ping() override;
};

#endif // FIELD_WALLET_BACKEND_H

#include "FieldWalletBackend.h"

#include <QtGlobal>

FieldWalletBackend::FieldWalletBackend(
    LogosAPI* logosAPI,
    QObject* parent)
    : FieldWalletBackendSimpleSource(parent)
{
    Q_UNUSED(logosAPI);

    setReady(true);
    setBuildLabel(QStringLiteral("mock-ui"));
}

QString FieldWalletBackend::ping()
{
    return QStringLiteral("field-wallet-ui");
}

#ifndef FIELD_WALLET_PLUGIN_H
#define FIELD_WALLET_PLUGIN_H

#include <QObject>
#include <QString>
#include <QtPlugin>

#include "FieldWalletPluginInterface.h"
#include "LogosViewPluginBase.h"

class LogosAPI;
class FieldWalletBackend;

class FieldWalletPlugin : public QObject,
                          public FieldWalletPluginInterface,
                          public FieldWalletBackendViewPluginBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(
        IID FieldWalletPluginInterface_iid
        FILE "../metadata.json")
    Q_INTERFACES(FieldWalletPluginInterface)

public:
    explicit FieldWalletPlugin(QObject* parent = nullptr);
    ~FieldWalletPlugin() override;

    QString name() const override
    {
        return QStringLiteral("field_wallet_ui");
    }

    QString version() const override
    {
        return QStringLiteral("0.1.0");
    }

    Q_INVOKABLE void initLogos(LogosAPI* api);

private:
    FieldWalletBackend* m_backend = nullptr;
};

#endif // FIELD_WALLET_PLUGIN_H

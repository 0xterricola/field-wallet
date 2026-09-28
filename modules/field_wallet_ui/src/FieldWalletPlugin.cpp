#include "FieldWalletPlugin.h"
#include "FieldWalletBackend.h"

#include <QDebug>

FieldWalletPlugin::FieldWalletPlugin(QObject* parent)
    : QObject(parent)
{
}

FieldWalletPlugin::~FieldWalletPlugin() = default;

void FieldWalletPlugin::initLogos(LogosAPI* api)
{
    if (m_backend) {
        return;
    }

    m_backend = new FieldWalletBackend(api, this);
    setBackend(m_backend);

    qDebug() << "FieldWalletPlugin: backend initialized";
}

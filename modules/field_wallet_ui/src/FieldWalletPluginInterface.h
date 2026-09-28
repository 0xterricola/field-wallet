#ifndef FIELD_WALLET_PLUGIN_INTERFACE_H
#define FIELD_WALLET_PLUGIN_INTERFACE_H

#include <QtPlugin>

#include "interface.h"

class FieldWalletPluginInterface : public PluginInterface
{
public:
    virtual ~FieldWalletPluginInterface() = default;
};

#define FieldWalletPluginInterface_iid "org.logos.FieldWalletPluginInterface"
Q_DECLARE_INTERFACE(
    FieldWalletPluginInterface,
    FieldWalletPluginInterface_iid)

#endif // FIELD_WALLET_PLUGIN_INTERFACE_H

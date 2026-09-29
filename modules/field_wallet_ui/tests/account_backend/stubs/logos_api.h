#pragma once
#include <QObject>
// Test-only transport replacement. No host, wallet files, or network are used.
class LogosAPI : public QObject {
public:
    LogosAPI(const char*, QObject* parent) : QObject(parent) {}
};

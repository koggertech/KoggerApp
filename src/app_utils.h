#pragma once

#include <QObject>
#include <QUuid>

class AppUtils : public QObject
{
    Q_OBJECT
public:
    explicit AppUtils(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString generateUuid() const {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
};

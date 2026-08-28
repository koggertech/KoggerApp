#pragma once

#include <QObject>
#include <QUuid>

class AppUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int instanceIndex READ instanceIndex CONSTANT)
    Q_PROPERTY(QString instanceSuffix READ instanceSuffix CONSTANT)

public:
    explicit AppUtils(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString generateUuid() const {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    int instanceIndex() const { return instanceIndex_; }

    QString instanceSuffix() const {
        return instanceIndex_ > 1 ? QStringLiteral(" (%1)").arg(instanceIndex_) : QString();
    }

    void setInstanceIndex(int index) { instanceIndex_ = index; }

private:
    int instanceIndex_ = 1;
};

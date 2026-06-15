#pragma once

#include <QObject>
#include <QString>

class EchogramStateSerializer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit EchogramStateSerializer(QObject* parent = nullptr);

    Q_INVOKABLE QString serialize(QObject* echogram) const;
    Q_INVOKABLE bool    deserialize(QObject* echogram, const QString& state);

    QString lastError() const;

signals:
    void lastErrorChanged();

private:
    void setLastError(const QString& text) const;

    mutable QString lastError_;
    static constexpr int kVersion = 1;
};

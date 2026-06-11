#pragma once

#include <QObject>
#include <QString>

class Notifications : public QObject
{
    Q_OBJECT

public:
    explicit Notifications(QObject* parent = nullptr);

    Q_INVOKABLE void info(const QString& text);
    Q_INVOKABLE void warning(const QString& text, const QString& tag = QString());
    Q_INVOKABLE void dismiss(const QString& tag);

signals:
    void messageRequested(int kind, const QString& text, const QString& tag);
    void dismissRequested(const QString& tag);
};

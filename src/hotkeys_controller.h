#pragma once

#include "hotkeys_manager.h"

#include <QObject>
#include <QQmlApplicationEngine>


class HotkeysController : public QObject
{
    Q_OBJECT
public:
    explicit HotkeysController(QQmlApplicationEngine* engine, QObject* parent = nullptr);

    Q_INVOKABLE bool updateHotkey(const QString& functionName, quint32 newScanCode, quint32 parameter);
    Q_INVOKABLE bool resetToDefaults();

signals:
    void hotkeysUpdated();

private:
    void refreshContextProperties();

    QQmlApplicationEngine* engine_;
    HotkeysManager manager_;
};

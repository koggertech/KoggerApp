#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "DeviceManager.h"


class DeviceManagerWrapper : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)

    /*methods*/
    DeviceManagerWrapper(QObject* parent = 0);
    ~DeviceManagerWrapper();


    DeviceManager* getWorker();

    QList<DevQProperty*> getDevList() { return getWorker()->getDevList(); }

public slots:

private slots:


signals:
    void sendOpenFile(QString path);
    void devChanged();

private:
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<DeviceManager> workerObject_;

}; // class DeviceWrapper

#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "DeviceManager.h"


class DeviceManagerWrapper : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManagerWrapper(QObject* parent = 0);
    ~DeviceManagerWrapper();


    DeviceManager* getWorker();

public slots:

private slots:


signals:
    void sendOpenFile(QString path);

private:
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<DeviceManager> workerObject_;

}; // class DeviceWrapper

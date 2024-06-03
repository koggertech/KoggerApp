#include "DeviceManagerWrapper.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent)
{
    //workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<DeviceManager>();

    // workerObject_->moveToThread(workerThread_.get());

    // connects
    QObject::connect(this, &DeviceManagerWrapper::sendOpenFile, workerObject_.get(), &DeviceManager::openFile);

    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,       this, &DeviceManagerWrapper::devChanged);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,    this, &DeviceManagerWrapper::streamChanged);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,       this, &DeviceManagerWrapper::vruChanged);

    //workerThread_->start();
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
    // if (workerThread_ && workerThread_->isRunning()) {
    //     workerThread_->quit();
    //     workerThread_->wait();
    // }

    // workerThread_->deleteLater();
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

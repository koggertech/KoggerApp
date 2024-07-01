#include "DeviceManagerWrapper.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent)
{
    //workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<DeviceManager>();

    // workerObject_->moveToThread(workerThread_.get());

    // connects

    auto connectionType = Qt::DirectConnection; // one thread
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,              connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,             connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,     connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,  connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,     connectionType);

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

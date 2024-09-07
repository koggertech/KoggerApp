#include "DeviceManagerWrapper.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent)
{
    workerObject_ = std::make_unique<DeviceManager>();

#ifdef SEPARATE_READING
    workerThread_ = std::make_unique<QThread>(this);
    auto connectionType = Qt::AutoConnection;
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,             connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,            connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,    connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged, connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,    connectionType));

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
#else
    auto connectionType = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,             connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,            connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,    connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged, connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,    connectionType);
#endif
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
#ifdef SEPARATE_READING
    for (auto& itm : deviceManagerConnections_)
        QObject::disconnect(itm);
    deviceManagerConnections_.clear();

    workerThread_->quit();
    workerThread_->wait();

    workerThread_.reset();
    workerObject_.reset();
#endif
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

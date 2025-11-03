#include "device_manager_wrapper.h"
#include "device_defs.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent),
    averageChartLosses_(0)
{
    workerObject_ = std::make_unique<DeviceManager>();

#ifdef SEPARATE_READING
    workerThread_ = std::make_unique<QThread>(this);
    auto ct = Qt::AutoConnection;
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,             ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct));

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->setObjectName("DevManThread");
    workerThread_->start();
#else
    auto ct = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct);
#endif
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
#ifdef SEPARATE_READING
    if (workerObject_) {
        QMetaObject::invokeMethod(workerObject_.get(), "shutdown", Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(workerObject_.get(), "deleteLater", Qt::QueuedConnection);
    }

    for (auto& itm : deviceManagerConnections_)
        QObject::disconnect(itm);
    deviceManagerConnections_.clear();

    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }

    workerObject_.release();
#endif
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

QUuid DeviceManagerWrapper::getFileUuid() const
{
    return QUuid(kFileUuidStr);
}

void DeviceManagerWrapper::calcAverageChartLosses()
{
    averageChartLosses_ = std::max(0, std::min(100, 100 - getWorker()->calcAverageChartLosses()));
    emit this->chartLossesChanged();
}

void DeviceManagerWrapper::setProtoBinConsoled(bool state)
{
    getWorker()->setProtoBinConsoled(state);
}

void DeviceManagerWrapper::setUSBLBeaconDirectAsk(bool is_ask)
{
    getWorker()->setUSBLBeaconDirectAsk(is_ask);
}

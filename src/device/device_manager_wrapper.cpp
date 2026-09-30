#include "device_manager_wrapper.h"
#include "device_defs.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent),
    averageChartLosses_(0),
    protoBinConsoledState_(false),
    nmeaConsoledState_(true),
    USBLBeaconDirectAskState_(false)
{
    workerObject_ = std::make_unique<DeviceManager>();

#ifdef SEPARATE_READING
    workerThread_ = std::make_unique<QThread>(this);
    auto ct = Qt::AutoConnection;
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::standAvailableChanged, this,                &DeviceManagerWrapper::standAvailableChanged,   ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,             ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::autopilotCommandAcked, this,               &DeviceManagerWrapper::autopilotCommandAcked,  ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct));

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->setObjectName("DevManThread");
#else
    auto ct = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::standAvailableChanged, this,                &DeviceManagerWrapper::standAvailableChanged,   ct);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::autopilotCommandAcked, this,               &DeviceManagerWrapper::autopilotCommandAcked,  ct);
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

void DeviceManagerWrapper::startWorkerThread()
{
#ifdef SEPARATE_READING
    if (workerThread_ && !workerThread_->isRunning()) {
        workerThread_->start();
    }
#endif
}

void DeviceManagerWrapper::initStreamList()
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "initStreamList", Qt::QueuedConnection);
#else
    workerObject_->initStreamList();
#endif
}

void DeviceManagerWrapper::startStreamDownload(int id)
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "startStreamDownload", Qt::QueuedConnection, Q_ARG(int, id));
#else
    workerObject_->startStreamDownload(id);
#endif
}

void DeviceManagerWrapper::cancelStreamDownload(int id)
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "cancelStreamDownload", Qt::QueuedConnection, Q_ARG(int, id));
#else
    workerObject_->cancelStreamDownload(id);
#endif
}

void DeviceManagerWrapper::refreshStreamList()
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "refreshStreamList", Qt::QueuedConnection);
#else
    workerObject_->refreshStreamList();
#endif
}

QString DeviceManagerWrapper::modeNameFor(int mode)
{
    switch (mode) {
    case 0:  return QStringLiteral("Manual");
    case 1:  return QStringLiteral("Acro");
    case 3:  return QStringLiteral("Steering");
    case 4:  return QStringLiteral("Hold");
    case 5:  return QStringLiteral("Loiter");
    case 6:  return QStringLiteral("Follow");
    case 7:  return QStringLiteral("Simple");
    case 8:  return QStringLiteral("Dock");
    case 9:  return QStringLiteral("Circle");
    case 10: return QStringLiteral("Auto");
    case 11: return QStringLiteral("RTL");
    case 12: return QStringLiteral("SmartRTL");
    case 15: return QStringLiteral("Guided");
    case 16: return QStringLiteral("Initialising");
    default: break;
    }
    return mode < 0 ? QString() : QStringLiteral("Mode %1").arg(mode);
}

void DeviceManagerWrapper::autopilotArm(bool arm)
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "autopilotArm", Qt::QueuedConnection, Q_ARG(bool, arm), Q_ARG(bool, false));
#else
    workerObject_->autopilotArm(arm, false);
#endif
}

void DeviceManagerWrapper::autopilotArmForce(bool arm)
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "autopilotArm", Qt::QueuedConnection, Q_ARG(bool, arm), Q_ARG(bool, true));
#else
    workerObject_->autopilotArm(arm, true);
#endif
}

void DeviceManagerWrapper::autopilotSetMode(int customMode)
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "autopilotSetMode", Qt::QueuedConnection, Q_ARG(int, customMode));
#else
    workerObject_->autopilotSetMode(customMode);
#endif
}

void DeviceManagerWrapper::autopilotStartMission()
{
#ifdef SEPARATE_READING
    QMetaObject::invokeMethod(workerObject_.get(), "autopilotStartMission", Qt::QueuedConnection);
#else
    workerObject_->autopilotStartMission();
#endif
}

void DeviceManagerWrapper::calcAverageChartLosses()
{
    averageChartLosses_ = std::max(0, std::min(100, 100 - getWorker()->calcAverageChartLosses()));
    emit this->chartLossesChanged();
}

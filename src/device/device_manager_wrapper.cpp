#include "device_manager_wrapper.h"
#include "device_defs.h"
#include "autopilot_messages.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent),
    averageChartLosses_(0),
    protoBinConsoledState_(false),
    nmeaConsoledState_(true),
    mavlinkConsoledState_(false),
    USBLBeaconDirectAskState_(false)
{
    workerObject_ = std::make_unique<DeviceManager>();
    qRegisterMetaType<AutopilotState>("AutopilotState");

#ifdef SEPARATE_READING
    workerThread_ = std::make_unique<QThread>(this);
    auto ct = Qt::AutoConnection;
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::standAvailableChanged, this,                &DeviceManagerWrapper::standAvailableChanged,   ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::onAutopilotState,       ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::autopilotCommandAcked, this,               &DeviceManagerWrapper::autopilotCommandAcked,  ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::calcAverageChartLosses, ct));

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->setObjectName("DevManThread");
#else
    auto ct = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::standAvailableChanged, this,                &DeviceManagerWrapper::standAvailableChanged,   ct);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::onAutopilotState,       ct);
    QObject::connect(workerObject_.get(), &DeviceManager::autopilotCommandAcked, this,               &DeviceManagerWrapper::autopilotCommandAcked,  ct);
    QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::calcAverageChartLosses, ct);
#endif

    qRegisterMetaType<autopilot::MissionItems>("autopilot::MissionItems");
    qRegisterMetaType<autopilot::MissionBatches>("autopilot::MissionBatches");
    missionThread_ = std::make_unique<QThread>();
    missionThread_->setObjectName("AutopilotMissionThread");
    missionTransfer_ = new autopilot::MissionTransfer();
    missionTransfer_->moveToThread(missionThread_.get());
    QObject::connect(missionThread_.get(), &QThread::finished, missionTransfer_, &QObject::deleteLater);

    DeviceManager* dm = workerObject_.get();
    QObject::connect(dm, &DeviceManager::missionFrameReceived, missionTransfer_, &autopilot::MissionTransfer::handleFrame);
    QObject::connect(dm, &DeviceManager::missionUploadStart,   missionTransfer_, &autopilot::MissionTransfer::startUpload);
    QObject::connect(dm, &DeviceManager::missionDownloadStart, missionTransfer_, &autopilot::MissionTransfer::startDownload);
    QObject::connect(dm, &DeviceManager::missionTransferAbort, missionTransfer_, &autopilot::MissionTransfer::abort);
    QObject::connect(missionTransfer_, &autopilot::MissionTransfer::sendMessage,     dm,   &DeviceManager::sendAutopilotMessage);
    QObject::connect(missionTransfer_, &autopilot::MissionTransfer::progressChanged,  this, &DeviceManagerWrapper::onMissionTransferProgress);
    QObject::connect(missionTransfer_, &autopilot::MissionTransfer::uploadFinished,   this, &DeviceManagerWrapper::onMissionUploadFinished);
    QObject::connect(missionTransfer_, &autopilot::MissionTransfer::downloadFinished, this, &DeviceManagerWrapper::onMissionDownloadFinished);
    QObject::connect(dm, &DeviceManager::missionUploadRefused, this, [this](int result, int missionType) { onMissionUploadFinished(false, result, missionType); });
    QObject::connect(dm, &DeviceManager::missionDownloadRefused, this, [this](int result) { onMissionDownloadFinished(false, result, 0, {}); });
    missionThread_->start();
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
    if (missionThread_) {
        missionThread_->quit();
        missionThread_->wait();
    }

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

bool DeviceManagerWrapper::beginMissionTransfer(bool download)
{
    if (missionTransferActive_) {
        return false;
    }
    missionTransferActive_ = true;
    missionDownloading_ = download;
    missionTransferProgress_ = 0.0;
    emit missionTransferChanged();
    return true;
}

void DeviceManagerWrapper::endMissionTransfer(bool ok)
{
    missionTransferActive_ = false;
    missionDownloading_ = false;
    if (ok) {
        missionTransferProgress_ = 1.0;
    }
    emit missionTransferChanged();
}

void DeviceManagerWrapper::downloadMission()
{
    if (!beginMissionTransfer(true)) {
        emit missionDownloadFinished(false, autopilot::MissionTransfer::ResultBusy, 0);
        return;
    }
#ifdef SEPARATE_READING
    DeviceManager* worker = workerObject_.get();
    QMetaObject::invokeMethod(worker, [worker]() { worker->startMissionDownload(); }, Qt::QueuedConnection);
#else
    workerObject_->startMissionDownload();
#endif
}

void DeviceManagerWrapper::uploadMission(const autopilot::MissionBatches& batches)
{
    if (!beginMissionTransfer(false)) {
        emit missionUploadFinished(false, autopilot::MissionTransfer::ResultBusy, 0);
        return;
    }
#ifdef SEPARATE_READING
    DeviceManager* worker = workerObject_.get();
    QMetaObject::invokeMethod(worker, [worker, batches]() { worker->startMissionUpload(batches); }, Qt::QueuedConnection);
#else
    workerObject_->startMissionUpload(batches);
#endif
}

void DeviceManagerWrapper::onAutopilotState(const AutopilotState& state)
{
    autopilotState_ = state;
    emit vruChanged();
}

void DeviceManagerWrapper::onMissionTransferProgress(int done, int total)
{
    if (!missionTransferActive_) {
        return;
    }
    missionTransferProgress_ = total > 0 ? qreal(done) / qreal(total) : 0.0;
    emit missionTransferChanged();
}

void DeviceManagerWrapper::onMissionUploadFinished(bool ok, int result, int missionType)
{
    if (!missionTransferActive_ || missionDownloading_) {
        return;
    }
    endMissionTransfer(ok);
    emit missionUploadFinished(ok, result, missionType);
}

void DeviceManagerWrapper::onMissionDownloadFinished(bool ok, int result, int missionType, const autopilot::MissionBatches& batches)
{
    if (!missionTransferActive_ || !missionDownloading_) {
        return;
    }
    endMissionTransfer(ok);
    const bool haveRoute = !batches.isEmpty() && batches.first().missionType == MavMissionTypeMission;
    emit missionDownloadFinished(ok, result, missionType);
    if (haveRoute) {
        emit missionDownloaded(batches);
    }
}

void DeviceManagerWrapper::calcAverageChartLosses()
{
    const int losses = getWorker()->calcAverageChartLosses();
    echogramDeliveryKnown_ = losses >= 0;
    averageChartLosses_ = echogramDeliveryKnown_ ? std::clamp(100 - losses, 0, 100) : 0;
    emit this->chartLossesChanged();
}

#include "device_manager_wrapper.h"
#include "device_defs.h"
#include "autopilot_messages.h"
#include <QDateTime>
#include <QVariantMap>

namespace {

constexpr int kAutopilotMessagesKept = 20;
constexpr int kRoverModeAuto = 10;
constexpr int kMissionRestartSeq = 0;

} // namespace

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

    auto ct = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,                      ct);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,                     ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,             ct);
    QObject::connect(workerObject_.get(), &DeviceManager::standAvailableChanged, this,                &DeviceManagerWrapper::standAvailableChanged,   ct);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,          ct);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::onAutopilotState,       ct);
    QObject::connect(workerObject_.get(), &DeviceManager::autopilotCommandAcked, this,               &DeviceManagerWrapper::autopilotCommandAcked,  ct);
    QObject::connect(workerObject_.get(), &DeviceManager::autopilotStatusText,  this,                &DeviceManagerWrapper::onAutopilotStatusText,  ct);
    QObject::connect(workerObject_.get(), &DeviceManager::autopilotMissionItemReached, this,         &DeviceManagerWrapper::autopilotMissionItemReached, ct);
    QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses, ct);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::calcAverageChartLosses, ct);

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
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

QUuid DeviceManagerWrapper::getFileUuid() const
{
    return QUuid(kFileUuidStr);
}

void DeviceManagerWrapper::initStreamList()
{
    workerObject_->initStreamList();
}

void DeviceManagerWrapper::startStreamDownload(int id)
{
    workerObject_->startStreamDownload(id);
}

void DeviceManagerWrapper::cancelStreamDownload(int id)
{
    workerObject_->cancelStreamDownload(id);
}

void DeviceManagerWrapper::refreshStreamList()
{
    workerObject_->refreshStreamList();
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
    workerObject_->autopilotArm(arm, false);
}

void DeviceManagerWrapper::autopilotArmForce(bool arm)
{
    workerObject_->autopilotArm(arm, true);
}

void DeviceManagerWrapper::autopilotSetMode(int customMode)
{
    workerObject_->autopilotSetMode(customMode);
}

void DeviceManagerWrapper::autopilotStartMission()
{
    workerObject_->autopilotStartMission();
}

void DeviceManagerWrapper::autopilotSetMissionCurrent(int seq)
{
    workerObject_->autopilotSetMissionCurrent(seq);
}

void DeviceManagerWrapper::autopilotSetSpeed(double speed)
{
    workerObject_->autopilotSetSpeed(float(speed));
}

bool DeviceManagerWrapper::beginMissionTransfer(bool download, bool silent)
{
    if (missionTransferActive_) {
        return false;
    }
    missionTransferActive_ = true;
    missionDownloading_ = download;
    missionTransferSilent_ = silent;
    missionTransferProgress_ = 0.0;
    emit missionTransferChanged();
    return true;
}

void DeviceManagerWrapper::endMissionTransfer(bool ok)
{
    missionTransferActive_ = false;
    missionDownloading_ = false;
    missionTransferSilent_ = false;
    if (ok) {
        missionTransferProgress_ = 1.0;
    }
    emit missionTransferChanged();
}

void DeviceManagerWrapper::cancelSilentTransfer()
{
    if (!missionTransferActive_ || !missionTransferSilent_) {
        return;
    }
    ++swallowSilentFinishes_;
    autopilot::MissionTransfer* transfer = missionTransfer_;
    QMetaObject::invokeMethod(transfer, [transfer]() { transfer->abort(autopilot::MissionTransfer::ResultCancelled); }, Qt::QueuedConnection);
    missionTransferActive_ = false;
    missionDownloading_ = false;
    missionTransferSilent_ = false;
}

void DeviceManagerWrapper::startWorkerDownload()
{
    workerObject_->startMissionDownload();
}

void DeviceManagerWrapper::downloadMission()
{
    cancelSilentTransfer();
    if (!beginMissionTransfer(true, false)) {
        emit missionDownloadFinished(false, autopilot::MissionTransfer::ResultBusy, 0);
        return;
    }
    startWorkerDownload();
}

void DeviceManagerWrapper::setRestartMissionAfterUpload(bool restart)
{
    if (restartMissionAfterUpload_ == restart) {
        return;
    }
    restartMissionAfterUpload_ = restart;
    emit restartMissionAfterUploadChanged();
}

void DeviceManagerWrapper::readMissionSilently()
{
    if (!autopilotState_.online || !beginMissionTransfer(true, true)) {
        return;
    }
    startWorkerDownload();
}

void DeviceManagerWrapper::uploadMission(const autopilot::MissionBatches& batches)
{
    cancelSilentTransfer();
    if (!beginMissionTransfer(false, false)) {
        emit missionUploadFinished(false, autopilot::MissionTransfer::ResultBusy, 0);
        return;
    }
    workerObject_->startMissionUpload(batches);
}

void DeviceManagerWrapper::onAutopilotState(const AutopilotState& state)
{
    autopilotState_ = state;
    emit vruChanged();
}

void DeviceManagerWrapper::onAutopilotStatusText(int severity, const QString& text)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("severity"), severity);
    entry.insert(QStringLiteral("text"), text);
    entry.insert(QStringLiteral("time"), QDateTime::currentMSecsSinceEpoch());
    autopilotMessages_.prepend(entry);
    while (autopilotMessages_.size() > kAutopilotMessagesKept) {
        autopilotMessages_.removeLast();
    }
    emit autopilotMessagesChanged();
    emit autopilotStatusText(severity, text);
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
    const bool routeOnVehicle = ok || missionType == MavMissionTypeFence || missionType == MavMissionTypeRally;
    if (restartMissionAfterUpload_ && routeOnVehicle && autopilotState_.online && autopilotState_.flightMode != kRoverModeAuto) {
        autopilotSetMissionCurrent(kMissionRestartSeq);
    }
    emit missionUploadFinished(ok, result, missionType);
}

void DeviceManagerWrapper::onMissionDownloadFinished(bool ok, int result, int missionType, const autopilot::MissionBatches& batches)
{
    if (swallowSilentFinishes_ > 0) {
        --swallowSilentFinishes_;
        return;
    }
    if (!missionTransferActive_ || !missionDownloading_) {
        return;
    }
    const bool silent = missionTransferSilent_;
    endMissionTransfer(ok);
    const bool haveRoute = !batches.isEmpty() && batches.first().missionType == MavMissionTypeMission;
    emit vehicleMissionRead(batches, ok, silent);
    if (silent) {
        return;
    }
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

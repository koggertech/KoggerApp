#include "firmware_upgrade_session.h"

#include <QDateTime>
#include <QFile>
#include <QUrl>

#include "dev_q_property.h"
#include "notifications.h"

extern Notifications notifications;

namespace {

constexpr int    kTickMs      = 500;
constexpr qint64 kStallMs     = 3000;
constexpr qint64 kAbandonMs   = 180000;
constexpr int    kNoDevice    = -2;

QString fileLabelOf(const QString& path)
{
    const QString normalized = QString(path).replace('\\', '/');
    const int slash = normalized.lastIndexOf('/');
    return slash >= 0 ? normalized.mid(slash + 1) : normalized;
}

QString deviceLabelOf(DevQProperty* dev)
{
    const QString name = dev->devName();
    const uint32_t serialNumber = dev->devSerialNumber();
    return serialNumber ? QStringLiteral("%1 (SN %2)").arg(name).arg(serialNumber) : name;
}

} // namespace

FirmwareUpgradeSession::FirmwareUpgradeSession(QObject* parent)
    : QObject(parent)
{
    ticker_.setTimerType(Qt::CoarseTimer);
    connect(&ticker_, &QTimer::timeout, this, &FirmwareUpgradeSession::onTick);
}

bool FirmwareUpgradeSession::start(const QString& firmwarePath, DevQProperty* dev)
{
    const QUrl url(firmwarePath);
    QFile file(url.isLocalFile() ? url.toLocalFile() : firmwarePath);
    const QString fileLabel = fileLabelOf(file.fileName());

    if (!file.open(QIODevice::ReadOnly)) {
        notifications.warning(tr("Failed to open firmware file: %1").arg(fileLabel));
        return false;
    }

    if (!dev) {
        notifications.warning(tr("No device to flash with file %1").arg(fileLabel));
        return false;
    }

    begin(deviceLabelOf(dev), fileLabel, dev->devSerialNumber());
    dev->sendUpdateFW(file.readAll());

    return true;
}

void FirmwareUpgradeSession::handleStatus(int status)
{
    if (!active_) {
        return;
    }

    lastActivityMs_ = QDateTime::currentMSecsSinceEpoch();

    if (status == DevDriver::successUpgrade) {
        finish(true, status);
        return;
    }

    if (status == DevDriver::failUpgrade) {
        finish(false, status);
        return;
    }

    percent_ = qBound(0, status, 100);
    publish();
}

void FirmwareUpgradeSession::begin(const QString& deviceLabel, const QString& fileName, uint32_t serialNumber)
{
    if (active_) {
        finish(false, kNoDevice);
    }

    tag_ = QStringLiteral("fw-upgrade-%1").arg(serialNumber);
    deviceLabel_ = deviceLabel;
    fileName_ = fileName;
    percent_ = 0;
    lastActivityMs_ = QDateTime::currentMSecsSinceEpoch();
    active_ = true;

    ticker_.start(kTickMs);
    publish();
}

void FirmwareUpgradeSession::publish()
{
    if (!active_) {
        return;
    }

    const bool stalled = (QDateTime::currentMSecsSinceEpoch() - lastActivityMs_) > kStallMs;

    if (stalled) {
        notifications.progress(tr("Flashing %1 with %2: waiting for the device")
                                   .arg(deviceLabel_, fileName_),
                               tag_, -1);
    }
    else {
        notifications.progress(tr("Flashing %1 with %2: %3%")
                                   .arg(deviceLabel_, fileName_)
                                   .arg(percent_),
                               tag_, percent_);
    }
}

void FirmwareUpgradeSession::finish(bool success, int status)
{
    if (!active_) {
        return;
    }

    active_ = false;
    ticker_.stop();
    notifications.dismiss(tag_);

    if (success) {
        notifications.info(tr("Device %1 successfully flashed with file %2")
                               .arg(deviceLabel_, fileName_));
    }
    else if (status == kNoDevice) {
        notifications.warning(tr("Flashing device %1 with file %2 interrupted: the device did not come back")
                                  .arg(deviceLabel_, fileName_));
    }
    else {
        notifications.warning(tr("Failed to flash device %1 with file %2 (error code %3)")
                                  .arg(deviceLabel_, fileName_)
                                  .arg(status));
    }
}

void FirmwareUpgradeSession::onTick()
{
    if (!active_) {
        ticker_.stop();
        return;
    }

    if (QDateTime::currentMSecsSinceEpoch() - lastActivityMs_ > kAbandonMs) {
        finish(false, kNoDevice);
        return;
    }

    publish();
}

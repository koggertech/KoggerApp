#include "instance_lock.h"

#include <chrono>

#include <QCryptographicHash>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>


namespace {

constexpr std::chrono::milliseconds kAcquireTimeout{ 100 };

QString slotDirectory()
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return runtime.isEmpty() ? QDir::tempPath() : runtime;
}

QString userTag()
{
    const QByteArray home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(home, QCryptographicHash::Sha1).toHex().left(8));
}

} // namespace


InstanceLock::InstanceLock() = default;

InstanceLock::~InstanceLock() = default;

QString InstanceLock::slotPath(int slot)
{
    return QStringLiteral("%1/KoggerApp-%2-instance-%3.lock")
        .arg(slotDirectory(), userTag(), QString::number(slot));
}

bool InstanceLock::acquire()
{
    for (int slot = 1; slot <= kMaxInstances; ++slot) {
        auto candidate = std::make_unique<QLockFile>(slotPath(slot));
        candidate->setStaleLockTime(std::chrono::milliseconds::zero());

        if (candidate->tryLock(kAcquireTimeout)) {
            lock_ = std::move(candidate);
            index_ = slot;
            return true;
        }

        if (candidate->error() != QLockFile::LockFailedError) {
            index_ = slot; // lock file unusable (permissions/disk) — never block startup on it
            return true;
        }
    }

    return false;
}

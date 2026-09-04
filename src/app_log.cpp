#include "app_log.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageLogContext>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>

namespace {

const char* levelTag(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return "DBG";
    case QtInfoMsg:     return "INF";
    case QtWarningMsg:  return "WRN";
    case QtCriticalMsg: return "CRT";
    case QtFatalMsg:    return "FTL";
    }
    return "UNK";
}

QString threadTag()
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
}

bool appendFileTo(const QString& sourcePath, const QString& targetPath)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        return false;
    }

    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return false;
    }

    constexpr qint64 kChunkBytes = 64 * 1024;
    while (!source.atEnd()) {
        const QByteArray chunk = source.read(kChunkBytes);
        if (chunk.isEmpty() || target.write(chunk) != chunk.size()) {
            return false;
        }
    }

    target.flush();
    return true;
}

}

AppLog& AppLog::instance()
{
    static AppLog logger;
    return logger;
}

QString AppLog::defaultDirectory()
{
#ifdef Q_OS_ANDROID
    return QStringLiteral("/storage/emulated/0/Documents/KoggerApp/AppLogs");
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/AppLogs");
#endif
}

QString AppLog::fallbackDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/AppLogs");
}

bool AppLog::start(const QString& dirPath, const QString& baseName, qint64 maxBytes, int maxFiles)
{
    QMutexLocker locker(&mutex_);

    if (active_) {
        return true;
    }

    if (dirPath.isEmpty() || baseName.isEmpty() || maxFiles < 1) {
        return false;
    }

    base_ = baseName;
    maxBytes_ = maxBytes;
    maxFiles_ = maxFiles;

    const QStringList candidates = dirPath == fallbackDirectory()
                                       ? QStringList{ dirPath }
                                       : QStringList{ dirPath, fallbackDirectory() };

    for (const QString& candidate : candidates) {
        if (!QDir().mkpath(candidate)) {
            continue;
        }

        dir_ = candidate;
        file_.setFileName(filePathUnlocked(0));
        if (file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            active_ = true;
            break;
        }
    }

    if (!active_) {
        dir_.clear();
        return false;
    }

    written_ = file_.size();

    appendUnlocked(QtInfoMsg,
                   QStringLiteral("session"),
                   QStringLiteral("--- log opened: %1 %2, pid %3 ---")
                       .arg(QCoreApplication::applicationName(),
                            QCoreApplication::applicationVersion())
                       .arg(QCoreApplication::applicationPid()));

    return true;
}

bool AppLog::relocate(const QString& dirPath)
{
    QMutexLocker locker(&mutex_);

    if (!active_ || dirPath.isEmpty() || dirPath == dir_) {
        return active_;
    }

    if (!QDir().mkpath(dirPath)) {
        return false;
    }

    const QString sourceDir = dir_;

    appendUnlocked(QtInfoMsg,
                   QStringLiteral("session"),
                   QStringLiteral("--- log continues in %1 ---").arg(dirPath));

    file_.flush();
    file_.close();

    for (int i = maxFiles_ - 1; i >= 1; --i) {
        const QString source = filePathIn(sourceDir, i);
        if (!QFile::exists(source)) {
            continue;
        }

        const QString target = filePathIn(dirPath, i);
        if (QFile::exists(target)) {
            QFile::remove(source);
            continue;
        }

        if (QFile::copy(source, target)) {
            QFile::remove(source);
        }
    }

    const QString activeSource = filePathIn(sourceDir, 0);
    if (QFile::exists(activeSource) && appendFileTo(activeSource, filePathIn(dirPath, 0))) {
        QFile::remove(activeSource);
    }

    dir_ = dirPath;
    file_.setFileName(filePathUnlocked(0));
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        dir_ = sourceDir;
        file_.setFileName(filePathUnlocked(0));
        if (!file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            active_ = false;
        }
        return false;
    }

    written_ = file_.size();

    appendUnlocked(QtInfoMsg,
                   QStringLiteral("session"),
                   QStringLiteral("--- log moved from %1 ---").arg(sourceDir));

    return true;
}

void AppLog::stop()
{
    QMutexLocker locker(&mutex_);

    if (!active_) {
        return;
    }

    appendUnlocked(QtInfoMsg, QStringLiteral("session"), QStringLiteral("--- log closed ---"));

    active_ = false;
    if (file_.isOpen()) {
        file_.flush();
        file_.close();
    }
}

bool AppLog::isActive() const
{
    QMutexLocker locker(&mutex_);
    return active_;
}

QString AppLog::directory() const
{
    QMutexLocker locker(&mutex_);
    return dir_;
}

QString AppLog::currentFilePath() const
{
    QMutexLocker locker(&mutex_);
    return dir_.isEmpty() ? QString() : filePathUnlocked(0);
}

QString AppLog::filePathIn(const QString& dir, int index) const
{
    return index == 0 ? QStringLiteral("%1/%2.log").arg(dir, base_)
                      : QStringLiteral("%1/%2.%3.log").arg(dir, base_).arg(index);
}

QString AppLog::filePathUnlocked(int index) const
{
    return filePathIn(dir_, index);
}

void AppLog::write(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const QString category = context.category ? QString::fromLatin1(context.category)
                                              : QStringLiteral("default");
    QMutexLocker locker(&mutex_);
    appendUnlocked(type, category, msg);
}

void AppLog::writeRaw(QtMsgType type, const QString& category, const QString& msg)
{
    QMutexLocker locker(&mutex_);
    appendUnlocked(type, category, msg);
}

void AppLog::appendUnlocked(QtMsgType type, const QString& category, const QString& msg)
{
    if (!file_.isOpen()) {
        return;
    }

    QString payload = msg;
    payload.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    payload.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    payload.replace(QLatin1Char('\n'), QLatin1String("\n        "));

    const QString line = QStringLiteral("%1 %2 [%3] %4: %5\n")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  QString::fromLatin1(levelTag(type)),
                                  threadTag(),
                                  category,
                                  payload);

    const qint64 count = file_.write(line.toUtf8());
    if (count < 0) {
        file_.close();
        return;
    }

    written_ += count;
    file_.flush();

    if (maxBytes_ > 0 && written_ >= maxBytes_) {
        rotateUnlocked();
    }
}

void AppLog::rotateUnlocked()
{
    file_.flush();
    file_.close();

    QFile::remove(filePathUnlocked(maxFiles_ - 1));
    for (int i = maxFiles_ - 2; i >= 1; --i) {
        QFile::rename(filePathUnlocked(i), filePathUnlocked(i + 1));
    }
    if (maxFiles_ > 1) {
        QFile::rename(filePathUnlocked(0), filePathUnlocked(1));
    }
    else {
        QFile::remove(filePathUnlocked(0));
    }

    file_.setFileName(filePathUnlocked(0));
    if (file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        written_ = file_.size();
    }
    else {
        written_ = 0;
    }
}

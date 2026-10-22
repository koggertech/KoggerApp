#pragma once

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSaveFile>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>

#include "kogger_revision.h"

class AppUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int instanceIndex READ instanceIndex CONSTANT)
    Q_PROPERTY(QString instanceSuffix READ instanceSuffix CONSTANT)
    Q_PROPERTY(QString gitRevision READ gitRevision CONSTANT)

public:
    explicit AppUtils(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString generateUuid() const {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    QString gitRevision() const { return QStringLiteral(KOGGER_GIT_REVISION); }

    Q_INVOKABLE QString licenseText(const QString& name) const {
        if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))
            || name.contains(QLatin1String(".."))) {
            return QString();
        }
        QFile file(QStringLiteral(":/licenses/") + name);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }

    // Accepts a local path or a file: URL, as FileDialog hands them over.
    Q_INVOKABLE QString toLocalPath(const QString& pathOrUrl) const {
        if (pathOrUrl.startsWith(QLatin1String("file:"), Qt::CaseInsensitive))
            return QUrl(pathOrUrl).toLocalFile();
        return pathOrUrl;
    }

    Q_INVOKABLE QString toFileUrl(const QString& path) const {
        return QUrl::fromLocalFile(toLocalPath(path)).toString();
    }

    Q_INVOKABLE QString folderUrlOf(const QString& pathOrUrl) const {
        const QString path = toLocalPath(pathOrUrl);
        if (path.isEmpty()) return QString();
        return QUrl::fromLocalFile(QFileInfo(path).absolutePath()).toString();
    }

    Q_INVOKABLE QVariantMap fileInfo(const QString& pathOrUrl) const {
        const QFileInfo fi(toLocalPath(pathOrUrl));
        QVariantMap out;
        out[QStringLiteral("exists")] = fi.exists();
        out[QStringLiteral("size")] = static_cast<double>(fi.exists() ? fi.size() : 0);
        out[QStringLiteral("modifiedMs")] = fi.exists() ? static_cast<double>(fi.lastModified().toMSecsSinceEpoch()) : 0.0;
        out[QStringLiteral("fileName")] = fi.fileName();
        out[QStringLiteral("baseName")] = fi.completeBaseName();
        out[QStringLiteral("path")] = QDir::toNativeSeparators(fi.absoluteFilePath());
        out[QStringLiteral("writable")] = fi.exists() ? fi.isWritable() : QFileInfo(fi.absolutePath()).isWritable();
        return out;
    }

    // Reads UTF-8 text. On failure returns an empty string and lastFileError() says why.
    Q_INVOKABLE QString readTextFile(const QString& pathOrUrl) {
        lastFileError_.clear();
        QFile file(toLocalPath(pathOrUrl));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lastFileError_ = file.errorString();
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }

    // Atomic: written to a temporary file and renamed, so a failed write keeps the old file.
    Q_INVOKABLE bool writeTextFile(const QString& pathOrUrl, const QString& text) {
        lastFileError_.clear();
        QSaveFile file(toLocalPath(pathOrUrl));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            lastFileError_ = file.errorString();
            return false;
        }
        const QByteArray bytes = text.toUtf8();
        if (file.write(bytes) != bytes.size()) {
            lastFileError_ = file.errorString();
            file.cancelWriting();
            return false;
        }
        if (!file.commit()) {
            lastFileError_ = file.errorString();
            return false;
        }
        return true;
    }

    Q_INVOKABLE QString lastFileError() const { return lastFileError_; }

    int instanceIndex() const { return instanceIndex_; }

    QString instanceSuffix() const {
        return instanceIndex_ > 1 ? QStringLiteral(" (%1)").arg(instanceIndex_) : QString();
    }

    void setInstanceIndex(int index) { instanceIndex_ = index; }

private:
    int instanceIndex_ = 1;
    QString lastFileError_;
};

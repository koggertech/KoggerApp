#include "video_stream_pool.h"

#include "link_defs.h"
#include "video_stream.h"

#include <QDebug>
#include <QMetaObject>
#include <QVariantMap>

namespace {

int roleByName(const QAbstractItemModel* model, const char* name)
{
    const QHash<int, QByteArray> names = model->roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        if (it.value() == name) {
            return it.key();
        }
    }
    return -1;
}

bool looksLocal(const QString& unified)
{
    if (unified.startsWith(QLatin1Char('/'))) {
        return true;
    }
    return unified.size() >= 3 && unified.at(0).isLetter() && unified.at(1) == QLatin1Char(':')
        && unified.at(2) == QLatin1Char('/');
}

QString toLocalSpec(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));

    int slashes = 0;
    while (slashes < path.size() && path.at(slashes) == QLatin1Char('/')) {
        ++slashes;
    }

    const bool driveFollows = path.size() >= slashes + 2 && path.at(slashes).isLetter()
                           && path.at(slashes + 1) == QLatin1Char(':');
    if (driveFollows) {
        return path.mid(slashes);
    }
    if (slashes >= 3) {
        return path.mid(slashes - 1);
    }
    return path;
}

} // namespace

VideoStreamPool::VideoStreamPool(QObject* parent)
    : QObject(parent)
{
}

VideoStreamPool::~VideoStreamPool() = default;

void VideoStreamPool::setSourceModel(QAbstractItemModel* model)
{
    if (model_ == model) {
        return;
    }

    if (model_) {
        model_->disconnect(this);
    }

    model_ = model;

    if (model_) {
        connect(model_, &QAbstractItemModel::dataChanged, this, &VideoStreamPool::refresh);
        connect(model_, &QAbstractItemModel::rowsInserted, this, &VideoStreamPool::refresh);
        connect(model_, &QAbstractItemModel::rowsRemoved, this, &VideoStreamPool::refresh);
        connect(model_, &QAbstractItemModel::rowsMoved, this, &VideoStreamPool::refresh);
        connect(model_, &QAbstractItemModel::modelReset, this, &VideoStreamPool::refresh);
    }

    refresh();
}

QString VideoStreamPool::buildUrl(const QString& address)
{
    QString trimmed = address.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (trimmed.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return QStringLiteral("file:") + toLocalSpec(trimmed.mid(5));
    }

    QString unified = trimmed;
    unified.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (looksLocal(unified)) {
        return QStringLiteral("file:") + toLocalSpec(unified);
    }

    if (trimmed.contains(QStringLiteral("://"))) {
        return trimmed;
    }
    return QStringLiteral("rtsp://") + trimmed;
}

void VideoStreamPool::refresh()
{
    if (refreshing_) {
        refreshPending_ = true;
        return;
    }

    refreshing_ = true;
    rebuild();
    refreshing_ = false;

    if (refreshPending_) {
        refreshPending_ = false;
        refresh();
    }
}

void VideoStreamPool::rebuild()
{
    if (!model_) {
        return;
    }

    const int uuidRole = roleByName(model_, "Uuid");
    const int typeRole = roleByName(model_, "LinkType");
    const int addressRole = roleByName(model_, "Address");
    const int statusRole = roleByName(model_, "ConnectionStatus");

    if (uuidRole < 0 || typeRole < 0 || addressRole < 0 || statusRole < 0) {
        return;
    }

    QVariantList descriptors;
    QHash<QString, VideoStream*> alive;

    const int rows = model_->rowCount();
    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = model_->index(row, 0);
        const int linkType = model_->data(index, typeRole).toInt();
        if (linkType != static_cast<int>(LinkType::kLinkVideo)) {
            continue;
        }

        const QString uuid = model_->data(index, uuidRole).toString();
        if (uuid.isEmpty()) {
            continue;
        }

        const QString address = model_->data(index, addressRole).toString();
        const QString url = buildUrl(address);
        const bool wanted = model_->data(index, statusRole).toBool() && !url.isEmpty();

        VideoStream* stream = streams_.value(uuid, nullptr);
        if (!stream) {
            stream = new VideoStream(this);
            connect(stream, &VideoStream::activeChanged, this, &VideoStreamPool::refresh);
            connect(stream, &VideoStream::hasFrameChanged, this, &VideoStreamPool::refresh);
            connect(stream, &VideoStream::hasFrameChanged, this, [this, uuid]() {
                VideoStream* changed = streams_.value(uuid, nullptr);
                if (!changed) {
                    return;
                }
                if (changed->hasFrame()) {
                    qInfo().noquote() << QStringLiteral("VIDEO: %1 first frame %2x%3")
                                             .arg(labelFor(uuid))
                                             .arg(changed->sourceWidth())
                                             .arg(changed->sourceHeight());
                }
                else {
                    qInfo().noquote() << QStringLiteral("VIDEO: %1 no frame").arg(labelFor(uuid));
                }
            });
            connect(stream, &VideoStream::retriesExhausted, this, [this, uuid]() {
                VideoStream* failing = streams_.value(uuid, nullptr);
                if (!failing) {
                    return;
                }
                failedUrls_.insert(uuid, failing->url());
                failing->stopWithFailure();
                refresh();
            });
            streams_.insert(uuid, stream);
        }
        alive.insert(uuid, stream);

        if (wanted) {
            if (stream->url() != url && failedUrls_.value(uuid) != url) {
                failedUrls_.remove(uuid);
                stream->start(url);
            }
        }
        else {
            failedUrls_.remove(uuid);
            if (!stream->url().isEmpty()) {
                stream->stop();
            }
        }

        const bool streaming = wanted && stream->hasFrame();
        if (reportedStreaming_.value(uuid, false) != streaming) {
            reportedStreaming_.insert(uuid, streaming);
            emit streamingChanged(uuid, streaming);
        }

        QVariantMap descriptor;
        descriptor[QStringLiteral("uuid")] = uuid;
        descriptor[QStringLiteral("label")] = address.isEmpty() ? url : address;
        descriptor[QStringLiteral("url")] = url;
        descriptor[QStringLiteral("open")] = wanted;
        descriptor[QStringLiteral("hasFrame")] = stream->hasFrame();
        descriptor[QStringLiteral("failed")] = failedUrls_.value(uuid) == url && !url.isEmpty();
        descriptors.append(descriptor);
    }

    QHash<QString, int> labelUses;
    for (int i = 0; i < descriptors.size(); ++i) {
        QVariantMap descriptor = descriptors.at(i).toMap();
        const QString base = descriptor.value(QStringLiteral("label")).toString();
        const int use = ++labelUses[base];
        if (use > 1) {
            descriptor[QStringLiteral("label")] = QStringLiteral("%1 (%2)").arg(base).arg(use);
            descriptors[i] = descriptor;
        }
    }

    for (auto it = streams_.begin(); it != streams_.end();) {
        if (alive.contains(it.key())) {
            ++it;
            continue;
        }
        it.value()->stop();
        it.value()->deleteLater();
        failedUrls_.remove(it.key());
        if (reportedStreaming_.take(it.key())) {
            emit streamingChanged(it.key(), false);
        }
        it = streams_.erase(it);
    }

    if (descriptors != descriptors_) {
        descriptors_ = descriptors;
        emit streamsChanged();
    }
}

QObject* VideoStreamPool::streamFor(const QString& uuid) const
{
    return streams_.value(uuid, nullptr);
}

bool VideoStreamPool::isStreamOpen(const QString& uuid) const
{
    if (uuid.isEmpty()) {
        return false;
    }

    for (const QVariant& entry : descriptors_) {
        const QVariantMap descriptor = entry.toMap();
        if (descriptor.value(QStringLiteral("uuid")).toString() == uuid) {
            return descriptor.value(QStringLiteral("open")).toBool();
        }
    }
    return false;
}

QString VideoStreamPool::labelFor(const QString& uuid) const
{
    for (const QVariant& entry : descriptors_) {
        const QVariantMap descriptor = entry.toMap();
        if (descriptor.value(QStringLiteral("uuid")).toString() == uuid) {
            return descriptor.value(QStringLiteral("label")).toString();
        }
    }
    return QString();
}

QString VideoStreamPool::firstOpenUuid() const
{
    for (const QVariant& entry : descriptors_) {
        const QVariantMap descriptor = entry.toMap();
        if (descriptor.value(QStringLiteral("open")).toBool()) {
            return descriptor.value(QStringLiteral("uuid")).toString();
        }
    }
    return QString();
}

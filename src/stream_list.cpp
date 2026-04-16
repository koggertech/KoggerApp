#include "stream_list.h"

#include <algorithm>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QVariantMap>
#include <cstring>

#include <core.h>
extern Core core;

StreamList::StreamList(QObject* parent) : QObject(parent)
{
}

StreamList::~StreamList()
{
    if (!_updater) {
        return;
    }

    _updater->stop();
    delete _updater;
}

void StreamList::initTimer()
{
    if (_updater) {
        return;
    }

    _updater = new QTimer(this);
    connect(_updater, &QTimer::timeout, this, &StreamList::process);
    _updater->start(100);
}

void StreamList::append(FrameParser* frame)
{
    if (!frame->isStream()) {
        return;
    }

    const uint16_t currentId = frame->streamId();
    if (_activeDownloadId >= 0 && currentId != _activeDownloadId) {
        return;
    }

    if (_lastStreamId != currentId) {
        if (_streams.contains(currentId)) {
            _lastStreamId = currentId;
            _lastStream = &_streams[currentId];
        }

        if (_lastStreamId != currentId) {
            updateStream(currentId);
            _lastStream = getStream(currentId);
            _lastStreamId = currentId;
        }
    }

    if (!_lastStream) {
        return;
    }

    insert(_lastStream, frame->frame(), frame->streamOffset(), frame->frameLen());
}

void StreamList::parse(FrameParser* frame)
{
    if (frame->id() != ID_STREAM || frame->type() != CONTENT || frame->resp()) {
        return;
    }

    if (frame->ver() == v0) {
        int itemCnt = frame->payloadLen() / 12;
        while (itemCnt--) {
            const int id = frame->read<U2>();
            const uint16_t flags = frame->read<U2>();
            const uint32_t size = frame->read<U4>();
            const uint32_t unixt = frame->read<U4>();

            updateStream(id);
            Stream* stream = getStream(id);
            if (!stream) {
                continue;
            }

            stream->listedSize = size;
            if (stream->uploadingState == UploadingIdle || _activeDownloadId != id) {
                stream->size = size;
            }
            stream->unixt = unixt;
            stream->recordingState = static_cast<RecordingState>(flags & 0x3);

            updateStream(id);
            _isListChenged = true;
        }
        return;
    }

    if (frame->ver() == v1) {
        if (frame->payloadLen() < 24) {
            return;
        }

        RetrievalDiagnostic diagnostic;
        diagnostic.logId = frame->read<U2>();
        diagnostic.problemId = static_cast<RetrievalDiagnostic::ProblemId>(frame->read<U2>());
        diagnostic.fileState = static_cast<RetrievalDiagnostic::FileState>(frame->read<U2>());
        diagnostic.reserved = frame->read<U2>();
        diagnostic.requestedStart = frame->read<U4>();
        diagnostic.requestedEnd = frame->read<U4>();
        diagnostic.actualFileSize = frame->read<U4>();
        diagnostic.visibleFileSize = frame->read<U4>();
        handleRetrievalDiagnostic(diagnostic);
    }
}

void StreamList::updateStream(int id)
{
    Stream& stream = _streams[id];
    stream.id = id;
    const uint32_t visibleSize = stream.listedSize > 0 ? stream.listedSize : stream.size;
    _modelList.appendEvent(stream.id,
                           visibleSize,
                           coveredSize(stream),
                           QStringLiteral("-"),
                           stream.recordingState,
                           stream.uploadingState,
                           statusText(stream),
                           stream.savedFilePath,
                           stream.retryRound,
                           missingRanges(stream).size());
    emitStateChanged();
}

void StreamList::startDownload(int id)
{
    if (_activeDownloadId >= 0 && _activeDownloadId != id) {
        if (Stream* previous = getStream(_activeDownloadId)) {
            completeDownload(previous, false);
        }
    }

    updateStream(id);
    Stream* stream = getStream(id);
    if (!stream) {
        return;
    }

    _activeDownloadId = id;
    _lastStreamId = static_cast<uint16_t>(id);
    _lastStream = stream;

    stream->data.clear();
    stream->gaps.clear();
    stream->savedFilePath.clear();
    stream->lastDiagnostic = {};
    stream->lastDiagnosticText.clear();
    stream->uploadingState = Uploading;
    stream->_counter = {};
    stream->size = stream->listedSize;
    stream->downloadStartedAt = timestamp();
    stream->lastDataAt = 0;
    stream->lastRequestAt = 0;
    stream->lastRequestedMissingBytes = stream->size;
    stream->lastRequestedMissingRanges = 1;
    stream->retryRound = 0;
    stream->noProgressRetryRounds = 0;
    stream->requestInFlight = false;
    stream->currentRequestRanges.clear();

    QVariantList fullRanges;
    fullRanges.append(QVariantMap{{QStringLiteral("start"), 0u}, {QStringLiteral("end"), 0x0FFFFFFFu}});
    dispatchRequest(stream, batchRanges(fullRanges, 1), false);
    updateStream(id);
}

void StreamList::debugAddGap(uint32_t start, uint32_t size)
{
    Q_UNUSED(start);
    Q_UNUSED(size);
}

void StreamList::debugSearchGap(uint32_t start, uint32_t size)
{
    Q_UNUSED(start);
    Q_UNUSED(size);
}

void StreamList::insert(Stream* stream, uint8_t* frame, uint32_t offset, uint16_t size)
{
    if (!stream || size == 0) {
        return;
    }

    const uint32_t end = offset + size;
    QList<Fragment>& gaps = stream->gaps;
    QByteArray& data = stream->data;

    _timeLastGapsInsert = timestamp();
    _isInserting = true;

    if (stream->size < end) {
        stream->size = end;
    }

    const uint32_t previousSize = static_cast<uint32_t>(data.size());
    if (previousSize < offset) {
        Fragment newFragment = {
            .start = previousSize,
            .end = offset,
            .timestamp = timestamp(),
            .status = FragmentStatus::FragmentNew
        };
        gaps.append(newFragment);
        stream->_counter._lostFragments++;
        debugAddGap(offset, offset - previousSize);
    } else if (previousSize > offset) {
        debugSearchGap(offset, size);
        trimGaps(stream, offset, end);
        stream->_counter._fillFragments++;
    } else {
        stream->_counter._fragments++;
    }

    if (static_cast<uint32_t>(data.size()) < end) {
        data.resize(static_cast<int>(end));
    }

    memcpy(data.data() + offset, frame, size);
    stream->lastDataAt = timestamp();

    if (isDownloadComplete(*stream)) {
        completeDownload(stream, true);
    } else {
        updateStream(stream->id);
    }

    _isInserting = false;
}

void StreamList::trimGaps(Stream* stream, uint32_t offset, uint32_t end)
{
    if (!stream || offset >= end) {
        return;
    }

    QList<Fragment>& gaps = stream->gaps;
    for (int i = 0; i < gaps.size(); ++i) {
        Fragment& gap = gaps[i];
        if (end <= gap.start || offset >= gap.end) {
            continue;
        }

        const uint32_t gapStart = gap.start;
        const uint32_t gapEnd = gap.end;

        if (offset <= gapStart && end >= gapEnd) {
            gaps.removeAt(i);
            --i;
            continue;
        }

        if (offset <= gapStart) {
            gap.start = end;
            gap.timestamp = timestamp();
            gap.status = FragmentStatus::FragmentProcessing;
            continue;
        }

        if (end >= gapEnd) {
            gap.end = offset;
            gap.timestamp = timestamp();
            gap.status = FragmentStatus::FragmentProcessing;
            continue;
        }

        gap.end = offset;
        gap.timestamp = timestamp();
        gap.status = FragmentStatus::FragmentProcessing;

        Fragment newFragment = {
            .start = end,
            .end = gapEnd,
            .timestamp = timestamp(),
            .status = FragmentStatus::FragmentNew
        };
        gaps.insert(i + 1, newFragment);
        break;
    }
}

uint32_t StreamList::coveredSize(const Stream& stream) const
{
    uint32_t covered = static_cast<uint32_t>(stream.data.size());
    for (const Fragment& gap : stream.gaps) {
        if (gap.end > gap.start) {
            const uint32_t gapSize = gap.end - gap.start;
            covered = gapSize > covered ? 0 : covered - gapSize;
        }
    }

    if (stream.size > 0 && covered > stream.size) {
        covered = stream.size;
    }

    return covered;
}

uint32_t StreamList::missingSize(const Stream& stream) const
{
    return stream.size > coveredSize(stream) ? stream.size - coveredSize(stream) : 0;
}

bool StreamList::isDownloadComplete(const Stream& stream) const
{
    return stream.size > 0 && coveredSize(stream) >= stream.size && missingRanges(stream).isEmpty();
}

QString StreamList::statusText(const Stream& stream) const
{
    if (!stream.lastDiagnosticText.isEmpty()) {
        switch (stream.uploadingState) {
        case Uploading:
        case UploadingRetrying:
            return QStringLiteral("%1 | %2")
                .arg(stream.uploadingState == UploadingRetrying
                         ? QStringLiteral("Retrying (%1)").arg(stream.retryRound)
                         : QStringLiteral("Downloading"))
                .arg(stream.lastDiagnosticText);
        case UploadingIncomplete:
            return QStringLiteral("Incomplete | %1").arg(stream.lastDiagnosticText);
        default:
            break;
        }
    }

    switch (stream.uploadingState) {
    case Uploading:
        return QStringLiteral("Downloading");
    case UploadingRetrying:
        return QStringLiteral("Retrying (%1)").arg(stream.retryRound);
    case UploadingComplete:
        return QStringLiteral("Complete");
    case UploadingIncomplete:
        return QStringLiteral("Incomplete");
    case UploadingIdle:
    default:
        return QStringLiteral("Idle");
    }
}

QString StreamList::resolveRecorderDirectoryPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp/recorder";
}

bool StreamList::saveStreamToFile(Stream* stream)
{
    if (!stream || !isDownloadComplete(*stream)) {
        return false;
    }

    QDir dir;
    const QString recorderPath = resolveRecorderDirectoryPath();
    if (!dir.mkpath(recorderPath)) {
        core.consoleWarning("Recorder download directory could not be created");
        return false;
    }

    const QString fileName = QString("recorder_log_%1_%2.kp2")
                                 .arg(stream->id)
                                 .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd_hh.mm.ss"));
    const QString fullPath = recorderPath + "/" + fileName;

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        core.consoleWarning("Recorder download file could not be created");
        return false;
    }

    const qint64 expectedSize = static_cast<qint64>(stream->size);
    const qint64 written = file.write(stream->data.constData(), expectedSize);
    file.close();

    if (written != expectedSize) {
        core.consoleWarning("Recorder download file write failed");
        return false;
    }

    stream->savedFilePath = fullPath;
    core.consoleInfo("Recorder log saved: " + fullPath);
    return true;
}

void StreamList::completeDownload(Stream* stream, bool isComplete)
{
    if (!stream) {
        return;
    }

    stream->requestInFlight = false;
    stream->currentRequestRanges.clear();

    if (isComplete) {
        stream->uploadingState = saveStreamToFile(stream) ? UploadingComplete : UploadingIncomplete;
    } else {
        stream->uploadingState = UploadingIncomplete;
    }

    _activeDownloadId = -1;
    _lastStream = nullptr;
    _lastStreamId = 0xFFFF;
    updateStream(stream->id);
}

QVariantList StreamList::missingRanges(const Stream& stream) const
{
    QVariantList result;
    QList<Fragment> sortedGaps = stream.gaps;
    std::sort(sortedGaps.begin(), sortedGaps.end(), [](const Fragment& lhs, const Fragment& rhs) {
        return lhs.start < rhs.start;
    });

    uint32_t cursor = static_cast<uint32_t>(stream.data.size());
    if (stream.size > 0 && cursor > stream.size) {
        cursor = stream.size;
    }

    for (const Fragment& gap : sortedGaps) {
        if (gap.end <= gap.start || gap.start >= stream.size) {
            continue;
        }

        const uint32_t start = gap.start;
        const uint32_t end = std::min(gap.end, stream.size);
        if (end > start) {
            result.append(QVariantMap{{QStringLiteral("start"), start}, {QStringLiteral("end"), end}});
        }
    }

    if (stream.size > cursor) {
        result.append(QVariantMap{{QStringLiteral("start"), cursor}, {QStringLiteral("end"), stream.size}});
    }

    return result;
}

QVariantList StreamList::batchRanges(const QVariantList& ranges, int maxCount) const
{
    QVariantList batch;
    for (int i = 0; i < ranges.size() && i < maxCount; ++i) {
        batch.append(ranges[i]);
    }
    return batch;
}

void StreamList::dispatchRequest(Stream* stream, const QVariantList& ranges, bool isRetry)
{
    if (!stream || ranges.isEmpty()) {
        return;
    }

    stream->uploadingState = isRetry ? UploadingRetrying : Uploading;
    if (isRetry) {
        ++stream->retryRound;
    }
    stream->requestInFlight = true;
    stream->lastRequestAt = timestamp();
    stream->lastRequestedMissingBytes = missingSize(*stream);
    stream->lastRequestedMissingRanges = missingRanges(*stream).size();
    stream->currentRequestRanges = ranges;
    if (stream->lastDiagnostic.problemId != RetrievalDiagnostic::ReadError) {
        stream->lastDiagnostic = {};
        stream->lastDiagnosticText.clear();
    }

    QStringList rangeTexts;
    rangeTexts.reserve(ranges.size());
    for (const QVariant& rangeVar : ranges) {
        const QVariantMap range = rangeVar.toMap();
        bool okStart = false;
        bool okEnd = false;
        const uint32_t start = range.value(QStringLiteral("start")).toUInt(&okStart);
        const uint32_t end = range.value(QStringLiteral("end")).toUInt(&okEnd);
        if (okStart && okEnd) {
            rangeTexts.append(QStringLiteral("%1-%2").arg(start).arg(end));
        }
    }

    core.consoleInfo(QString("Recorder request log=%1 ranges=%2 retryRound=%3 fragments=[%4]")
                         .arg(stream->id)
                         .arg(ranges.size())
                         .arg(stream->retryRound)
                         .arg(rangeTexts.join(QStringLiteral(", "))));
    emit requestRanges(stream->id, ranges);
    updateStream(stream->id);
}

void StreamList::handleRetrievalDiagnostic(const RetrievalDiagnostic& diagnostic)
{
    if (!diagnostic.isValid()) {
        return;
    }

    Stream* stream = getStream(diagnostic.logId);
    if (!stream) {
        return;
    }

    stream->lastDiagnostic = diagnostic;
    stream->lastDiagnosticText = diagnosticText(diagnostic);
    stream->requestInFlight = false;
    stream->currentRequestRanges.clear();

    core.consoleWarning(QString("Recorder retrieval diagnostic log=%1 problem=%2 fileState=%3 requested=[%4-%5] actual=%6 visible=%7")
                            .arg(diagnostic.logId)
                            .arg(stream->lastDiagnosticText)
                            .arg(diagnostic.fileState == RetrievalDiagnostic::Live ? QStringLiteral("Live")
                                                                                   : diagnostic.fileState == RetrievalDiagnostic::Archived ? QStringLiteral("Archived")
                                                                                                                                           : QStringLiteral("Unknown"))
                            .arg(diagnostic.requestedStart)
                            .arg(diagnostic.requestedEnd)
                            .arg(diagnostic.actualFileSize)
                            .arg(diagnostic.visibleFileSize));

    switch (diagnostic.problemId) {
    case RetrievalDiagnostic::TruncatedTail:
        applyTruncatedTail(stream, diagnostic);
        if (isDownloadComplete(*stream)) {
            completeDownload(stream, true);
        } else {
            const QVariantList nextMissingRanges = missingRanges(*stream);
            if (nextMissingRanges.isEmpty()) {
                completeDownload(stream, true);
            } else {
                // Reuse the normal retry throttle instead of dispatching immediately,
                // otherwise a terminal tail diagnostic can spin the same request loop
                // almost continuously.
                stream->requestInFlight = true;
                stream->lastRequestAt = timestamp();
                updateStream(stream->id);
            }
        }
        break;
    case RetrievalDiagnostic::ReadError:
        stream->requestInFlight = true;
        stream->lastRequestAt = timestamp();
        updateStream(stream->id);
        break;
    case RetrievalDiagnostic::WrongStart:
    case RetrievalDiagnostic::WrongEnd:
    case RetrievalDiagnostic::InvalidRange:
    default:
        core.consoleWarning(QString("Recorder recovery stopped for log=%1 because request generation must change")
                                .arg(stream->id));
        completeDownload(stream, false);
        break;
    }
}

void StreamList::applyTruncatedTail(Stream* stream, const RetrievalDiagnostic& diagnostic)
{
    if (!stream) {
        return;
    }

    uint32_t newSize = stream->size;
    if (newSize == 0) {
        newSize = diagnostic.actualFileSize;
    }
    if (diagnostic.visibleFileSize > 0) {
        newSize = std::min(newSize, diagnostic.visibleFileSize);
    }
    if (diagnostic.actualFileSize > 0) {
        newSize = std::min(newSize, diagnostic.actualFileSize);
    }
    // The most common truncated-tail case is an implicit tail gap:
    // data.size() marks the last complete locally assembled byte boundary,
    // while missingRanges() synthesizes [data.size(), stream.size) as the tail.
    // Clamp to that boundary so the same EOF tail is not requested again.
    newSize = std::min(newSize, static_cast<uint32_t>(stream->data.size()));

    // TruncatedTail means the visible EOF lands inside a frame, so anything after
    // the last locally complete boundary must be dropped from this session.
    bool trimmedTrailingGap = true;
    while (trimmedTrailingGap) {
        trimmedTrailingGap = false;
        for (const Fragment& gap : stream->gaps) {
            if (gap.end > gap.start && gap.start < newSize && gap.end >= newSize) {
                newSize = gap.start;
                trimmedTrailingGap = true;
                break;
            }
        }
    }

    stream->size = newSize;
    if (static_cast<uint32_t>(stream->data.size()) > stream->size) {
        stream->data.resize(static_cast<int>(stream->size));
    }

    QList<Fragment> trimmedGaps;
    trimmedGaps.reserve(stream->gaps.size());
    for (const Fragment& gap : stream->gaps) {
        if (gap.start >= stream->size) {
            continue;
        }

        Fragment trimmed = gap;
        trimmed.end = std::min(trimmed.end, stream->size);
        if (trimmed.end > trimmed.start) {
            trimmedGaps.append(trimmed);
        }
    }
    stream->gaps = trimmedGaps;
}

QString StreamList::diagnosticText(const RetrievalDiagnostic& diagnostic) const
{
    switch (diagnostic.problemId) {
    case RetrievalDiagnostic::WrongStart:
        return QStringLiteral("Wrong start");
    case RetrievalDiagnostic::WrongEnd:
        return QStringLiteral("Wrong end");
    case RetrievalDiagnostic::TruncatedTail:
        return QStringLiteral("Truncated tail");
    case RetrievalDiagnostic::ReadError:
        return QStringLiteral("Read error");
    case RetrievalDiagnostic::InvalidRange:
        return QStringLiteral("Invalid range");
    case RetrievalDiagnostic::ProblemNone:
    default:
        return QStringLiteral("Unknown problem");
    }
}

void StreamList::emitStateChanged()
{
    emit stateChanged();
}

void StreamList::process()
{
    if (_isInserting || _activeDownloadId < 0) {
        return;
    }

    Stream* stream = getStream(_activeDownloadId);
    if (!stream || !stream->requestInFlight) {
        return;
    }

    const uint64_t now = timestamp();
    const uint64_t lastActivityAt = std::max(stream->lastRequestAt, stream->lastDataAt);
    if (lastActivityAt == 0 || now - lastActivityAt < kRequestIdleMs) {
        return;
    }

    stream->requestInFlight = false;
    stream->currentRequestRanges.clear();

    if (isDownloadComplete(*stream)) {
        completeDownload(stream, true);
        return;
    }

    const uint32_t currentMissingBytes = missingSize(*stream);
    const int currentMissingRanges = missingRanges(*stream).size();
    const bool improved = currentMissingBytes < stream->lastRequestedMissingBytes
                       || currentMissingRanges < stream->lastRequestedMissingRanges;

    if (improved) {
        stream->noProgressRetryRounds = 0;
    } else {
        ++stream->noProgressRetryRounds;
    }

    if (stream->noProgressRetryRounds > kMaxNoProgressRetryRounds) {
        core.consoleWarning(QString("Recorder recovery stopped for log=%1 after %2 retry rounds")
                                .arg(stream->id)
                                .arg(stream->retryRound));
        completeDownload(stream, false);
        return;
    }

    const QVariantList nextMissingRanges = missingRanges(*stream);
    if (nextMissingRanges.isEmpty()) {
        completeDownload(stream, true);
        return;
    }

    dispatchRequest(stream, batchRanges(nextMissingRanges, kMaxRangesPerRequest), true);
}

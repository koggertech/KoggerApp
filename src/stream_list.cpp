#include "stream_list.h"

#include <core.h>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QRandomGenerator>
extern Core core;

StreamList::StreamList(QObject* parent) : QObject(parent)
{
//    createStream(0);
//    _lastStream = getStream(0);
//    _lastStreamId = 0;
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

void StreamList::debugAddGap(uint32_t start, uint32_t size) {
    Q_UNUSED(start);
    Q_UNUSED(size);
//    core.consoleInfo(QString("Find a gap %1 from %2").arg(size).arg(start));
}


void StreamList::debugSearchGap(uint32_t start, uint32_t size) {
    Q_UNUSED(start);
    Q_UNUSED(size);
//    core.consoleInfo(QString("Search a gap %1 from %2").arg(size).arg(start));
}


void StreamList::startDownload(int id) {
    // Single active download at a time: reset any previous one so its row doesn't
    // strand at "Downloading…" while process() drives the new id.
    if(_activeDownloadId >= 0 && _activeDownloadId != id) {
        cancelDownload(_activeDownloadId);
    }
    if(!isStreamExist(id)) {
        updateStream(id);               // create a model entry for an as-yet-unlisted log
    }
    Stream* s = getStream(id);
    s->recvFrames.clear();
    s->frontier = 0;
    s->actualFileSize = 0;
    s->eof = false;
    s->requestInFlight = false;
    s->lastReqAt = 0;
    s->noProgressRounds = 0;
    s->roundDone = false;
    s->expectedDiags = 0;
    s->diagsThisRound = 0;
    s->lastRecvCount = -1;
    s->savedFilePath.clear();
    s->uploadingState = Uploading;
    _activeDownloadId = id;
    updateStream(id);
    core.consoleInfo(QString("Recorder download start: log=%1").arg(id));
    qInfo("Recorder download start: log=%d", id);
}

void StreamList::cancelDownload(int id) {
    if(!isStreamExist(id)) { return; }
    Stream* s = getStream(id);
    if(_activeDownloadId == id) {
        // Preempt the device, or it keeps streaming the current request (the bulk phase
        // asks for the whole remainder). Sending a fresh request resets the recorder's
        // read ring buffer and supersedes the in-flight transfer; a single (0,0) range is
        // treated as an empty request (unused-slot skip), so the device just goes idle.
        QVector<quint32> stop;
        stop.push_back(0);
        stop.push_back(0);
        emit requestRanges(id, stop);
        _activeDownloadId = -1;
    }
    // Discard partial coverage — a re-Download starts fresh. Reset to a clean idle state
    // so the row shows "Download" again (not "Saved") with no progress bar.
    s->recvFrames.clear();
    s->frontier = 0;
    s->actualFileSize = 0;
    s->eof = false;
    s->requestInFlight = false;
    s->roundDone = false;
    s->expectedDiags = 0;
    s->diagsThisRound = 0;
    s->lastRecvCount = -1;
    s->noProgressRounds = 0;
    s->uploadingState = UploadingIdle;
    updateStream(id);
    core.consoleInfo(QString("Recorder download cancelled: log=%1").arg(id));
    qInfo("Recorder download cancelled: log=%d", id);
}

void StreamList::downloadFrame(uint16_t id, uint32_t offset, uint8_t* bytes, uint16_t len) {
    if(static_cast<int>(id) != _activeDownloadId || !isStreamExist(id)) {
        return;
    }
    // Loss-recovery test hook: env-gated random frame drop (like kptools --drop-prob).
    // A dropped frame is never inserted, so the frontier stalls and the range is re-requested.
    static const double kDropProb = qEnvironmentVariable("KOGGER_DROP_PROB").toDouble();
    if(kDropProb > 0.0 && QRandomGenerator::global()->generateDouble() < kDropProb) {
        return;
    }
    Stream* s = getStream(id);
    // Store the whole KP2 frame keyed by its self-reported offset; advance the
    // frontier over the contiguous run. Re-serve of an already-held offset is ignored.
    if(offset >= s->frontier && !s->recvFrames.contains(offset)) {
        s->recvFrames.insert(offset, QByteArray(reinterpret_cast<char*>(bytes), len));
    }
    while(s->recvFrames.contains(s->frontier)) {
        s->frontier += static_cast<uint32_t>(s->recvFrames[s->frontier].size());
    }
    if(s->size < s->frontier) { s->size = s->frontier; }
}

void StreamList::handleDiagnostic(FrameParser* frame) {
    const uint16_t plen = frame->payloadLen();
    if(plen < 22) { return; }

    const uint16_t logId     = frame->read<U2>();
    const uint16_t problem   = frame->read<U2>();
    const uint16_t fileState = frame->read<U2>();
    Q_UNUSED(fileState);

    // Tail-anchor the 4x U4 block: a reserved U2 may or may not sit after the head
    // (24-byte spec vs a firmware build that dropped it = 22 bytes). Mirrors kptools.
    frame->readSkip(static_cast<uint16_t>(plen - 6 - 16));
    const uint32_t reqStart = frame->read<U4>(); Q_UNUSED(reqStart);
    const uint32_t reqEnd   = frame->read<U4>(); Q_UNUSED(reqEnd);
    const uint32_t actual   = frame->read<U4>();
    const uint32_t visible  = frame->read<U4>(); Q_UNUSED(visible);

    if(!isStreamExist(logId)) { return; }
    Stream* s = getStream(logId);
    if(actual > 0) { s->actualFileSize = actual; }

    // problem_id: 1 WrongStart, 2 WrongEnd, 3 TruncatedTail, 4 ReadError, 5 InvalidRange.
    // TruncatedTail/InvalidRange are terminal (real EOF / bad); WrongEnd is normal
    // mid-file; WrongStart/ReadError are handled by the frontier + stall logic.
    if(problem == 3 || problem == 5) {
        s->eof = true;
    }
    if(static_cast<int>(logId) == _activeDownloadId) {
        // One diagnostic terminates each requested range. A multi-range request yields
        // one diagnostic per range, so only pace the next request once ALL have arrived
        // (the idle timeout in process() covers a diagnostic lost on a real link).
        if(++s->diagsThisRound >= s->expectedDiags) {
            s->roundDone = true;
        }
    }
}

void StreamList::completeDownload(Stream* stream, bool ok) {
    stream->requestInFlight = false;
    if(ok) {
        saveStream(stream);
        stream->uploadingState = UploadingIdle;
    } else {
        stream->uploadingState = UploadingError;
        core.consoleInfo(QString("Recorder download incomplete: log=%1 got=%2 of %3")
                         .arg(stream->id).arg(stream->frontier).arg(stream->actualFileSize));
        qInfo("Recorder download incomplete: log=%d got=%u", stream->id, stream->frontier);
    }
    _activeDownloadId = -1;
    updateStream(stream->id);
}

void StreamList::saveStream(Stream* stream) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp/recorder";
    if(!QDir().mkpath(dir)) {
        core.consoleInfo("Recorder download: cannot create " + dir);
        return;
    }
    const QString path = dir + QString("/recorder_log_%1.kp2").arg(stream->id);
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly)) {
        core.consoleInfo("Recorder download: cannot write " + path);
        return;
    }
    for(auto it = stream->recvFrames.constBegin(); it != stream->recvFrames.constEnd(); ++it) {
        if(it.key() >= stream->frontier) { break; }   // only the contiguous [0, frontier) prefix
        f.write(it.value());
    }
    f.close();
    stream->savedFilePath = path;
    core.consoleInfo(QString("Recorder download saved: %1 (%2 bytes)").arg(path).arg(stream->frontier));
    qInfo("Recorder download complete: log=%d bytes=%u file=%s",
          stream->id, stream->frontier, qUtf8Printable(path));
}

QVector<quint32> StreamList::computeGaps(Stream* s) const {
    QVector<quint32> gaps;   // flat [start,end, start,end, ...], tail range last
    uint32_t cur = 0;
    for(auto it = s->recvFrames.constBegin(); it != s->recvFrames.constEnd(); ++it) {
        const uint32_t off = it.key();
        const uint32_t len = static_cast<uint32_t>(it.value().size());
        if(off < cur) { continue; }              // duplicate/overlap
        if(off > cur) {
            // Internal gap [cur, off). End at off+len so the device serves only the
            // missing frames and withholds the already-held frame at `off` (a frame is
            // served iff its end < the floored requested_end).
            gaps.push_back(cur);
            gaps.push_back(off + len);
        }
        cur = off + len;
    }
    // Tail: request the whole remainder when EOF is unknown or the file extends past the
    // last received frame. A sub-frame trailing tail (device file size sector-rounded a
    // few bytes past the last frame) never yields a frame and is closed by completion.
    if(s->actualFileSize == 0 || cur < s->actualFileSize) {
        gaps.push_back(cur);
        gaps.push_back(0x0FFFFFFFu);
    }
    return gaps;
}

void StreamList::process() {
    if(_activeDownloadId < 0 || !isStreamExist(_activeDownloadId)) { return; }
    Stream* s = getStream(_activeDownloadId);
    if(s->uploadingState != Uploading) { return; }

    const uint64_t now = timestamp();
    updateStream(_activeDownloadId);              // ~10 Hz progress refresh

    const bool idle = (now - s->lastReqAt) >= kRequestIdleMs;
    // Wait for all requested ranges' terminating diagnostics before re-requesting; the
    // idle timeout is the fallback when a real link drops the frames + the diagnostic.
    if(s->requestInFlight && !s->roundDone && !idle) { return; }

    const uint32_t maxEnd = s->recvFrames.isEmpty()
        ? 0u
        : (s->recvFrames.lastKey() + static_cast<uint32_t>(s->recvFrames[s->recvFrames.lastKey()].size()));
    const bool contiguous = (s->frontier == maxEnd);          // no internal holes
    const int cnt = s->recvFrames.size();
    const bool progressed = (!s->requestInFlight) || (cnt > s->lastRecvCount);

    // Completion: EOF confirmed, all received data contiguous, and the last round (a
    // tail probe) brought nothing new. Fast path: reached the reported file size.
    if(s->eof && contiguous && s->requestInFlight && !progressed) { completeDownload(s, true); return; }
    if(s->actualFileSize && s->frontier >= s->actualFileSize && contiguous) { completeDownload(s, true); return; }

    if(s->requestInFlight && !progressed) {
        if(++s->noProgressRounds >= kMaxNoProgressRounds) { completeDownload(s, s->eof && contiguous); return; }
    } else if(progressed) {
        s->noProgressRounds = 0;
    }

    // Batched gap-fill (requires the multi-range firmware fix): the first round with an
    // empty buffer requests one whole-remainder range (bulk); later rounds request the
    // exact missing ranges, up to 16 per request, each with a re-serve-nothing end. This
    // fills scattered losses in far fewer round-trips than re-streaming the whole tail —
    // the high-latency win. See kpclient::download_stream_batched.
    QVector<quint32> gaps = computeGaps(s);
    if(gaps.isEmpty()) { completeDownload(s, contiguous); return; }
    if(gaps.size() > 2 * kMaxRangesPerRequest) {
        gaps.resize(2 * kMaxRangesPerRequest);    // remaining gaps fill on later rounds
    }

    s->lastRecvCount = cnt;
    s->diagsThisRound = 0;
    s->expectedDiags = gaps.size() / 2;
    s->roundDone = false;
    s->lastReqAt = now;
    s->requestInFlight = true;
    emit requestRanges(_activeDownloadId, gaps);
}

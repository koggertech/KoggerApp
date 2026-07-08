#ifndef STREAMLIST_H
#define STREAMLIST_H

#include "stdint.h"
#include "QByteArray"
#include "QHash"
#include "QMap"
#include "proto_binnary.h"
#include "stream_list_model.h"
#include "QTime"
#include "QTimer"
#include <QString>
#include <QVector>


using namespace Parsers;

class StreamList : public QObject
{
    Q_OBJECT
public:
    explicit StreamList(QObject* parent = nullptr);
    ~StreamList() override;

    void initTimer();

    typedef enum : uint8_t {
        RecordingError,
        RecordingIdle,
        RecordingPause,
        Recording
    } RecordingState;

    typedef enum : uint8_t {
        UploadingError,
        UploadingIdle,
        UploadingPause,
        Uploading
    } UploadingState;

    typedef enum : uint8_t {
        FragmentNone,
        FragmentNew,
        FragmentWait,
        FragmentProcessing
    } FragmentStatus;

    typedef struct {
        uint32_t start, end;
        uint64_t timestamp;
        FragmentStatus status;
    } Fragment;

    struct Stream
    {
        uint16_t id = 0;
        RecordingState recordingState = RecordingError;
        UploadingState uploadingState = UploadingError;
        uint32_t size = 0;
        uint32_t unixt = 0;
        QByteArray data;
        QList<Fragment> gaps;
        struct {
            uint32_t _fragments;
            uint32_t _lostFragments = 0;
            uint32_t _fillFragments = 0;
        } _counter;
        int modelIndex = -1;

        // download: contiguous-frontier reassembly (see Recorder-Host-Integration-Guide.md)
        QMap<uint32_t, QByteArray> recvFrames; // stored KP2 frames keyed by stream offset
        uint32_t frontier = 0;                 // next byte not yet received (a frame boundary)
        uint32_t actualFileSize = 0;           // completion target from the retrieval diagnostic
        bool     eof = false;
        bool     requestInFlight = false;
        uint64_t lastReqAt = 0;
        int      noProgressRounds = 0;
        bool     roundDone = false;            // set when ALL requested ranges have terminated
        int      expectedDiags = 0;            // diagnostics expected this round (= ranges requested)
        int      diagsThisRound = 0;           // diagnostics received since the last request
        int      lastRecvCount = -1;           // recvFrames.size() at last request (progress metric)
        QString  savedFilePath;
    };

    void append(FrameParser* frame) {
        if(frame->isStream()) {
            downloadFrame(frame->streamId(), frame->streamOffset(),
                          frame->frame(), frame->frameLen());
        }
    }

    void parse(FrameParser* frame) {
        if(frame->id() != ID_STREAM || frame->type() != CONTENT) { return; }

        if(frame->ver() == v0 && !frame->resp()) {
            int item_cnt = frame->payloadLen()/12;
            while(item_cnt--) {
                int id = frame->read<U2>();
                uint16_t flags = frame->read<U2>();
                uint32_t size = frame->read<U4>();
                uint32_t unixt = frame->read<U4>();

                updateStream(id);
                Stream* stream = getStream(id);

                if(stream->size < size) { stream->size = size; }
                stream->unixt = unixt;
                stream->recordingState = (RecordingState)(flags & 0x3);

                updateStream(id);
                _isListChenged = true;
            }
        } else if(frame->ver() == v1) {
            handleDiagnostic(frame);
        }
    }

    void updateStream(int id) {
        _streams[id].id = id;
        Q_EMIT _modelList.appendEvent(_streams[id].id, _streams[id].size, _streams[id].frontier, "", _streams[id].recordingState, _streams[id].uploadingState);
    }

    Stream* getStream(int id) {
        if(isStreamExist(id)) {
            return &_streams[id];
        } else {
            return nullptr;
        }
    }

    bool isStreamExist(int id) {
        return _streams.contains(id);
    }

    StreamListModel* streamsList() {
        return &_modelList;
    }

    bool isListChenged() {
        if(_isListChenged) {
            _isListChenged = false;
            return true;
        }
        return false;
    }

    void startDownload(int id);
    void cancelDownload(int id);

signals:
    void requestRange(int id, quint32 start, quint32 end);
    void requestRanges(int id, QVector<quint32> ranges);   // flat [s0,e0,s1,e1,...], <=16 pairs

protected:
    QMap<int, Stream> _streams;
    uint16_t _lastStreamId = 0xFFFF;
    Stream* _lastStream;
    bool _isListChenged = false;
    StreamListModel _modelList;
    QTimer* _updater = nullptr;
    uint64_t _timeLastGapsUpdate = 0;
    uint64_t _timeLastGapsInsert = 0;
    bool _isInserting = false;
    int _activeDownloadId = -1;



    void insert(Stream* stream, uint8_t* frame, uint32_t offset, uint16_t size) {
        uint32_t end = offset + size;
        QList<Fragment>& gaps = stream->gaps;
        QByteArray& data = stream->data;

        _timeLastGapsInsert = timestamp();
        _isInserting = true;

        if(stream->size < end) {
            stream->size = end;
        }

        if (static_cast<uint32_t>(data.size()) < offset) {
            Fragment new_fragment = {
                .start = (uint32_t)data.size(),
                .end = offset,
                .timestamp = timestamp(),
                .status = FragmentStatus::FragmentNew
            };
            gaps.append(new_fragment);
            stream->_counter._lostFragments++;
            debugAddGap(offset, offset - (uint32_t)data.size());
        }
        else if (static_cast<uint32_t>(data.size()) > offset) {
            debugSearchGap(offset, size);

            for(int32_t i = 0; i < gaps.size(); i++) {
                uint32_t g_start = gaps[i].start;
                uint32_t g_end = gaps[i].end;

                if(g_start <= offset && g_end > offset) {
                    if(g_start == offset) {
                        if(g_end > end) {
                            gaps[i].start = end + 1;
                        } else {
                            gaps.removeAt(i);
                            i-=1;
                        }
                    } else {
                        gaps[i].end = offset - 1;
                        if(g_end >= end) {
                            Fragment new_fragment = {
                                .start = end + 1,
                                .end = g_end,
                                .timestamp = timestamp(),
                                .status = FragmentStatus::FragmentNew
                            };

                            gaps.insert(i, new_fragment);
                            i+=1;
                        }
                    }

                    gaps[i].timestamp = timestamp();
                    gaps[i].status = FragmentStatus::FragmentProcessing;
                    break;
                }
            }

            stream->_counter._fillFragments++;
        } else {
            stream->_counter._fragments++;
        }

        data.replace(offset, size, (char*)frame, size);
        updateStream(stream->id);

        _isInserting = false;
//        process();
    }

    uint64_t timestamp() {
        return QDateTime::currentMSecsSinceEpoch();
    }

    void debugAddGap(uint32_t start, uint32_t size);
    void debugSearchGap(uint32_t start, uint32_t size);

    void downloadFrame(uint16_t id, uint32_t offset, uint8_t* bytes, uint16_t len);
    void handleDiagnostic(FrameParser* frame);
    void completeDownload(Stream* stream, bool ok);
    void saveStream(Stream* stream);
    QVector<quint32> computeGaps(Stream* stream) const;   // flat [s0,e0,...] missing ranges, tail last

    static constexpr uint64_t kRequestIdleMs = 1500;
    static constexpr int kMaxNoProgressRounds = 3;
    static constexpr int kMaxRangesPerRequest = 16;       // device FRAGMENTS_NBR

protected slots:
    void process();
};

#endif // STREAMLIST_H

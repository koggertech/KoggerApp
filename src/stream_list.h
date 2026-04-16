#ifndef STREAMLIST_H
#define STREAMLIST_H

#include "stdint.h"
#include "QByteArray"
#include "QHash"
#include "QMap"
#include "proto_binnary.h"
#include "stream_list_model.h"
#include "QDateTime"
#include "QTime"
#include "QTimer"
#include <QVariantList>
#include <QString>


using namespace Parsers;

class StreamList : public QObject
{
    Q_OBJECT
public:
    explicit StreamList(QObject* parent = nullptr);
    ~StreamList();

    void initTimer();

    typedef enum {
        RecordingError,
        RecordingIdle,
        RecordingPause,
        Recording
    } RecordingState;

    typedef enum {
        UploadingIdle,
        Uploading,
        UploadingRetrying,
        UploadingComplete,
        UploadingIncomplete
    } UploadingState;

    typedef enum {
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

    struct RetrievalDiagnostic
    {
        enum ProblemId : uint16_t {
            ProblemNone = 0,
            WrongStart = 1,
            WrongEnd = 2,
            TruncatedTail = 3,
            ReadError = 4,
            InvalidRange = 5
        };

        enum FileState : uint16_t {
            FileStateNone = 0,
            Archived = 1,
            Live = 2
        };

        uint16_t logId = 0;
        ProblemId problemId = ProblemNone;
        FileState fileState = FileStateNone;
        uint16_t reserved = 0;
        uint32_t requestedStart = 0;
        uint32_t requestedEnd = 0;
        uint32_t actualFileSize = 0;
        uint32_t visibleFileSize = 0;

        bool isValid() const { return problemId != ProblemNone; }
    };

    struct Stream
    {
        uint16_t id = 0;
        RecordingState recordingState = RecordingError;
        UploadingState uploadingState = UploadingIdle;
        uint32_t listedSize = 0;
        uint32_t size = 0;
        uint32_t unixt = 0;
        QByteArray data;
        QList<Fragment> gaps;
        QString savedFilePath;
        RetrievalDiagnostic lastDiagnostic;
        QString lastDiagnosticText;
        struct {
            uint32_t _fragments = 0;
            uint32_t _lostFragments = 0;
            uint32_t _fillFragments = 0;
        } _counter;
        int modelIndex = -1;
        uint64_t downloadStartedAt = 0;
        uint64_t lastDataAt = 0;
        uint64_t lastRequestAt = 0;
        uint32_t lastRequestedMissingBytes = 0;
        int lastRequestedMissingRanges = 0;
        int retryRound = 0;
        int noProgressRetryRounds = 0;
        bool requestInFlight = false;
        QVariantList currentRequestRanges;
    };

    void append(FrameParser* frame);

    void parse(FrameParser* frame);

    void updateStream(int id);
    void startDownload(int id);
    void handleRetrievalDiagnostic(const RetrievalDiagnostic& diagnostic);

    Stream* getStream(int id) {
        if(isStreamExist(id)) {
            return &_streams[id];
        } else {
            return NULL;
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

protected:
    QMap<int, Stream> _streams;
    int _activeDownloadId = -1;
    uint16_t _lastStreamId = 0xFFFF;
    Stream* _lastStream = nullptr;
    bool _isListChenged = false;
    StreamListModel _modelList;
    QTimer* _updater = nullptr;
    uint64_t _timeLastGapsUpdate = 0;
    uint64_t _timeLastGapsInsert = 0;
    bool _isInserting = false;

    void insert(Stream* stream, uint8_t* frame, uint32_t offset, uint16_t size);
    void trimGaps(Stream* stream, uint32_t offset, uint32_t end);
    uint32_t coveredSize(const Stream& stream) const;
    uint32_t missingSize(const Stream& stream) const;
    bool isDownloadComplete(const Stream& stream) const;
    QString statusText(const Stream& stream) const;
    QString resolveRecorderDirectoryPath() const;
    bool saveStreamToFile(Stream* stream);
    void completeDownload(Stream* stream, bool isComplete);
    QVariantList missingRanges(const Stream& stream) const;
    QVariantList batchRanges(const QVariantList& ranges, int maxCount) const;
    void dispatchRequest(Stream* stream, const QVariantList& ranges, bool isRetry);
    void applyTruncatedTail(Stream* stream, const RetrievalDiagnostic& diagnostic);
    QString diagnosticText(const RetrievalDiagnostic& diagnostic) const;
    void emitStateChanged();

    static constexpr int kMaxRangesPerRequest = 16;
    static constexpr uint64_t kRequestIdleMs = 1500;
    static constexpr int kMaxNoProgressRetryRounds = 3;

    uint64_t timestamp() {
        return QDateTime::currentMSecsSinceEpoch();
    }

    void debugAddGap(uint32_t start, uint32_t size);
    void debugSearchGap(uint32_t start, uint32_t size);

protected slots:
    void process();

signals:
    void requestRanges(int logId, const QVariantList& ranges);
    void stateChanged();
};

#endif // STREAMLIST_H

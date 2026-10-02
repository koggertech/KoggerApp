#pragma once

#include <cstdint>

#include <QByteArray>
#include <QMetaType>
#include <QObject>
#include <QTimer>
#include <QVector>

namespace autopilot {

struct MissionItem
{
    uint16_t command = 16;
    uint8_t frame = 0;
    uint8_t current = 0;
    uint8_t autocontinue = 1;
    float param1 = 0.0f;
    float param2 = 0.0f;
    float param3 = 0.0f;
    float param4 = 0.0f;
    int32_t x = 0;
    int32_t y = 0;
    float z = 0.0f;
};

using MissionItems = QVector<MissionItem>;

struct MissionBatch
{
    int missionType = 0;
    MissionItems items;
};

using MissionBatches = QVector<MissionBatch>;

/**
 * Client side of the MAVLink mission protocol, one transfer at a time, for several mission types
 * in a row (route, fence, rally).
 *
 * Upload, per batch: MISSION_COUNT, then one MISSION_ITEM_INT per MISSION_REQUEST_INT /
 * MISSION_REQUEST from the vehicle, until MISSION_ACK. An empty batch clears that list.
 * Download, per mission type: MISSION_REQUEST_LIST, the vehicle answers MISSION_COUNT, then
 * MISSION_REQUEST_INT for every seq and MISSION_ITEM_INT back, closed by our MISSION_ACK.
 *
 * Transport-free: incoming payloads arrive through handleFrame(), outgoing payloads leave
 * through sendMessage(). Runs in its own thread.
 */
class MissionTransfer : public QObject
{
    Q_OBJECT

public:
    /** Local results; positive values are MAV_MISSION_RESULT codes from the vehicle. */
    enum Result : int {
        ResultOk = 0,
        ResultNoLink = -1,
        ResultNoResponse = -2,
        ResultOutOfRange = -3,
        ResultLinkLost = -4,
        ResultBusy = -5,
        ResultIncomplete = -6,
        ResultMavlink1 = -7,
        ResultCancelled = -8
    };

    static constexpr int kAckTimeoutMs = 1500;
    static constexpr int kMaxRetries = 5;

    explicit MissionTransfer(QObject* parent = nullptr);

public slots:
    /** Uploads all batches in order. Emits uploadFinished() once, after the last batch or the first failure. */
    void startUpload(const autopilot::MissionBatches& batches, int targetSystem, int targetComponent);
    /** Reads the lists of @p missionTypes in order. Emits downloadFinished() once with what was read. */
    void startDownload(const QVector<int>& missionTypes, int targetSystem, int targetComponent);
    /** Feeds a payload of MISSION_REQUEST (40), MISSION_COUNT (44), MISSION_ACK (47), MISSION_REQUEST_INT (51) or MISSION_ITEM_INT (73). */
    void handleFrame(quint32 msgId, const QByteArray& payload);
    /** Ends a running transfer with @p result without talking to the vehicle. */
    void abort(int result);

signals:
    void sendMessage(quint32 msgId, const QByteArray& payload, int v1Length);
    void progressChanged(int done, int total);
    /** @p missionType is the batch that failed, or the last batch on success. */
    void uploadFinished(bool ok, int result, int missionType);
    /** @p batches holds every list read completely, also on failure; @p missionType is the failed or the last list. */
    void downloadFinished(bool ok, int result, int missionType, const autopilot::MissionBatches& batches);

private:
    enum class State { Idle, UploadCount, UploadItems, DownloadList, DownloadItems };

    bool busy() const { return state_ != State::Idle; }
    bool downloading() const { return state_ == State::DownloadList || state_ == State::DownloadItems; }
    bool addressedToUs(int targetSystem, int targetComponent) const;

    void handleUploadFrame(quint32 msgId, const QByteArray& payload);
    void handleDownloadFrame(quint32 msgId, const QByteArray& payload);

    void startUploadBatch();
    void uploadBatchDone();
    void sendCount();
    void sendItem(int seq);

    void startDownloadList();
    void downloadListDone();
    void sendRequestList();
    void sendRequest(int seq);
    void sendAck(int result);

    void onTimeout();
    void finish(bool ok, int result);
    void reportProgress();

    State state_ = State::Idle;
    bool download_ = false;
    MissionBatches batches_;
    QVector<int> downloadTypes_;
    int batchIndex_ = 0;
    int doneBefore_ = 0;
    int total_ = 0;
    MissionItems items_;
    int missionType_ = 0;
    int targetSystem_ = 0;
    int targetComponent_ = 0;
    int lastItem_ = -1;
    int requested_ = 0;
    int expectedCount_ = 0;
    int retries_ = 0;
    QTimer timer_{ this };
};

} // namespace autopilot

Q_DECLARE_METATYPE(autopilot::MissionItems)
Q_DECLARE_METATYPE(autopilot::MissionBatches)

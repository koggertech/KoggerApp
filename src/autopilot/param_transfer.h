#pragma once

#include <cstdint>

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

namespace autopilot {

/**
 * Client side of the MAVLink parameter protocol for one vehicle component.
 *
 * Download: PARAM_REQUEST_LIST, the vehicle streams PARAM_VALUE with param_index / param_count.
 * After a silence the indices still missing are read one by one (PARAM_REQUEST_READ by index),
 * a few at a time, each a limited number of times; what never arrives is reported as missing.
 * Write: one PARAM_SET at a time from a queue, each followed by a PARAM_REQUEST_READ of that name,
 * confirmed by a PARAM_VALUE with the same name and value (the echo of the save or the answer to
 * the read); a timeout sends both again, a limited number of times. Values travel as
 * the numeric float of the value (ArduPilot C-cast encoding), with the type of the last
 * PARAM_VALUE for that name.
 *
 * Every PARAM_VALUE, also unrequested ones, comes out as valueReceived(). Transport-free:
 * incoming payloads arrive through handleParamValue(), outgoing payloads leave through
 * sendMessage().
 */
class ParamTransfer : public QObject
{
    Q_OBJECT

public:
    enum Result : int8_t {
        ResultOk = 0,
        ResultNoResponse = -2,
        ResultLinkLost = -4,
        ResultCancelled = -8,
        ResultNotConfirmed = -9
    };

    static constexpr int kListTimeoutMs = 5000;
    static constexpr int kListRetries = 4;
    static constexpr int kSilenceMs = 3000;
    static constexpr int kIndexBatch = 10;
    static constexpr int kIndexRetries = 5;
    static constexpr int kWriteTimeoutMs = 1000;
    static constexpr int kWriteAttempts = 3;
    static constexpr int kReadTimeoutMs = 1500;
    static constexpr int kReadAttempts = 3;
    static constexpr uint16_t kUnrequestedIndex = 65535;

    explicit ParamTransfer(QObject* parent = nullptr);

    bool downloading() const { return state_ != State::Idle; }
    int pendingWrites() const { return int(queue_.size()) + (writing_ ? 1 : 0); }
    /** The equality a write confirmation uses: whole numbers for integer types, a small relative tolerance for REAL32. */
    static bool sameValue(float a, float b, uint8_t type);

public slots:
    /** Addresses every following request to @p targetSystem / @p targetComponent. */
    void setTarget(int targetSystem, int targetComponent);
    /** Reads the whole list; a download already running starts over. Emits downloadFinished() once. */
    void startDownload();
    /** Queues a write of @p value as MAV_PARAM_TYPE @p type; a queued write of the same name is replaced. Emits writeFinished(). */
    void write(const QString& name, float value, int type);
    /**
     * Reads @p names one by one (PARAM_REQUEST_READ by name), asking again for the ones still
     * unanswered every kReadTimeoutMs, kReadAttempts times in all. Names added while a read runs
     * join it and restart the attempts. Emits readFinished() once nothing is left.
     */
    void readNames(const QStringList& names);
    /** Feeds the payload of one PARAM_VALUE (22) of the target component. */
    void handleParamValue(const QByteArray& payload);
    /** Ends the download and every write with @p result without talking to the vehicle. */
    void abort(int result);

signals:
    void sendMessage(quint32 msgId, const QByteArray& payload, int v1Length);
    void valueReceived(const QString& name, float value, int type, int index, int count);
    void progressChanged(int received, int total);
    /** @p missing counts the indices that never arrived; ok is false only when the list was never answered or the download was aborted. */
    void downloadFinished(bool ok, int result, int missing);
    void writeFinished(const QString& name, bool ok, int result);
    /** @p missing are the names that never answered (unknown to the vehicle, or the link failed). */
    void readFinished(const QStringList& missing);

private:
    enum class State : uint8_t { Idle, WaitList, Streaming };

    struct Write
    {
        QString name;
        float value = 0.0f;
        uint8_t type = 0;
    };

    void sendRequestList();
    void sendReadIndex(int index);
    void sendReadName(const QString& name);
    void sendSet(const Write& w);

    void onListTimeout();
    void onWriteTimeout();
    void onReadTimeout();
    void finishRead();
    void fillIndexBatch();
    void finishDownload(bool ok, int result);
    void reportProgress();

    void startNextWrite();
    void finishWrite(bool ok, int result);

    State state_ = State::Idle;
    int targetSystem_ = 0;
    int targetComponent_ = 0;
    int count_ = 0;
    int listRetries_ = 0;
    bool batchActive_ = false;
    QSet<int> waiting_;
    QSet<int> failed_;
    QVector<int> inFlight_;
    QHash<int, int> indexTries_;
    QTimer listTimer_{ this };

    QVector<Write> queue_;
    Write current_;
    bool writing_ = false;
    int writeAttempts_ = 0;
    QTimer writeTimer_{ this };

    QStringList readPending_;
    int readAttempts_ = 0;
    QTimer readTimer_{ this };
};

} // namespace autopilot

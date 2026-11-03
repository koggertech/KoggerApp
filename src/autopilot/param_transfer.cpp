#include "param_transfer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "autopilot_messages.h"
#include "frame_codec.h"

namespace autopilot {

namespace {

constexpr float kFloatRelativeTolerance = 1e-5f;
constexpr float kFloatAbsoluteTolerance = 1e-6f;

void copyParamId(char* dst, const QString& name)
{
    const QByteArray id = name.toLatin1().left(kMavParamIdLength);
    std::memcpy(dst, id.constData(), size_t(id.size()));
}

} // namespace

ParamTransfer::ParamTransfer(QObject* parent)
    : QObject(parent)
{
    listTimer_.setSingleShot(true);
    connect(&listTimer_, &QTimer::timeout, this, &ParamTransfer::onListTimeout);
    writeTimer_.setSingleShot(true);
    writeTimer_.setInterval(kWriteTimeoutMs);
    connect(&writeTimer_, &QTimer::timeout, this, &ParamTransfer::onWriteTimeout);
}

void ParamTransfer::setTarget(int targetSystem, int targetComponent)
{
    targetSystem_ = targetSystem;
    targetComponent_ = targetComponent;
}

void ParamTransfer::startDownload()
{
    state_ = State::WaitList;
    count_ = 0;
    listRetries_ = 0;
    batchActive_ = false;
    waiting_.clear();
    failed_.clear();
    inFlight_.clear();
    indexTries_.clear();
    emit progressChanged(0, 0);
    sendRequestList();
    listTimer_.start(kListTimeoutMs);
}

void ParamTransfer::write(const QString& name, float value, int type)
{
    const Write w{ name, value, uint8_t(type) };
    for (auto& queued : queue_) {
        if (queued.name == name) {
            queued = w;
            return;
        }
    }
    queue_.append(w);
    if (!writing_) {
        startNextWrite();
    }
}

void ParamTransfer::handleParamValue(const QByteArray& payload)
{
    const auto value = decodePayload<MAVLink_MSG_PARAM_VALUE>(payload);
    const QString name = QString::fromLatin1(value.param_id, int(qstrnlen(value.param_id, kMavParamIdLength)));
    if (name.isEmpty()) {
        return;
    }
    emit valueReceived(name, value.param_value, value.param_type, value.param_index, value.param_count);

    if (writing_ && name == current_.name && sameValue(value.param_value, current_.value, current_.type)) {
        finishWrite(true, ResultOk);
    }

    if (state_ == State::Idle || value.param_index == kUnrequestedIndex) {
        return;
    }
    if (state_ == State::WaitList) {
        if (value.param_count == 0) {
            finishDownload(true, ResultOk);
            return;
        }
        state_ = State::Streaming;
        count_ = value.param_count;
        waiting_.reserve(count_);
        for (int i = 0; i < count_; ++i) {
            waiting_.insert(i);
        }
    }
    const int index = value.param_index;
    if (waiting_.remove(index)) {
        inFlight_.removeOne(index);
        if (batchActive_) {
            fillIndexBatch();
        }
    }
    reportProgress();
    if (waiting_.isEmpty()) {
        finishDownload(true, ResultOk);
        return;
    }
    listTimer_.start(kSilenceMs);
}

void ParamTransfer::abort(int result)
{
    if (downloading()) {
        finishDownload(false, result);
    }
    const QVector<Write> dropped = queue_;
    queue_.clear();
    if (writing_) {
        finishWrite(false, result);
    }
    for (const auto& w : dropped) {
        emit writeFinished(w.name, false, result);
    }
}

void ParamTransfer::sendRequestList()
{
    MAVLink_MSG_PARAM_REQUEST_LIST list;
    list.target_system = uint8_t(targetSystem_);
    list.target_component = uint8_t(targetComponent_);
    emit sendMessage(MAVLink_MSG_PARAM_REQUEST_LIST::getID(), payloadOf(list), MAVLink_MSG_PARAM_REQUEST_LIST::v1Length());
}

void ParamTransfer::sendReadIndex(int index)
{
    MAVLink_MSG_PARAM_REQUEST_READ read;
    read.param_index = int16_t(index);
    read.target_system = uint8_t(targetSystem_);
    read.target_component = uint8_t(targetComponent_);
    emit sendMessage(MAVLink_MSG_PARAM_REQUEST_READ::getID(), payloadOf(read), MAVLink_MSG_PARAM_REQUEST_READ::v1Length());
}

void ParamTransfer::sendReadName(const QString& name)
{
    MAVLink_MSG_PARAM_REQUEST_READ read;
    read.param_index = -1;
    read.target_system = uint8_t(targetSystem_);
    read.target_component = uint8_t(targetComponent_);
    copyParamId(read.param_id, name);
    emit sendMessage(MAVLink_MSG_PARAM_REQUEST_READ::getID(), payloadOf(read), MAVLink_MSG_PARAM_REQUEST_READ::v1Length());
}

void ParamTransfer::sendSet(const Write& w)
{
    MAVLink_MSG_PARAM_SET set;
    set.param_value = w.value;
    set.target_system = uint8_t(targetSystem_);
    set.target_component = uint8_t(targetComponent_);
    copyParamId(set.param_id, w.name);
    set.param_type = w.type;
    emit sendMessage(MAVLink_MSG_PARAM_SET::getID(), payloadOf(set), MAVLink_MSG_PARAM_SET::v1Length());
}

void ParamTransfer::onListTimeout()
{
    if (state_ == State::WaitList) {
        if (++listRetries_ > kListRetries) {
            finishDownload(false, ResultNoResponse);
            return;
        }
        sendRequestList();
        listTimer_.start(kListTimeoutMs);
        return;
    }
    if (state_ != State::Streaming) {
        return;
    }
    batchActive_ = true;
    inFlight_.clear();
    fillIndexBatch();
    reportProgress();
    if (waiting_.isEmpty()) {
        finishDownload(true, ResultOk);
        return;
    }
    listTimer_.start(kSilenceMs);
}

void ParamTransfer::fillIndexBatch()
{
    QVector<int> pending(waiting_.cbegin(), waiting_.cend());
    std::sort(pending.begin(), pending.end());
    for (const int index : std::as_const(pending)) {
        if (inFlight_.size() >= kIndexBatch) {
            break;
        }
        if (inFlight_.contains(index)) {
            continue;
        }
        if (++indexTries_[index] > kIndexRetries) {
            waiting_.remove(index);
            failed_.insert(index);
            continue;
        }
        inFlight_.append(index);
        sendReadIndex(index);
    }
}

void ParamTransfer::finishDownload(bool ok, int result)
{
    listTimer_.stop();
    state_ = State::Idle;
    const int missing = int(failed_.size() + waiting_.size());
    waiting_.clear();
    inFlight_.clear();
    indexTries_.clear();
    emit downloadFinished(ok, result, ok ? missing : 0);
}

void ParamTransfer::reportProgress()
{
    emit progressChanged(count_ - int(waiting_.size()), count_);
}

void ParamTransfer::startNextWrite()
{
    if (queue_.isEmpty()) {
        writing_ = false;
        return;
    }
    current_ = queue_.takeFirst();
    writing_ = true;
    writeAttempts_ = 1;
    sendSet(current_);
    sendReadName(current_.name);
    writeTimer_.start();
}

void ParamTransfer::onWriteTimeout()
{
    if (!writing_) {
        return;
    }
    if (writeAttempts_ >= kWriteAttempts) {
        finishWrite(false, ResultNotConfirmed);
        return;
    }
    ++writeAttempts_;
    sendSet(current_);
    sendReadName(current_.name);
    writeTimer_.start();
}

void ParamTransfer::finishWrite(bool ok, int result)
{
    writeTimer_.stop();
    const QString name = current_.name;
    writing_ = false;
    emit writeFinished(name, ok, result);
    startNextWrite();
}

bool ParamTransfer::sameValue(float a, float b, uint8_t type)
{
    if (type != MavParamTypeReal32) {
        return std::lround(a) == std::lround(b);
    }
    const float scale = std::max(std::fabs(a), std::fabs(b));
    return std::fabs(a - b) <= std::max(kFloatAbsoluteTolerance, kFloatRelativeTolerance * scale);
}

} // namespace autopilot

#include "mission_transfer.h"

#include <algorithm>

#include "autopilot_messages.h"
#include "frame_codec.h"

namespace autopilot {

namespace {

constexpr int kProgressScale = 1000;

} // namespace

MissionTransfer::MissionTransfer(QObject* parent)
    : QObject(parent)
{
    timer_.setSingleShot(true);
    timer_.setInterval(kAckTimeoutMs);
    connect(&timer_, &QTimer::timeout, this, &MissionTransfer::onTimeout);
}

void MissionTransfer::startUpload(const MissionBatches& batches, int targetSystem, int targetComponent)
{
    if (busy()) {
        emit uploadFinished(false, ResultBusy, batches.isEmpty() ? 0 : batches.first().missionType);
        return;
    }
    if (batches.isEmpty()) {
        emit uploadFinished(true, ResultOk, 0);
        return;
    }
    download_ = false;
    batches_ = batches;
    targetSystem_ = targetSystem;
    targetComponent_ = targetComponent;
    batchIndex_ = 0;
    doneBefore_ = 0;
    total_ = 0;
    for (const auto& b : std::as_const(batches_)) {
        total_ += int(b.items.size());
    }
    startUploadBatch();
}

void MissionTransfer::startDownload(const QVector<int>& missionTypes, int targetSystem, int targetComponent)
{
    if (busy()) {
        emit downloadFinished(false, ResultBusy, missionTypes.isEmpty() ? 0 : missionTypes.first(), {});
        return;
    }
    if (missionTypes.isEmpty()) {
        emit downloadFinished(true, ResultOk, 0, {});
        return;
    }
    download_ = true;
    downloadTypes_ = missionTypes;
    batches_.clear();
    targetSystem_ = targetSystem;
    targetComponent_ = targetComponent;
    batchIndex_ = 0;
    startDownloadList();
}

void MissionTransfer::handleFrame(quint32 msgId, const QByteArray& payload)
{
    if (!busy()) {
        return;
    }
    if (downloading()) {
        handleDownloadFrame(msgId, payload);
    } else {
        handleUploadFrame(msgId, payload);
    }
}

void MissionTransfer::abort(int result)
{
    if (busy()) {
        finish(false, result);
    }
}

bool MissionTransfer::addressedToUs(int targetSystem, int targetComponent) const
{
    return (targetSystem == 0 || targetSystem == kMavGcsSystemId)
        && (targetComponent == 0 || targetComponent == kMavGcsComponentId);
}

void MissionTransfer::handleUploadFrame(quint32 msgId, const QByteArray& payload)
{
    if (msgId == MAVLink_MSG_MISSION_REQUEST_INT::getID() || msgId == MAVLink_MSG_MISSION_REQUEST::getID()) {
        const auto request = decodePayload<MAVLink_MSG_MISSION_REQUEST_INT>(payload);
        if (!addressedToUs(request.target_system, request.target_component) || request.mission_type != missionType_) {
            return;
        }
        if (request.seq >= items_.size()) {
            finish(false, ResultOutOfRange);
            return;
        }
        state_ = State::UploadItems;
        retries_ = 0;
        sendItem(request.seq);
        requested_ = std::max(requested_, int(request.seq) + 1);
        reportProgress();
        timer_.start();
        return;
    }

    if (msgId == MAVLink_MSG_MISSION_ACK::getID()) {
        const auto ack = decodePayload<MAVLink_MSG_MISSION_ACK>(payload);
        if (!addressedToUs(ack.target_system, ack.target_component) || ack.mission_type != missionType_) {
            return;
        }
        if (ack.type == MavMissionInvalidSequence && state_ == State::UploadItems) {
            return;
        }
        if (ack.type != MavMissionAccepted) {
            finish(false, ack.type);
            return;
        }
        if (requested_ < items_.size()) {
            finish(false, ResultIncomplete);
            return;
        }
        uploadBatchDone();
    }
}

void MissionTransfer::handleDownloadFrame(quint32 msgId, const QByteArray& payload)
{
    if (msgId == MAVLink_MSG_MISSION_COUNT::getID()) {
        if (state_ != State::DownloadList) {
            return;
        }
        const auto count = decodePayload<MAVLink_MSG_MISSION_COUNT>(payload);
        if (!addressedToUs(count.target_system, count.target_component) || count.mission_type != missionType_) {
            return;
        }
        expectedCount_ = count.count;
        retries_ = 0;
        if (expectedCount_ == 0) {
            sendAck(MavMissionAccepted);
            downloadListDone();
            return;
        }
        state_ = State::DownloadItems;
        items_.clear();
        items_.reserve(expectedCount_);
        reportProgress();
        sendRequest(0);
        timer_.start();
        return;
    }

    if (msgId == MAVLink_MSG_MISSION_ITEM_INT::getID()) {
        if (state_ != State::DownloadItems) {
            return;
        }
        const auto item = decodePayload<MAVLink_MSG_MISSION_ITEM_INT>(payload);
        if (!addressedToUs(item.target_system, item.target_component) || item.mission_type != missionType_
            || item.seq != items_.size()) {
            return;
        }
        MissionItem m;
        m.command = item.command;
        m.frame = item.frame;
        m.current = item.current;
        m.autocontinue = item.autocontinue;
        m.param1 = item.param1;
        m.param2 = item.param2;
        m.param3 = item.param3;
        m.param4 = item.param4;
        m.x = item.x;
        m.y = item.y;
        m.z = item.z;
        items_.append(m);
        retries_ = 0;
        reportProgress();
        if (items_.size() >= expectedCount_) {
            sendAck(MavMissionAccepted);
            downloadListDone();
            return;
        }
        sendRequest(int(items_.size()));
        timer_.start();
        return;
    }

    if (msgId == MAVLink_MSG_MISSION_ACK::getID()) {
        const auto ack = decodePayload<MAVLink_MSG_MISSION_ACK>(payload);
        if (!addressedToUs(ack.target_system, ack.target_component) || ack.mission_type != missionType_) {
            return;
        }
        if (ack.type != MavMissionAccepted) {
            finish(false, ack.type);
        }
    }
}

void MissionTransfer::startUploadBatch()
{
    const MissionBatch& batch = batches_.at(batchIndex_);
    items_ = batch.items;
    missionType_ = batch.missionType;
    lastItem_ = -1;
    requested_ = 0;
    retries_ = 0;
    state_ = State::UploadCount;
    reportProgress();
    sendCount();
    timer_.start();
}

void MissionTransfer::uploadBatchDone()
{
    doneBefore_ += int(items_.size());
    if (batchIndex_ + 1 < batches_.size()) {
        ++batchIndex_;
        startUploadBatch();
        return;
    }
    finish(true, ResultOk);
}

void MissionTransfer::startDownloadList()
{
    missionType_ = downloadTypes_.at(batchIndex_);
    items_.clear();
    expectedCount_ = 0;
    retries_ = 0;
    state_ = State::DownloadList;
    reportProgress();
    sendRequestList();
    timer_.start();
}

void MissionTransfer::downloadListDone()
{
    batches_.append(MissionBatch{ missionType_, items_ });
    if (batchIndex_ + 1 < downloadTypes_.size()) {
        ++batchIndex_;
        startDownloadList();
        return;
    }
    finish(true, ResultOk);
}

void MissionTransfer::sendCount()
{
    MAVLink_MSG_MISSION_COUNT count;
    count.count = uint16_t(items_.size());
    count.target_system = uint8_t(targetSystem_);
    count.target_component = uint8_t(targetComponent_);
    count.mission_type = uint8_t(missionType_);
    emit sendMessage(MAVLink_MSG_MISSION_COUNT::getID(), payloadOf(count), MAVLink_MSG_MISSION_COUNT::v1Length());
}

void MissionTransfer::sendItem(int seq)
{
    const MissionItem& src = items_.at(seq);
    MAVLink_MSG_MISSION_ITEM_INT item;
    item.param1 = src.param1;
    item.param2 = src.param2;
    item.param3 = src.param3;
    item.param4 = src.param4;
    item.x = src.x;
    item.y = src.y;
    item.z = src.z;
    item.seq = uint16_t(seq);
    item.command = src.command;
    item.target_system = uint8_t(targetSystem_);
    item.target_component = uint8_t(targetComponent_);
    item.frame = src.frame;
    item.current = src.current;
    item.autocontinue = src.autocontinue;
    item.mission_type = uint8_t(missionType_);
    lastItem_ = seq;
    emit sendMessage(MAVLink_MSG_MISSION_ITEM_INT::getID(), payloadOf(item), MAVLink_MSG_MISSION_ITEM_INT::v1Length());
}

void MissionTransfer::sendRequestList()
{
    MAVLink_MSG_MISSION_REQUEST_LIST list;
    list.target_system = uint8_t(targetSystem_);
    list.target_component = uint8_t(targetComponent_);
    list.mission_type = uint8_t(missionType_);
    emit sendMessage(MAVLink_MSG_MISSION_REQUEST_LIST::getID(), payloadOf(list), MAVLink_MSG_MISSION_REQUEST_LIST::v1Length());
}

void MissionTransfer::sendRequest(int seq)
{
    MAVLink_MSG_MISSION_REQUEST_INT request;
    request.seq = uint16_t(seq);
    request.target_system = uint8_t(targetSystem_);
    request.target_component = uint8_t(targetComponent_);
    request.mission_type = uint8_t(missionType_);
    emit sendMessage(MAVLink_MSG_MISSION_REQUEST_INT::getID(), payloadOf(request), MAVLink_MSG_MISSION_REQUEST_INT::v1Length());
}

void MissionTransfer::sendAck(int result)
{
    MAVLink_MSG_MISSION_ACK ack;
    ack.target_system = uint8_t(targetSystem_);
    ack.target_component = uint8_t(targetComponent_);
    ack.type = uint8_t(result);
    ack.mission_type = uint8_t(missionType_);
    emit sendMessage(MAVLink_MSG_MISSION_ACK::getID(), payloadOf(ack), MAVLink_MSG_MISSION_ACK::v1Length());
}

void MissionTransfer::onTimeout()
{
    if (!busy()) {
        return;
    }
    if (++retries_ > kMaxRetries) {
        finish(false, ResultNoResponse);
        return;
    }
    switch (state_) {
    case State::UploadCount:
        sendCount();
        break;
    case State::UploadItems:
        if (lastItem_ >= 0 && requested_ < items_.size()) {
            sendItem(lastItem_);
        }
        break;
    case State::DownloadList:
        sendRequestList();
        break;
    case State::DownloadItems:
        sendRequest(int(items_.size()));
        break;
    case State::Idle:
        return;
    }
    timer_.start();
}

void MissionTransfer::finish(bool ok, int result)
{
    timer_.stop();
    const bool wasDownload = download_;
    state_ = State::Idle;
    const int missionType = missionType_;
    items_.clear();
    if (wasDownload) {
        const MissionBatches read = batches_;
        batches_.clear();
        emit downloadFinished(ok, result, missionType, read);
        return;
    }
    batches_.clear();
    emit uploadFinished(ok, result, missionType);
}

void MissionTransfer::reportProgress()
{
    if (download_) {
        const int lists = int(downloadTypes_.size());
        const int part = expectedCount_ > 0 ? int(items_.size()) * kProgressScale / expectedCount_ : 0;
        emit progressChanged(batchIndex_ * kProgressScale + part, lists * kProgressScale);
        return;
    }
    emit progressChanged(doneBefore_ + requested_, total_);
}

} // namespace autopilot

#include "dataset.h"

#include "core.h"
#include "data_processor_defs.h"
extern Core core;
#include <algorithm>
#include <QTimer>


Dataset::Dataset() :
    interpolator_(this),
    lastBottomTrackEpoch_(0),
    bSProc_(new BlackStripesProcessor()),
    sonarPosIndx_(0),
    mosaicFirstSubChId_(0),
    mosaicSecondSubChId_(0),
    lastDimRectindx_(0),
    lAngleOffset_(0.0f),
    rAngleOffset_(0.0f)
{
    qRegisterMetaType<ChannelId>("ChannelId");
    qRegisterMetaType<uint64_t>("uint64_t");
    resetDataset();
}

Dataset::~Dataset()
{
    delete bSProc_;
}

void Dataset::setState(DatasetState state)
{
    if (state_ == state) {
        return;
    }

    state_ = state;

    emit datasetStateChanged(static_cast<int>(state_)); // 0 -und, 1 -file, 2-conn
}

void Dataset::setActiveZeroing(bool state)
{
    activeZeroing_ = state;
}

Dataset::DatasetState Dataset::getState() const
{
    return state_;
}

void Dataset::getMaxDistanceRange(float *from, float *to, const ChannelId& channel1, uint8_t subAddressCh1, const ChannelId& channel2, uint8_t subAddressCh2)
{
    const int sz = size();
    float channel1_max = 0;
    float channel2_max = 0;
    for(int iepoch = 0; iepoch < sz; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if (epoch != NULL) {
            if (epoch->chartAvail(channel1, subAddressCh1)) {
                float range = epoch->chart(channel1, subAddressCh1)->range();
                if (channel1_max < range) {
                    channel1_max = range;
                }
            }

            if (epoch->chartAvail(channel2, subAddressCh2)) {
                float range = epoch->chart(channel2, subAddressCh2)->range();
                if (channel2_max < range) {
                    channel2_max = range;
                }
            }
        }
    }

    if (channel1_max > 0) {
        if (channel2_max > 0) {
            *from = -channel1_max;
            *to = channel2_max;
        }
        else {
            *from = 0;
            *to = channel1_max;
        }

    }
    else {
        *from = NAN;
        *to = NAN;
    }
}

int Dataset::getLastBottomTrackEpoch() const
{
    return lastBottomTrackEpoch_;
}

float Dataset::getLastArtificalYaw() const
{
    return lastAYaw_;
}

float Dataset::getLastArtificaPitch() const
{
    return lastAPitch_;
}

float Dataset::getLastArtificalRoll() const
{
    return lastARoll_;
}

LLARef Dataset::getLlaRef() const
{
    return _llaRef;
}

void Dataset::setLlaRef(const LLARef &val, LlaRefState state)
{
    if ((llaRefState_ == LlaRefState::kUndefined) ||
        (llaRefState_ == LlaRefState::kSettings   && (state == LlaRefState::kConnection  || state == LlaRefState::kFile)) ||
        (llaRefState_ == LlaRefState::kFile       &&  state == LlaRefState::kConnection) ||
        (llaRefState_ == LlaRefState::kConnection &&  state == LlaRefState::kFile)) {

        _llaRef = val;
        llaRefState_ = state;

        emit updatedLlaRef();
        //qDebug() << "Dataset::setLlaRef setted" << _llaRef.refLla.latitude << _llaRef.refLla.longitude << static_cast<int>(llaRefState_);
    }
}


Dataset::LlaRefState Dataset::getCurrentLlaRefState() const
{
    LlaRefState retVal = llaRefState_;

    switch (state_) {
    case DatasetState::kConnection: { retVal = LlaRefState::kConnection; break; }
    case DatasetState::kFile:       { retVal = LlaRefState::kFile;       break; }
    default: break;
    }

    return retVal;
}

void Dataset::addEvent(int timestamp, int id, int unixt) {
    lastEventTimestamp = timestamp;
    lastEventId = id;

    //    if(poolLastIndex() < 0) {
    addNewEpoch();
    //    }

    {
        QWriteLocker wl(&poolMtx_);
        pool_[endIndex()].setEvent(timestamp, id, unixt);
    }

    emit dataUpdate();
}

void Dataset::addEncoder(float angle1_deg, float angle2_deg, float angle3_deg)
{
    Q_UNUSED(angle3_deg)

    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    if(last_epoch->isEncodersSeted()) {
        last_epoch = addNewEpoch();

        if(last_epoch->isUsblSolutionAvailable()) {
            float usbl_az = last_epoch->usblSolution().azimuth_deg;
            pool_[endIndex()].setEncoders(angle1_deg, angle2_deg, (angle1_deg+usbl_az)*10);
        }
    } else {
        if(last_epoch->isUsblSolutionAvailable()) {
            float usbl_az = last_epoch->usblSolution().azimuth_deg;
            pool_[endIndex()].setEncoders(angle1_deg, angle2_deg, (angle1_deg+usbl_az)*10);
        }
    }

    // last_epoch->setEncoders(angle1_deg, angle2_deg, NAN);
    qDebug("Encoder was added");
    emit dataUpdate();
}

void Dataset::addTimestamp(int timestamp) {
    Q_UNUSED(timestamp);
}

void Dataset::setTranscSetup(const ChannelId& channelId, uint16_t freq, uint8_t pulse, uint8_t boost)
{
    usingRecordParameters_[channelId].freq  = freq;
    usingRecordParameters_[channelId].pulse = pulse;
    usingRecordParameters_[channelId].boost = boost;
}

void Dataset::setSoundSpeed(const ChannelId& channelId, uint32_t soundSpeed)
{
    usingRecordParameters_[channelId].soundSpeed  = soundSpeed;
}

void Dataset::setSonarOffset(float x, float y, float z)
{
    sonarOffset_ = QVector3D(x, y, z);
}

void Dataset::setChartSetup(const ChannelId& channelId, uint16_t resol, uint16_t count, uint16_t offset)
{
    usingRecordParameters_[channelId].resol  = resol;
    usingRecordParameters_[channelId].count = count;
    usingRecordParameters_[channelId].offset = offset;

    channelsToResizeEthData_.insert(channelId);
}

void Dataset::setFixBlackStripesState(bool state)
{
    bSProc_->setState(state);
}

void Dataset::setFixBlackStripesForwardSteps(int val)
{
    bSProc_->setForwardSteps(val);
}

void Dataset::setFixBlackStripesBackwardSteps(int val)
{
    bSProc_->setBackwardSteps(val);
}

void Dataset::addChart(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset)
{
    if (data.empty() || qFuzzyIsNull(resolution)) {
        return;
    }

    // ! we need all channels in data !
    uint8_t numSubChannels = data.size();

    if (shouldAddNewEpoch(channelId, numSubChannels)) {
        addNewEpoch();
    }

    updateEpochWithChart(channelId, chartParams, data, resolution, offset);
    const int endIndx = endIndex();

    // BLACK STRIPES PROCESSOR
    if (bSProc_->getState()) {
        QSet<int> updatedIndxs;

        // resize eth data
        if (channelsToResizeEthData_.contains(channelId)) {
            channelsToResizeEthData_.remove(channelId);
            uint16_t count = usingRecordParameters_[channelId].count;
            bSProc_->tryResizeEthalonData(channelId, numSubChannels, BlackStripesProcessor::Direction::kForward, count);
            bSProc_->tryResizeEthalonData(channelId, numSubChannels, BlackStripesProcessor::Direction::kBackward, count);
        }


        // FORWARD
        if (bSProc_->getForwardSteps()) {

            auto getPreChart = [&](int i, uint8_t subChannelId) -> const Epoch::Echogram* {
                const int preEpIndx = std::max(0, i - 1);
                if (auto* preEpoch = &pool_[preEpIndx]; preEpoch) {
                    return preEpoch->chart(channelId, subChannelId);
                }

                return nullptr;
            };

            const int remainingIndx = lastAddChartEpochIndx_[channelId] + 1;
            const uint8_t subChannelId = 0; //

            for (int i = remainingIndx; i <= endIndx; ++i) {
                if (auto* iEpoch = &pool_[i]; iEpoch) {
                    float iResolution = 0.0f;
                    float iOffset = 0.0f;

                    if (i == endIndx) {
                        iResolution = resolution;
                        iOffset = offset;
                    }
                    else {
                        if (const auto* iChart = iEpoch->chart(channelId, subChannelId); iChart) {
                            iResolution = iChart->resolution;
                            iOffset = iChart->offset;
                        }
                        else {
                            if (auto* preChart = getPreChart(i, subChannelId); preChart) {
                                iResolution = preChart->resolution;
                                iOffset = preChart->offset;
                            }
                        }
                    }

                    if (auto* preChart = getPreChart(i, subChannelId); preChart) {
                        if (!qFuzzyCompare(preChart->resolution, iResolution) || !qFuzzyCompare(preChart->offset, iOffset)) {
                            bSProc_->clearEthalonData(channelId, BlackStripesProcessor::Direction::kForward);
                        }
                    }

                    if (bSProc_->update(channelId, iEpoch, BlackStripesProcessor::Direction::kForward, iResolution, iOffset)) {
                        updatedIndxs.insert(i);
                    }
                }
            }
        }

        // BACKWARD
        int backSteps = bSProc_->getBackwardSteps();
        if (backSteps) {
            bSProc_->clearEthalonData(channelId, BlackStripesProcessor::Direction::kBackward);

            const int startIndx  = std::max(0, endIndx - backSteps);
            for (int i = endIndx; i >= startIndx; --i) {
                if (auto* iEpoch = &pool_[i]; iEpoch) {
                    float iResolution = resolution;
                    float iOffset = offset;
                    if (const auto* iChart = iEpoch->chart(channelId); iChart) {
                        iResolution = iChart->resolution;
                        iOffset = iChart->offset;
                        if (!qFuzzyCompare(iChart->resolution, resolution) || !qFuzzyCompare(iChart->offset, offset)) {
                            bSProc_->clearEthalonData(channelId, BlackStripesProcessor::Direction::kBackward);
                        }
                    }

                    if (bSProc_->update(channelId, iEpoch, BlackStripesProcessor::Direction::kBackward, iResolution, iOffset)) {
                        updatedIndxs.insert(i);
                    }
                }
            }
        }


        if (!updatedIndxs.empty()) {
            emit redrawEpochs(updatedIndxs);
        }
    }

    lastAddChartEpochIndx_[channelId] = endIndx;

    for (int i = 0; i < numSubChannels; ++i) {
        validateChannelList(channelId, i);
    }

    int lastIndx = std::max(0, (size() - 1) - (bSProc_->getState() ? bSProc_->getBackwardSteps() : 0)); // TODO: РЅРµ РїСЂРѕСЃС‚Рѕ РєРѕР»-РІРѕ СЌРїРѕС… - РѕРєРЅРѕ РЅР°Р·Р°Рґ, Р° РїРѕСЃР»РµРґРЅСЏСЏ РЅРµРёР·РјРµРЅРЅР°СЏ СЌРїРѕС…Р° РїРѕ С‡Р°СЂС‚Р°Рј
    emit dataUpdate();
    emit chartAdded(lastIndx);
}

void Dataset::rawDataRecieved(const ChannelId& channelId, RawData raw_data) {
    RawData::RawDataHeader header = raw_data.header;
    ComplexF* compelex_data = (ComplexF*)raw_data.data.data();
    int16_t* real16_data = (int16_t*)raw_data.data.data();
    int16_t* complex16_data = (int16_t*)raw_data.data.data();
    int size = raw_data.samplesPerChannel();

    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    std::reference_wrapper<ComplexSignals> compex_signals = last_epoch->complexSignals();

    ChannelId dev_id(channelId.uuid, header.channelGroup); // channelId.uuid

    if(compex_signals.get()[dev_id].contains(header.channelGroup)) {
        float offset_m = 0;
        float offset_db = 0;
        offset_db = -20;

        Q_UNUSED(offset_m)
        Q_UNUSED(offset_db)

        // last_epoch->moveComplexToEchogram(offset_m, offset_db);
        last_epoch = addNewEpoch();

        compex_signals = std::ref(last_epoch->complexSignals());

    }

    QVector<ComplexSignal>& channels = compex_signals.get()[dev_id][header.channelGroup];
    channels.resize(header.channelCount);

    for(int ich = 0; ich < header.channelCount; ich++) {
        ComplexSignal& signal = channels[ich]; //??

        signal.globalOffset = header.globalOffset;
        signal.sampleRate = header.sampleRate;
        signal.data.resize(size);

        ComplexF* signal_data = signal.data.data();

        if(header.dataType == 0) {
            signal.isComplex = true;

            for(int i  = 0; i < size; i++) {
                signal_data[i] = compelex_data[i*header.channelCount + ich];
            }
        } else if(header.dataType == 1) {
            signal.isComplex = false;

            for(int i  = 0; i < size; i++) {
                signal_data[i] = ComplexF(real16_data[i*header.channelCount + ich], 0);
            }
        } else if(header.dataType == 2) {
            signal.isComplex = true;

            for(int i  = 0; i < size; i++) {
                signal_data[i] = ComplexF(complex16_data[(i*header.channelCount + ich)*2], complex16_data[(i*header.channelCount + ich)*2+1]);
            }
        }
    }

    float offset_m = 0;
    float offset_db = 0;
    offset_db = -20;
    last_epoch->moveComplexToEchogram(dev_id, header.channelGroup, offset_m, offset_db);

    for(int ich = 0; ich < header.channelCount; ich++) {
        validateChannelList(dev_id, ich);
    }

    emit dataUpdate();
}

void Dataset::addDist(const ChannelId& channelId, int dist)
{
    int pool_index = endIndex();

    if (pool_index < 0 || pool_[pool_index].distAvail() == true) {
        addNewEpoch();
        pool_index = endIndex();
    }

    pool_[pool_index].setDist(channelId, dist);

    const float distMeters = static_cast<float>(dist) * 0.001f;
    setLastRangefinderDepth(distMeters);
    setLastDepth(distMeters);

    emit dataUpdate();
}

void Dataset::addRangefinder(const ChannelId& channelId, float distance)
{
    Epoch* epoch = last();
    if (!epoch) {
        return;
    }
    if (epoch->distAvail()) {
        epoch = addNewEpoch();
    }

    setLastRangefinderDepth(distance);
    setLastDepth(distance);

    epoch->setDist(channelId, distance * 1000);

    emit dataUpdate();
}

void Dataset::addUsblSolution(IDBinUsblSolution::UsblSolution data) {
    int pool_index = endIndex();
    if(pool_index < 0 || pool_[pool_index].isUsblSolutionAvailable() == true) {
        addNewEpoch();
        //pool_index = endIndex();
    }

    // tracks[data.id].data_.append(QVector3D(data.x_m, data.y_m, data.depth_m));
    tracks[-1].data_.append(QVector3D());
    tracks[-1].objectColor_ = QColor(0, 255, 255);
    tracks[-1].type_ = UsblView::UsblObjectType::kBeacon;
    //tracks[-1].yaw_ = 100.0f;


    Position pos;
    pos.lla = LLA(data.usbl_latitude, data.usbl_longitude);

    static float dist_save = NAN;
    static float angl_save = NAN;

    Q_UNUSED(dist_save);
    Q_UNUSED(angl_save);

    // float angl = qDegreesToRadians(-data.usbl_yaw - data.azimuth_deg);
    float angl_usbl = data.azimuth_deg;
    float dist = data.distance_m;

    // float x_beacon = dist*cosf(angl);
    // float y_beacon = dist*sinf(angl);

    // data.azimuth_deg = (data.usbl_yaw + data.azimuth_deg);


    if(pos.lla.isCoordinatesValid()
        // && abs(dist_save - dist) < 0.5
        // && (abs(angl_save - angl_usbl) < 10)
        // && x_beacon < 0
        ) {
        // qDebug("usbl lat %f, lon %f", data.usbl_latitude, data.usbl_longitude);

        setLlaRef(LLARef(pos.lla), getCurrentLlaRefState());

        pos.LLA2NED(&_llaRef);
        // qDebug("usbl x %f, y %f", pos.ned.n, pos.ned.e);

        tracks[-2].data_.append(QVector3D(pos.ned.n, pos.ned.e, 0));
        tracks[-2].objectColor_ = QColor(0, 200, 0);
        tracks[-2].type_ = UsblView::UsblObjectType::kUsbl;
        tracks[-2].yaw_ = 0.0f;


        float beacon_n = data.beacon_n_m;
        float beacon_e = data.beacon_e_m;

        if(pos.ned.isCoordinatesValid()) {
            beacon_n += pos.ned.n;
            beacon_e += pos.ned.e;
        }

        tracks[-4].data_.append(QVector3D(beacon_n, beacon_e, 0));
        tracks[-4].objectColor_ = QColor(200, 0, 0);
        tracks[-4].lineWidth_ = 15;
        tracks[-4].pointRadius_ = 50;
        tracks[-4].type_ = UsblView::UsblObjectType::kBeacon;
        tracks[-4].yaw_ = 90.0f;


        // Position pos_beacon;
        // pos_beacon.lla = LLA(data.latitude_deg, data.longitude_deg);
        // if(pos_beacon.lla.isCoordinatesValid()) {
        //     pos_beacon.LLA2NED(&_llaRef);
        //     tracks[-5].data_.append(QVector3D(pos_beacon.ned.n, pos_beacon.ned.e, 0));
        //     tracks[-5].objectColor_ = QColor(0, 200, 0);
        //     tracks[-5].lineWidth_ = 5;
        // }

        // _pool[endIndex()].set(data);
    } else {
         tracks[-4].data_.append(QVector3D(NAN, NAN, 0));
    }
    dist_save = dist;
    angl_save = angl_usbl;

    std::shared_ptr<UsblView> view = scene3dViewPtr_->getUsblViewPtr();
    view->setTrackRef(tracks);

    pool_[endIndex()].setAtt(data.usbl_yaw, data.usbl_pitch, data.usbl_roll);
    pool_[endIndex()].set(data);
    // float enc_az= pool_[endIndex()].encoder1();
    // float enc_el= pool_[endIndex()].encoder2();
    // float usbl_az = data.azimuth_deg;
    // pool_[endIndex()].setEncoders(enc_az, enc_el, -enc_az+usbl_az);
    emit dataUpdate();
}

void Dataset::addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    int pool_index = endIndex();

    if(pool_index < 0 || (pool_[pool_index].isDopplerBeamAvail() == true)) { //
        addNewEpoch();
    }

    pool_index = endIndex();

    pool_[pool_index].setDopplerBeam(beams, cnt);
    emit dataUpdate();
}

void Dataset::addDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    int pool_index = endIndex();

    if(pool_index < 0 || (pool_[pool_index].isDopplerBeamAvail() == false)) { //
        addNewEpoch();
        pool_index = endIndex();
    }

    pool_[pool_index].setDVLSolution(dvlSolution);
    emit dataUpdate();
}

void Dataset::addAtt(float yaw, float pitch, float roll)
{
    if (activeZeroing_) {
        Epoch* lastEp = last();
        if (!lastEp) {
            return;
        }

        ++testTime_;

        Position pos;
        double lat = 55.0151f, lon = 21.1183f;
        pos.lla = LLA(lat, lon);
        pos.time = DateTime(testTime_, 100);

        if (pos.lla.isCoordinatesValid()) {
            if (lastEp->getPositionGNSS().lla.isCoordinatesValid()) {
                //qDebug() << "pos add new epoch" << _pool.size();
                lastEp = addNewEpoch();
            }
            uint64_t lastIndx = pool_.size() - 1;
            if (!getLlaRef().isInit) {
                LlaRefState llaState = state_ == DatasetState::kUndefined ? LlaRefState::kFile : (state_ == DatasetState::kFile ?  LlaRefState::kFile :  LlaRefState::kConnection);
                setLlaRef(LLARef(pos.lla), llaState /*Dataset::LlaRefState::kConnection*/); // TODO
            }

            tryResetDataset(pos.lla.latitude, pos.lla.longitude);

            lastEp->setPositionLLA(pos);
            lastEp->setPositionRef(&_llaRef);
            lastEp->setPositionDataType(DataType::kRaw);
            interpolator_.interpolatePos(false); //

            {
                speed_ =  0.0;
                emit speedChanged();
            }

            boatLatitute_ = pos.lla.latitude;
            boatLongitude_ = pos.lla.longitude;

            emit positionAdded(lastIndx);
            emit dataUpdate();
            emit lastPositionChanged();
        }
    }

    uint64_t lastIndx = pool_.size() - 1;

    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    if(last_epoch->isAttAvail()) {
        // last_epoch = addNewEpoch();
    }

    last_epoch->setAtt(yaw, pitch, roll);

    _lastYaw = yaw;
    _lastPitch = pitch;
    _lastRoll = roll;

    interpolator_.interpolateAtt(false);

    emit attitudeAdded(lastIndx);
    emit dataUpdate();
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec)
{
    if (activeZeroing_) {
        return;
    }

    Epoch* lastEp = last();
    if (!lastEp) {
        return;
    }

    Position pos;
    pos.lla = LLA(lat, lon);
    pos.time = DateTime(unix_time, nanosec);
    const bool oneHzNoTimestamp = (unix_time == 0 && nanosec == 0);

    if (pos.lla.isCoordinatesValid()) {
        if (lastEp->getPositionGNSS().lla.isCoordinatesValid()) {
            //qDebug() << "pos add new epoch" << _pool.size();
            lastEp = addNewEpoch();
        }
        uint64_t lastIndx = pool_.size() - 1;
        if (!getLlaRef().isInit) {
            LlaRefState llaState = state_ == DatasetState::kUndefined ? LlaRefState::kFile : (state_ == DatasetState::kFile ?  LlaRefState::kFile :  LlaRefState::kConnection);
            setLlaRef(LLARef(pos.lla), llaState /*Dataset::LlaRefState::kConnection*/); // TODO
        }

        tryResetDataset(pos.lla.latitude, pos.lla.longitude);

        lastEp->setPositionLLA(pos);
        lastEp->setPositionRef(&_llaRef);
        lastEp->setPositionDataType(DataType::kRaw);
        interpolator_.interpolatePos(false);

        if (Epoch* prevEp = lastlast(); prevEp) {
            const auto& prev = prevEp->getPositionGNSS();
            if (prev.lla.isCoordinatesValid()) {
                const double dist = distanceMetersLLA(prev.lla.latitude, prev.lla.longitude, pos.lla.latitude,  pos.lla.longitude);

                if (oneHzNoTimestamp) {
                    speed_ = (dist / 0.1) * 3.6; // TODO: kostyl
                }
                else {
                    const auto& c = pos.time;
                    const auto& p = prev.time;

                    int64_t dsec  = int64_t(c.sec)     - int64_t(p.sec);
                    int64_t dnano = int64_t(c.nanoSec) - int64_t(p.nanoSec);
                    if (dnano < 0) {
                        dsec -= 1;
                        dnano += 1000000000;
                    }

                    double dt = double(dsec) + double(dnano) * 1e-9;

                    if (dt <= 0.0) {
                        dt = 1.0;
                    }

                    speed_ = (dist / dt) * 3.6;
                }

                emit speedChanged();
            }
        }

        //qDebug() << "add pos for" << lastIndx;

        boatLatitute_ = pos.lla.latitude;
        boatLongitude_ = pos.lla.longitude;

        if (isValidActiveContactIndx()) {
            if (auto* ep = fromIndex(activeContactIndx_); ep) {
                const double latTarget = ep->contact_.lat;
                const double lonTarget = ep->contact_.lon;
                const double latBoat = pos.lla.latitude;
                const double lonBoat = pos.lla.longitude;

                distToActiveContact_ = distanceMetersLLA(latBoat, lonBoat, latTarget, lonTarget);

                double yawDeg = _lastYaw;
                if (!std::isfinite(yawDeg)) {
                    yawDeg = lastAYaw_;
                }

                if (std::isfinite(yawDeg)) {
                    angleToActiveContact_ = angleToTargetDeg(latBoat, lonBoat, latTarget, lonTarget, yawDeg);
                }
            }
        }

        emit positionAdded(lastIndx);
        emit dataUpdate();
        emit lastPositionChanged();
    }

    addArtificalYaw();
}

void Dataset::addArtificalYaw()
{
    auto* llPtr = lastlast();
    auto* lPtr  = last();
    if (!llPtr || !lPtr) {
        return;
    }

    auto llNed = llPtr->getPositionGNSS().ned;
    auto lNed  = lPtr->getPositionGNSS().ned;
    if (!llNed.isCoordinatesValid() || !lNed.isCoordinatesValid()) {
        return;
    }

    const double dN = lNed.n - llNed.n;
    const double dE = lNed.e - llNed.e;
    if (qFuzzyIsNull(dN) && qFuzzyIsNull(dE)) {
        return;
    }

    double yawRad = std::atan2(dE, dN);
    double yawDeg = qRadiansToDegrees(yawRad);
    if (yawDeg < 0.0) {
        yawDeg += 360.0;
    }

    uint64_t indx = pool_.size() - 1;
    const float aYaw = static_cast<float>(yawDeg);
    const float aPitch = 0.0f;
    const float aRoll = 0.0f;

    lPtr->setArtificalAtt(aYaw, aPitch, aRoll);
    lastAYaw_   = aYaw;
    lastAPitch_ = aPitch;
    lastARoll_  = aRoll;

    interpolator_.interpolateArtificalAtt(false);

    emit artificalAttitudeAdded(indx);
}

void Dataset::addPositionRTK(Position position) {
    if (activeZeroing_) {
        return;
    }

    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    last_epoch->setExternalPosition(position);
}

void Dataset::addDepth(float depth) {
    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    if(last_epoch->isDepthAvail()) {
        last_epoch = addNewEpoch();
    }

    setLastDepth(depth);

    last_epoch->setDepth(depth);
}

void Dataset::addGnssVelocity(double h_speed, double course) {
    int pool_index = endIndex();
    if(pool_index < 0) {
        addNewEpoch();
        pool_index = endIndex();
    }

//    if(isfinite(_pool[pool_index].gnssHSpeed())) {
//        makeNewEpoch();
//        pool_index = endIndex();
//    }


    pool_[pool_index].setGnssVelocity(h_speed, course);
    emit dataUpdate();
}

void Dataset::addTemp(float temp_c) {
    //qDebug() << "Dataset::addTemp" << temp_c;
    lastTemp_ = temp_c;
    Epoch* last_epoch = last();
    if (!last_epoch) {
        return;
    }
    last_epoch->setTemp(temp_c);
    // qDebug() << "Dataset Temp: " << temp_c;

    // int pool_index = endIndex();
    // if(pool_index < 0) {
    //     addNewEpoch();
    //     pool_index = endIndex();
    // }
    // pool_[pool_index].setTemp(temp_c);
    // emit dataUpdate();
}

void Dataset::mergeGnssTrack(QList<Position> track) {
    const int64_t max_difference_ns = 1e9;
    const int psize = size();
    const int tsize = track.size();
    int track_pos_save = 0;
    volatile int sync_count = 0;

    for(int iepoch = 0; iepoch < psize; iepoch++) {
        Epoch* epoch =  fromIndex(iepoch);
        Position boatPos = epoch->getPositionGNSS();

        DateTime time_epoch = *epoch->time();
        if(time_epoch.sec > 0) {
            boatPos.time = *epoch->time();
            boatPos.time.sec -= 18;
        }

        int64_t internal_ns  = boatPos.time.sec*1e9+boatPos.time.nanoSec;


        if(internal_ns > 0) {
            int64_t min_dif_ns = max_difference_ns;
            int min_ind = -1;
            for(int track_pos = track_pos_save; track_pos < tsize;track_pos++) {
                int64_t track_ns  = track[track_pos].time.sec*1e9+track[track_pos].time.nanoSec;
                if(track_ns > 0) {
                    int64_t dif_ns = track_ns - internal_ns;
                    if(min_dif_ns > abs(dif_ns)) {
                        min_dif_ns = abs(dif_ns);
                        min_ind = track_pos;
                    }

                    if(dif_ns > max_difference_ns) { break; }
                }
            }

            if(min_ind > 0) {
                track_pos_save = min_ind;
                epoch->setExternalPosition(track[min_ind]);
                sync_count++;
            }
        }
    }
    emit dataUpdate();
}


void Dataset::resetDataset()
{
    //qDebug() << "Dataset::resetDataset()";
    {
        QWriteLocker locker(&lock_);
        channelsSetup_.clear();
        firstChannelId_ = DatasetChannel();
    }

    resetRenderBuffers();

    resetDistProcessing();

    setState(DatasetState::kUndefined);

    testTime_ = 1740466541;
    usingRecordParameters_.clear();
    lastAddChartEpochIndx_.clear();
    channelsToResizeEthData_.clear();

    activeContactIndx_    = -1;
    boatLatitute_         = 0.0f;
    boatLongitude_        = 0.0f;
    distToActiveContact_  = 0.0f;
    angleToActiveContact_ = 0.0f;
    lastDepth_            = 0.0f;
    lastRangefinderDepth_ = NAN;
    lastBottomTrackDepth_ = NAN;

    sonarPosIndx_ = 0;
    pendingSonarPosIndx_ = 0;
    pendingDimRectIndx_ = 0;
    setSpatialPreparing(false);
    lastDimRectindx_ = 0;

    emit lastDepthChanged();
    emit channelsUpdated();
    emit dataUpdate();
    emit lastPositionChanged();
    emit activeContactChanged();
}

void Dataset::softResetDataset() // for long-distance camera movement
{
    {
        QWriteLocker locker(&lock_);
        channelsSetup_.clear();
        firstChannelId_ = DatasetChannel();
    }

    resetRenderBuffers();

    resetDistProcessing();
    testTime_ = 1740466541;
    usingRecordParameters_.clear();
    lastAddChartEpochIndx_.clear();
    channelsToResizeEthData_.clear();

    activeContactIndx_    = -1;
    boatLatitute_         = 0.0f;
    boatLongitude_        = 0.0f;
    distToActiveContact_  = 0.0f;
    angleToActiveContact_ = 0.0f;
    lastDepth_            = 0.0f;
    lastRangefinderDepth_ = NAN;
    lastBottomTrackDepth_ = NAN;

    sonarPosIndx_ = 0;
    pendingSonarPosIndx_ = 0;
    pendingDimRectIndx_ = 0;
    setSpatialPreparing(false);

    mosaicFirstChId_.clear();
    mosaicSecondChId_.clear();
    mosaicFirstSubChId_ = 0;
    mosaicSecondSubChId_ = 0;
    lastDimRectindx_ = 0;

    emit lastDepthChanged();
    emit channelsUpdated();
    emit dataUpdate();
    emit lastPositionChanged();
    emit activeContactChanged();
}

void Dataset::resetRenderBuffers()
{
    clearTileEpochIndex();
    tracks.clear();
    pool_.clear();
    pool_.shrink_to_fit();//
    lastAYaw_ = NAN;
    lastAPitch_ = NAN;
    lastARoll_ = NAN;
    _lastYaw = NAN;
    _lastPitch = NAN;
    _lastRoll = NAN;
    lastTemp_ = NAN;
    lastRangefinderDepth_ = NAN;
    lastBottomTrackDepth_ = NAN;
    interpolator_.clear();
    _llaRef = LLARef();
    llaRefState_ = LlaRefState::kUndefined;
    bSProc_->clear();
    lastBottomTrackEpoch_ = 0;
    pendingSonarPosIndx_ = 0;
    pendingDimRectIndx_ = 0;
    setSpatialPreparing(false);
}

void Dataset::resetDistProcessing() {
//     int pool_size = size();
//     for(int i = 0; i < pool_size; i++) {
// //        Epoch* dataset = fromIndex(i);
// //        dataset->resetDistProccesing();
//     }
}

void Dataset::setChannelOffset(const ChannelId& channelId, float x, float y, float z)
{
    QWriteLocker locker(&lock_);

    // write to all on ChannelId
    for (int16_t i = 0; i < channelsSetup_.size(); ++i) {
        if (channelsSetup_.at(i).channelId_ == channelId) {
            channelsSetup_[i].localPosition_.x = x;
            channelsSetup_[i].localPosition_.y = y;
            channelsSetup_[i].localPosition_.z = z;
        }
    }
}

void Dataset::spatialProcessing() {
    auto ch_list = channelsList();
    for (auto it = ch_list.cbegin(); it != ch_list.cend(); ++it) {
        ChannelId ich = it->channelId_;

        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }

            Position ext_pos = epoch->getExternalPosition();

            if(epoch->chartAvail(ich)) {
                Epoch::Echogram* data = epoch->chart(ich);

                if(data == NULL) { continue; }

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += it->localPosition_.z;
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= it->localPosition_.z;
                }

                data->sensorPosition = ext_pos;

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += data->bottomProcessing.getDistance();
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= data->bottomProcessing.getDistance();
                }

                data->bottomProcessing.bottomPoint = ext_pos;
            }
        }
    }
}

void Dataset::usblProcessing() {
    const int to_size = size();
    int from_index = 0;

    _beaconTrack.clear();
    _beaconTrack1.clear();

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* epoch = fromIndex(i);
        Position boatPos = epoch->getPositionGNSS();

        // if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
        //     if(!_llaRef.isInit) {
        //         _llaRef = LLARef(pos.lla);
        //     }
        //     pos.LLA2NED(&_llaRef);
        // }

        if(boatPos.ned.isCoordinatesValid() && epoch->isAttAvail() && epoch->isUsblSolutionAvailable()) {
            double n = boatPos.ned.n, e = boatPos.ned.e;
            Q_UNUSED(n);
            Q_UNUSED(e);
            double yaw = epoch->yaw();
            double azimuth = epoch->usblSolution().azimuth_deg-180;
            double dist = epoch->usblSolution().distance_m;
            double dir = ((yaw + azimuth) + 120);
            double rel_n = dist*cos(qDegreesToRadians(dir));
            double rel_e = dist*sin(qDegreesToRadians(dir));
            Q_UNUSED(rel_n);
            Q_UNUSED(rel_e);
            // if(i > 4000 && i < 4500) {
            //     _beaconTrack.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 250 && (abs(azimuth)  > 170)) {
            //     _beaconTrack1.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 50 && azimuth < 10  && azimuth > -10) {
            //     _beaconTrack.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 250 && (abs(azimuth)  > 170)) {
            //     _beaconTrack1.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }

        }
    }
}

void Dataset::setRefPosition(int epoch_index) {
    Epoch*  ref_epoch = fromIndex(epoch_index);
    setRefPosition(ref_epoch);
}

void Dataset::setRefPosition(Epoch* epoch) {
    if(epoch == NULL) { return; }

    setRefPosition(epoch->getPositionGNSS());
}

void Dataset::setRefPosition(Position ref_pos) {
    if(ref_pos.lla.isCoordinatesValid()) {
        setLlaRef(LLARef(ref_pos.lla), getCurrentLlaRefState());
        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }
            epoch->setPositionRef(&_llaRef);
        }
    }
}

void Dataset::setRefPositionByFirstValid() {
    Epoch* epoch = getFirstEpochByValidPosition();
    if(epoch == NULL) { return; }

    setRefPosition(epoch);
}

Epoch *Dataset::getFirstEpochByValidPosition() {
    for(int iepoch = 0; iepoch < size(); iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch == NULL) { continue; }
        if(epoch->getPositionGNSS().lla.isCoordinatesValid()) {
            return epoch;
        }
    }

    return NULL;
}

QStringList Dataset::channelsNameList()
{
    channelsNames_.clear();
    channelsIds_.clear();
    subChannelIds_.clear();

    QStringList result;

    result << QString(tr("None"));

    channelsNames_ << QString(tr("None"));
    channelsIds_   << ChannelId();
    subChannelIds_ << 0x00;

    const QVector<DatasetChannel> chList = channelsList();

    for (const auto& channel : chList) {

        const ChannelId& chId = channel.channelId_;
        uint8_t sub = channel.subChannelId_;

        QString name = QString("%1|%2|%3").arg(channel.portName_, QString::number(channel.channelId_.address), QString::number(sub));

        result << name;

        channelsNames_ << name;
        channelsIds_   << chId;
        subChannelIds_ << sub;
    }

    return result;
}

void Dataset::onDistCompleted(int epIndx, const ChannelId& channelId, float dist)
{
    Epoch* epPtr = fromIndex(epIndx);

    if (!epPtr) {
        return;
    }

    bool settedChart = false;

    // for all sub ch
    auto numSubChs = epPtr->getChartsSizeByChannelId(channelId);
    for (int subChId = 0; subChId < numSubChs; ++subChId) {
        if (epPtr->chartAvail(channelId, subChId)) {
            Epoch::Echogram* chart = epPtr->chart(channelId, subChId);
            if (chart) {
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSource::DistanceSourceProcessing);
                settedChart = true;
            }
        }
    }

    if (settedChart) {
        setLastBottomTrackDepth(dist);
        setLastDepth(dist);

        if (firstChannelId_.channelId_ != channelId) { // only if first channel updated
            return;
        }

        int guardInterval = bottomTrackParam_.windowSize; // bottomTrack will proceed epIndx - guardInterval in next iteration
        int compIndx = epIndx > guardInterval ? epIndx - guardInterval : epIndx;
        emit bottomTrackAdded(compIndx);
    }
}

void Dataset::onDistCompletedBatch(const QVector<BottomTrackUpdate>& updates)
{
    if (updates.isEmpty()) {
        return;
    }

    bool haveDepth = false;
    float lastDepth = NAN;
    int maxCompIndx = -1;

    for (const auto& update : updates) {
        Epoch* epPtr = fromIndex(update.epochIndex);
        if (!epPtr) {
            continue;
        }

        bool settedChart = false;
        const int numSubChs = epPtr->getChartsSizeByChannelId(update.channelId);
        for (int subChId = 0; subChId < numSubChs; ++subChId) {
            if (epPtr->chartAvail(update.channelId, subChId)) {
                Epoch::Echogram* chart = epPtr->chart(update.channelId, subChId);
                if (chart) {
                    chart->bottomProcessing.setDistance(update.distance, Epoch::DistProcessing::DistanceSource::DistanceSourceProcessing);
                    settedChart = true;
                }
            }
        }

        if (!settedChart) {
            continue;
        }

        lastDepth = update.distance;
        haveDepth = true;

        if (firstChannelId_.channelId_ != update.channelId) {
            continue;
        }

        const int guardInterval = bottomTrackParam_.windowSize;
        const int compIndx = update.epochIndex > guardInterval ? update.epochIndex - guardInterval : update.epochIndex;
        if (compIndx > maxCompIndx) {
            maxCompIndx = compIndx;
        }
    }

    if (haveDepth) {
        setLastBottomTrackDepth(lastDepth);
        setLastDepth(lastDepth);
    }

    if (maxCompIndx >= 0) {
        emit bottomTrackAdded(maxCompIndx);
    }
}

void Dataset::onLastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual, bool redrawAll)
{
    bottomTrackParam_ = btP;
    lastBottomTrackEpoch_ = val;

    const int minMagicRenderGap = std::max(3, bottomTrackParam_.windowSize);
    const int lEpoch = std::max(0, bottomTrackParam_.indexFrom - minMagicRenderGap);
    const int rEpoch = std::max(0, bottomTrackParam_.indexTo   - minMagicRenderGap);

    emit dataUpdate(); // for 2D
    emit bottomTrackUpdated(channelId, lEpoch, rEpoch, manual, redrawAll); // 3D
}

void Dataset::onDimensionRectCanCalc(uint64_t indx)
{
    //qDebug() << "Dataset::onDimensionRectCanCalc" << indx;

    pendingDimRectIndx_ = std::max(pendingDimRectIndx_, indx);

    if (!dimRectIndexingEnabled_) {
        return;
    }

    const uint64_t safeTarget = std::min(pendingDimRectIndx_, sonarPosIndx_);
    if (safeTarget <= lastDimRectindx_) {
        return;
    }

    uint64_t chunkTarget = safeTarget;
    if (chunkedSpatialCatchup_) {
        static constexpr uint64_t kDimRectChunk = 128;
        chunkTarget = std::min(safeTarget, lastDimRectindx_ + kDimRectChunk);
    }

    calcDimensionRects(chunkTarget);

    pendingDimRectIndx_ = std::max(pendingDimRectIndx_, lastDimRectindx_);

    if (chunkedSpatialCatchup_ && (std::min(pendingDimRectIndx_, sonarPosIndx_) > lastDimRectindx_)) {
        scheduleSpatialCatchup();
    }
}

void Dataset::validateChannelList(const ChannelId &channelId, uint8_t subChannelId)
{
    int16_t indx = -1;

    {
        QWriteLocker locker(&lock_);

        if (channelsSetup_.empty()) {
            firstChannelId_ = DatasetChannel(channelId, subChannelId); //
        }

        for (int16_t i = 0; i < channelsSetup_.size(); ++i) {
            if (channelsSetup_.at(i).channelId_ == channelId &&
                channelsSetup_.at(i).subChannelId_ == subChannelId) {
                indx = i;
                break;
            }
        }

        if (indx != -1) {
            if (channelsSetup_[indx].portName_.isEmpty()) {
                auto links = core.getLinkNames();
                if (links.contains(channelId.uuid)) {
                    channelsSetup_[indx].portName_ = links[channelId.uuid];
                }
            }

            channelsSetup_[indx].counter();
        }
        else {
            auto newDCh = DatasetChannel(channelId, subChannelId);
            auto links = core.getLinkNames();

            if (links.contains(channelId.uuid)) {
                newDCh.portName_ = links[channelId.uuid];
            }
            else {
                newDCh.portName_ = "None";
            }

            channelsSetup_.push_back(newDCh);
        }
    }

    if (indx == -1) {
        emit channelsUpdated();
    }
}

Epoch *Dataset::addNewEpoch()
{
    bool beenAdded = false;
    int indxAdded = -1;
    Epoch* ptrAdded = nullptr;

    {
        QWriteLocker wl(&poolMtx_);

        uint64_t newSize = pool_.size() + 1;
        pool_.resize(newSize);
        ptrAdded = last();

        beenAdded = true;
        indxAdded = newSize;
    }

    if (beenAdded) {
        emit epochAdded(indxAdded);
    }

    return ptrAdded;
}

bool Dataset::shouldAddNewEpoch(const ChannelId &channelId, uint8_t numSubChannels) const
{
    const int lastIndx = endIndex();

    if (lastIndx == -1) {
        return true;
    }

    const auto& epoch = pool_[lastIndx];

    for (int i = 0; i < numSubChannels; ++i) {
        if (!epoch.chartAvail(channelId, i)) {
            return false;
        }
    }

    return true;
}

void Dataset::updateEpochWithChart(const ChannelId &channelId, const ChartParameters &chartParams, const QVector<QVector<uint8_t> > &data, float resolution, float offset)
{
    const int indx = endIndex();
    auto& epoch = pool_[indx];

    RecordParameters recParam;
    if (usingRecordParameters_.contains(channelId)) {
        recParam = usingRecordParameters_[channelId];
    }

    epoch.setChart(channelId, data, resolution, offset);
    epoch.setRecParameters(channelId, recParam);
    epoch.setChartParameters(channelId, chartParams);
}

void Dataset::setLastDepth(float val)
{
    lastDepth_ = val;

    emit lastDepthChanged();
}

void Dataset::setLastRangefinderDepth(float val)
{
    lastRangefinderDepth_ = val;
}

void Dataset::setLastBottomTrackDepth(float val)
{
    lastBottomTrackDepth_ = val;
}

void Dataset::calcDimensionRects(uint64_t indx)
{
    //qDebug() << "void Dataset::calcTracingDimensions()";

    auto* mip = core.getMosaicIndexProviderPtr();
    if (!mip) {
        return;
    }

    const bool hasMosaicChannels = mosaicFirstChId_.isValid() && mosaicSecondChId_.isValid();
    const int baseZoom = mip->getMaxZoom();
    const int maxZoom = mip->getMinZoom();

    uint64_t lastIndx = lastDimRectindx_;
    uint64_t currIndx = indx;

    if (currIndx >= static_cast<uint64_t>(pool_.size())) {
        qWarning() << "Dataset::calcTracingDimensions out of indxs";
        return;
    }

    auto parentIndex2 = [](int i) -> int {
        if (i >= 0) {
            return i >> 1;
        }
        return -(((-i) + 1) >> 1);
    };

    auto buildTilesByZoom = [&](const QSet<TileKey>& baseTiles) -> QMap<int, QSet<TileKey>> {
        QMap<int, QSet<TileKey>> tilesByZoom;
        if (baseTiles.isEmpty()) {
            return tilesByZoom;
        }

        tilesByZoom[baseZoom] = baseTiles;

        for (int z = baseZoom + 1; z <= maxZoom; ++z) {
            const auto& prevSet = tilesByZoom[z - 1];
            auto& currSet = tilesByZoom[z];

            for (const TileKey& k : prevSet) {
                TileKey parent;
                parent.zoom = z;
                parent.x    = parentIndex2(k.x);
                parent.y    = parentIndex2(k.y);
                currSet.insert(parent);
            }
        }

        return tilesByZoom;
    };

    auto publishTilesForEpoch = [&](uint64_t epochIndx, const QMap<int, QSet<TileKey>>& tilesByZoom) -> bool {
        const auto baseIt = tilesByZoom.constFind(baseZoom);
        if (baseIt == tilesByZoom.cend() || baseIt->isEmpty()) {
            return false;
        }

        pool_[epochIndx].setTraceTileIndxs(tilesByZoom); // в эпоху в датасете
        appendTileEpochIndex(static_cast<int>(epochIndx), tilesByZoom); // в датасет
        emit sendTilesByZoom(static_cast<int>(epochIndx), tilesByZoom); // в dataProcessor
        return true;
    };

    auto tryGetEpochNed = [](Epoch* epoch, NED* outNed) -> bool {
        if (!epoch || !outNed) {
            return false;
        }

        NED ned = epoch->getSonarPosition().ned;
        if (!ned.isCoordinatesValid()) {
            ned = epoch->getPositionGNSS().ned;
        }
        if (!ned.isCoordinatesValid()) {
            return false;
        }

        *outNed = ned;
        return true;
    };

    auto publishFallbackPointTile = [&](uint64_t epochIndx, Epoch* epoch) -> bool {
        NED ned;
        if (!tryGetEpochNed(epoch, &ned)) {
            return false;
        }

        QSet<TileKey> baseTiles;
        baseTiles.insert(tileKeyFromWorld(static_cast<float>(ned.n), static_cast<float>(ned.e), baseZoom));
        return publishTilesForEpoch(epochIndx, buildTilesByZoom(baseTiles));
    };

    for (uint64_t i = lastIndx; i < currIndx; ++i) {
        uint64_t llIndx = i;
        uint64_t  lIndx = i + 1;

        auto* llPtr = &pool_[llIndx];
        auto* lPtr  = &pool_[lIndx];
        if (!llPtr || !lPtr) {
            qWarning() << "Dataset::calcTracingDimensions: !llPtr || !lPtr";
            lastDimRectindx_ = lIndx;
            continue;
        }

        bool published = false;

        if (hasMosaicChannels) {
            NED llNed;
            NED lNed;
            const bool llNedOk = tryGetEpochNed(llPtr, &llNed);
            const bool lNedOk  = tryGetEpochNed(lPtr, &lNed);

            if (llNedOk && lNedOk) {
                const auto llYaw = llPtr->tryRetValidYaw();
                const auto lYaw  = lPtr->tryRetValidYaw();

                if (std::isfinite(llYaw) && std::isfinite(lYaw)) {
                    auto* fChLlCharts = llPtr->chart(mosaicFirstChId_,  mosaicFirstSubChId_);
                    auto* fChlCharts  =  lPtr->chart(mosaicFirstChId_,  mosaicFirstSubChId_);
                    auto* sChLlCharts = llPtr->chart(mosaicSecondChId_, mosaicSecondSubChId_);
                    auto* sChlCharts  =  lPtr->chart(mosaicSecondChId_, mosaicSecondSubChId_);

                    if ((fChLlCharts && fChlCharts) || (sChLlCharts && sChlCharts)) {
                        QVector<QVector3D> traceLinesVertices;
                        traceLinesVertices.reserve(8);

                        const QVector3D llPos(llNed.n, llNed.e, 0.0f);
                        const QVector3D lPos (lNed.n,  lNed.e,  0.0f);

                        const double llAzRad = qDegreesToRadians(llYaw);
                        const double lAzRad  = qDegreesToRadians(lYaw);

                        const double firstAngleOffsetDeg  = lAngleOffset_;
                        const double secondAngleOffsetDeg = rAngleOffset_;

                        if (fChLlCharts && fChlCharts) {
                            const float llRange = fChLlCharts->range();
                            const float lRange  = fChlCharts->range();
                            const double llLeftAzRad = llAzRad - M_PI_2 + qDegreesToRadians(firstAngleOffsetDeg);
                            const double lLeftAzRad  = lAzRad  - M_PI_2 + qDegreesToRadians(firstAngleOffsetDeg);

                            QVector3D llBeg(llPos.x() + llRange * std::cos(llLeftAzRad), llPos.y() + llRange * std::sin(llLeftAzRad), 0.0f); // llPtr f ray
                            QVector3D llEnd(llPos);
                            QVector3D lBeg(lPos.x() + lRange * std::cos(lLeftAzRad), lPos.y() + lRange * std::sin(lLeftAzRad), 0.0f); // lPtr f ray
                            QVector3D lEnd(lPos);

                            traceLinesVertices << llBeg << llEnd << lBeg  << lEnd;
                        }

                        if (sChLlCharts && sChlCharts) {
                            const float llRange = sChLlCharts->range();
                            const float lRange  = sChlCharts->range();

                            const double llRightAzRad = llAzRad + M_PI_2 - qDegreesToRadians(secondAngleOffsetDeg);
                            const double lRightAzRad  = lAzRad  + M_PI_2 - qDegreesToRadians(secondAngleOffsetDeg);

                            QVector3D llBeg(llPos.x() + llRange * std::cos(llRightAzRad), llPos.y() + llRange * std::sin(llRightAzRad), 0.0f); // llPtr s ray
                            QVector3D llEnd(llPos);
                            QVector3D lBeg(lPos.x() + lRange * std::cos(lRightAzRad), lPos.y() + lRange * std::sin(lRightAzRad), 0.0f); // lPtr s ray
                            QVector3D lEnd(lPos);

                            traceLinesVertices << llBeg << llEnd << lBeg  << lEnd;
                        }

                        if (!traceLinesVertices.isEmpty()) {
                            float minN = std::numeric_limits<float>::max();
                            float maxN = std::numeric_limits<float>::lowest();
                            float minE = std::numeric_limits<float>::max();
                            float maxE = std::numeric_limits<float>::lowest();

                            for (auto it = traceLinesVertices.cbegin(); it != traceLinesVertices.cend(); ++it) {
                                minN = std::min(minN, it->x());  // N
                                maxN = std::max(maxN, it->x());
                                minE = std::min(minE, it->y());  // E
                                maxE = std::max(maxE, it->y());
                            }

                            const QRectF currRaysRect(QPointF(minN, minE), QPointF(maxN, maxE));
                            std::array<QPointF, 4> visQuad = {
                                currRaysRect.topLeft(),
                                currRaysRect.topRight(),
                                currRaysRect.bottomRight(),
                                currRaysRect.bottomLeft()
                            };
                            const auto lvl1 = mip->tilesInQuadNed(visQuad, baseZoom, /*padTiles*/0);
                            published = publishTilesForEpoch(llIndx, buildTilesByZoom(lvl1));
                        }
                    }
                }
            }
        }

        if (!published) {
            publishFallbackPointTile(llIndx, llPtr);
        }

        lastDimRectindx_ = lIndx; // store progress
    }
}

void Dataset::appendTileEpochIndex(int epochIndx, const QMap<int, QSet<TileKey>>& tilesByZoom)
{
    QWriteLocker locker(&tileEpochIdxMtx_);

    const int minZoom = 7;
    if (tileEpochIndxsByZoom_.size() < minZoom) {
        tileEpochIndxsByZoom_.resize(minZoom);
    }

    for (auto it = tilesByZoom.cbegin(); it != tilesByZoom.cend(); ++it) {
        const int zoom = it.key() - 1;
        if (zoom < 0 || zoom >= tileEpochIndxsByZoom_.size()) {
            continue;
        }

        auto& indexForZoom = tileEpochIndxsByZoom_[zoom];
        const QSet<TileKey>& tileSet = it.value();
        for (const TileKey& tk : tileSet) {
            auto& epochList = indexForZoom[tk];
            if (epochList.isEmpty() || epochList.back() != epochIndx) {
                epochList.push_back(epochIndx);
            }
        }
    }
}

void Dataset::clearTileEpochIndex()
{
    QWriteLocker locker(&tileEpochIdxMtx_);
    tileEpochIndxsByZoom_.clear();
}

QMap<int, QSet<TileKey>> Dataset::traceTileKeysForEpoch(int epochIndx) const
{
    QReadLocker locker(&poolMtx_);

    if (epochIndx < 0 || epochIndx >= pool_.size()) {
        return {};
    }

    return pool_.at(epochIndx).traceTileIndxs();
}

void Dataset::tryResetDataset(float lat, float lon)
{
    if (!std::isfinite(lat) || !std::isfinite(lon)) {
        return;
    }

    //qDebug() << pos.lla.latitude << pos.lla.longitude <<boatLatitute_ << boatLongitude_;
    const double dist = distanceMetersLLA(lat, lon, boatLatitute_, boatLongitude_);
    if (dist > 1e3) {
        core.onRequestClearing();
    }
}

std::tuple<ChannelId, uint8_t, QString>  Dataset::channelIdFromName(const QString& name) const
{
    auto retVal = std::make_tuple(ChannelId(), 0x00, QString());

    if (name.isEmpty() || channelsNames_.isEmpty() ||
        channelsIds_.size() != channelsNames_.size() ||
        subChannelIds_.size() != channelsNames_.size()) {

        return retVal;
    }

    int index = channelsNames_.indexOf(name);

    if (index >= 0 && index < channelsIds_.size()) {
        return std::make_tuple(channelsIds_[index], subChannelIds_[index], channelsNames_[index]);
    }

    return retVal;
}

void Dataset::setActiveContactIndx(int64_t indx)
{
    activeContactIndx_ = indx;
    emit activeContactChanged();
    emit dataUpdate();
}

int64_t Dataset::getActiveContactIndx() const
{
    return activeContactIndx_;
}

void Dataset::setSpatialIndexingEnabled(bool sonarState, bool dimRectState, bool chunkedCatchup)
{
    if (sonarIndexingEnabled_ == sonarState &&
        dimRectIndexingEnabled_ == dimRectState &&
        chunkedSpatialCatchup_ == chunkedCatchup) {
        return;
    }

    sonarIndexingEnabled_ = sonarState;
    dimRectIndexingEnabled_ = dimRectState;
    chunkedSpatialCatchup_ = chunkedCatchup;

    if (!(sonarIndexingEnabled_ || dimRectIndexingEnabled_)) {
        setSpatialPreparing(false);
        return;
    }

    if (chunkedSpatialCatchup_) {
        scheduleSpatialCatchup();
        return;
    }

    setSpatialPreparing(false);

    if (sonarIndexingEnabled_ && pendingSonarPosIndx_ > sonarPosIndx_) {
        onSonarPosCanCalc(pendingSonarPosIndx_);
    }
    if (dimRectIndexingEnabled_ && pendingDimRectIndx_ > lastDimRectindx_) {
        onDimensionRectCanCalc(pendingDimRectIndx_);
    }
}

void Dataset::scheduleSpatialCatchup()
{
    const bool canRun = chunkedSpatialCatchup_ && (sonarIndexingEnabled_ || dimRectIndexingEnabled_);
    const bool sonarPending = sonarIndexingEnabled_ && (pendingSonarPosIndx_ > sonarPosIndx_);
    const bool dimPending = dimRectIndexingEnabled_ && (std::min(pendingDimRectIndx_, sonarPosIndx_) > lastDimRectindx_);
    const bool hasPending = sonarPending || dimPending;

    setSpatialPreparing(canRun && hasPending);

    if (!canRun || spatialCatchupScheduled_ || !hasPending) {
        return;
    }

    spatialCatchupScheduled_ = true;
    QTimer::singleShot(0, this, [this]() {
        spatialCatchupScheduled_ = false;

        if (sonarIndexingEnabled_ && pendingSonarPosIndx_ > sonarPosIndx_) {
            onSonarPosCanCalc(pendingSonarPosIndx_);
        }
        if (dimRectIndexingEnabled_ && pendingDimRectIndx_ > lastDimRectindx_) {
            onDimensionRectCanCalc(pendingDimRectIndx_);
        }

        const bool sonarPending = sonarIndexingEnabled_ && pendingSonarPosIndx_ > sonarPosIndx_;
        const bool dimPending = dimRectIndexingEnabled_ && (std::min(pendingDimRectIndx_, sonarPosIndx_) > lastDimRectindx_);
        if (sonarPending || dimPending) {
            scheduleSpatialCatchup();
        } else {
            setSpatialPreparing(false);
        }
    });
}

void Dataset::setSpatialPreparing(bool state)
{
    if (spatialPreparing_ == state) {
        return;
    }

    spatialPreparing_ = state;
    emit spatialPreparingChanged();
}

void Dataset::setMosaicChannels(const QString& firstChStr, const QString& secondChStr)
{
    auto [ch1, sub1, name1] = channelIdFromName(firstChStr);
    auto [ch2, sub2, name2] = channelIdFromName(secondChStr);

    bool beenChanged = false;
    if (mosaicFirstChId_     != ch1  ||
        mosaicSecondChId_    != ch2  ||
        mosaicFirstSubChId_  != sub1 ||
        mosaicSecondSubChId_ != sub2) {
        beenChanged = true;
    }

    if (beenChanged) {
        mosaicFirstChId_ = ch1;
        mosaicSecondChId_ = ch2;
        mosaicFirstSubChId_ = sub1;
        mosaicSecondSubChId_ = sub2;

        // TODO: recalc rects on change channels!
    }
}

void Dataset::onSetLAngleOffset(float val)
{
    lAngleOffset_ = val;
}

void Dataset::onSetRAngleOffset(float val)
{
    rAngleOffset_ = val;
}

void Dataset::onSonarPosCanCalc(uint64_t indx)
{
    pendingSonarPosIndx_ = std::max(pendingSonarPosIndx_, indx);

    if (!sonarIndexingEnabled_) {
        return;
    }

    const uint64_t calcTarget = std::max(indx, pendingSonarPosIndx_);
    if (calcTarget <= sonarPosIndx_) {
        return;
    }

    uint64_t chunkTarget = calcTarget;
    if (chunkedSpatialCatchup_) {
        static constexpr uint64_t kSonarChunk = 1024;
        chunkTarget = std::min(calcTarget, sonarPosIndx_ + kSonarChunk);
    }

    for (uint64_t i = sonarPosIndx_ + 1; i <= chunkTarget; ++i) {
        if (auto* ep = fromIndex(i); ep) {
            if (sonarOffset_.isNull()) {
                ep->setSonarPosition(ep->getPositionGNSS()); // interp been before
            }
            else {
                Position boatPos = ep->getPositionGNSS(); // interp been before
                const NED d = fruOffsetToNed(sonarOffset_, ep->tryRetValidYaw());
                NED sonarNed(boatPos.ned.n + d.n, boatPos.ned.e + d.e, /*always zero*/0.0);
                LLA sonarLla(&sonarNed, &_llaRef, /*spherical=*/true);
                boatPos.lla      = sonarLla;
                boatPos.LLA2NED(&_llaRef); // ned
                ep->setSonarPosition(boatPos);
            }

            ep->setSonarPositionDataType(ep->getPositionDataType());
        }
    }

    sonarPosIndx_ = chunkTarget;
    pendingSonarPosIndx_ = sonarPosIndx_;

    if (chunkedSpatialCatchup_ && calcTarget > sonarPosIndx_) {
        pendingSonarPosIndx_ = calcTarget;
        scheduleSpatialCatchup();
    }
}

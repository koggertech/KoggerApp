#include "dataset.h"

#include "core.h"
extern Core core;


Dataset::Dataset() :
    interpolator_(this),
    lastBoatTrackEpoch_(0),
    lastBottomTrackEpoch_(0),
    boatTrackValidPosCounter_(0),
    bSProc_(new BlackStripesProcessor())
{
    resetDataset();
}

Dataset::~Dataset()
{
    delete bSProc_;
}

void Dataset::setState(DatasetState state)
{
    state_ = state;
}

#if defined(FAKE_COORDS)
void Dataset::setActiveZeroing(bool state)
{
    activeZeroing_ = state;
}
#endif

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

QVector<QVector3D> Dataset::boatTrack() const
{
    return _boatTrack;
}

const QHash<int, int>& Dataset::getSelectedIndicesBoatTrack() const
{
    return selectedBoatTrackVertexIndices_;
}

int Dataset::getLastBottomTrackEpoch() const
{
    return lastBottomTrackEpoch_;
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

    pool_[endIndex()].setEvent(timestamp, id, unixt);
    emit dataUpdate();
}

void Dataset::addEncoder(float angle1_deg, float angle2_deg, float angle3_deg) {
    Epoch* last_epoch = last();
    if(last_epoch->isEncodersSeted()) {
        last_epoch = addNewEpoch();
    }

    last_epoch->setEncoders(angle1_deg, angle2_deg, angle3_deg);
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

    int n = std::max(0, size() - (bSProc_->getState() ? bSProc_->getBackwardSteps() : 0)); // TODO: не просто кол-во эпох - окно назад, а последняя неизменная эпоха по чартам
    emit dataUpdate();
    emit chartsUpdated(n);
}

void Dataset::rawDataRecieved(const ChannelId& channelId, RawData raw_data) {
    RawData::RawDataHeader header = raw_data.header;
    ComplexF* compelex_data = (ComplexF*)raw_data.data.data();
    int16_t* real16_data = (int16_t*)raw_data.data.data();
    int size = raw_data.samplesPerChannel();

    Epoch* last_epoch = last();
    ComplexSignals& compex_signals = last_epoch->complexSignals();

    ChannelId dev_id(channelId.uuid, header.channelGroup); // channelId.uuid

    if(compex_signals[dev_id].contains(header.channelGroup)) {
        float offset_m = 0;
        float offset_db = 0;
        offset_db = -20;

        Q_UNUSED(offset_m)
        Q_UNUSED(offset_db)

        // last_epoch->moveComplexToEchogram(offset_m, offset_db);
        last_epoch = addNewEpoch();
        compex_signals = last_epoch->complexSignals();
    }

    QVector<ComplexSignal>& channels = compex_signals[dev_id][header.channelGroup];
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
    emit dataUpdate();
}

void Dataset::addRangefinder(const ChannelId& channelId, float distance)
{
    Epoch* epoch = last();
    if (epoch->distAvail()) {
        epoch = addNewEpoch();
    }

    epoch->setDist(channelId, distance * 1000);

    emit dataUpdate();
}

void Dataset::addUsblSolution(IDBinUsblSolution::UsblSolution data) {
    int pool_index = endIndex();
    if(pool_index < 0 || pool_[pool_index].isUsblSolutionAvailable() == true) {
        addNewEpoch();
        pool_index = endIndex();
    }

    // tracks[data.id].data_.append(QVector3D(data.x_m, data.y_m, data.depth_m));
    tracks[-1].data_.append(QVector3D());
    tracks[-1].objectColor_ = QColor(0, 255, 255);



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

        float beacon_n = data.beacon_n;
        float beacon_e = data.beacon_e;

        if(pos.ned.isCoordinatesValid()) {
            beacon_n += pos.ned.n;
            beacon_e += pos.ned.e;
        }

        tracks[-4].data_.append(QVector3D(beacon_n, beacon_e, 0));
        tracks[-4].objectColor_ = QColor(200, 0, 0);
        tracks[-4].lineWidth_ = 5;


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

void Dataset::addAtt(float yaw, float pitch, float roll) {
    Epoch* last_epoch = last();
    if(last_epoch->isAttAvail()) {
    }

    last_epoch->setAtt(yaw, pitch, roll);
    last_epoch->isAttAvail();

    _lastYaw = yaw;
    _lastPitch = pitch;
    _lastRoll = roll;

#if defined(FAKE_COORDS)
    if (state_ == DatasetState::kConnection && activeZeroing_) {
        ++testTime_;
        double lat = 40.203792, lon = 44.497496;
        Position pos;
        pos.lla = LLA(lat, lon);
        pos.time = DateTime(testTime_, 100);
        if(pos.lla.isCoordinatesValid()) {
            if(last_epoch->getPositionGNSS().lla.isCoordinatesValid()) {
                last_epoch = addNewEpoch();
            }
            setLlaRef(LLARef(pos.lla), getCurrentLlaRefState());
            last_epoch->setPositionLLA(pos);
            last_epoch->setPositionRef(&_llaRef);
            _lastPositionGNSS = last_epoch->getPositionGNSS();
        }
        updateBoatTrack();
    }
#endif

    interpolator_.interpolateYaw(false);
    emit dataUpdate();
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec)
{
    Epoch* last_epoch = last();

    Position pos;
    pos.lla = LLA(lat, lon);
    pos.time = DateTime(unix_time, nanosec);

    if (pos.lla.isCoordinatesValid()) {
        if (last_epoch->getPositionGNSS().lla.isCoordinatesValid()) {
            //qDebug() << "pos add new epoch" << _pool.size();
            last_epoch = addNewEpoch();
        }

        setLlaRef(LLARef(pos.lla), getCurrentLlaRefState());

        last_epoch->setPositionLLA(pos);
        last_epoch->setPositionRef(&_llaRef);

        _lastPositionGNSS = last_epoch->getPositionGNSS();

        last_epoch->setPositionDataType(DataType::kRaw);
    }

    interpolator_.interpolatePos(false);
    emit dataUpdate();
    updateBoatTrack();
}

void Dataset::addPositionRTK(Position position) {
    Epoch* last_epoch = last();
    last_epoch->setExternalPosition(position);
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
    lastTemp_ = temp_c;
    Epoch* last_epoch = last();
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
        Position p_internal = epoch->getPositionGNSS();

        DateTime time_epoch = *epoch->time();
        if(time_epoch.sec > 0) {
            p_internal.time = *epoch->time();
            p_internal.time.sec -= 18;
        }

        int64_t internal_ns  = p_internal.time.sec*1e9+p_internal.time.nanoSec;


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
    {
        QWriteLocker locker(&lock_);
        channelsSetup_.clear();
    }

    pool_.clear();
    _llaRef.isInit = false;
    lastBottomTrackEpoch_ = 0;
    resetDistProcessing();
    interpolator_.clear();
    llaRefState_ = LlaRefState::kUndefined;
    state_ = DatasetState::kUndefined;
    clearBoatTrack();

#if defined(FAKE_COORDS)
    testTime_ = 1740466541;
#endif

    usingRecordParameters_.clear();
    //bSProc_->clear();
    lastAddChartEpochIndx_.clear();
    channelsToResizeEthData_.clear();

    emit channelsUpdated();
    emit dataUpdate();
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
    for (const auto& channel : ch_list) {
        ChannelId ich = channel.channelId_;

        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }

            Position ext_pos = epoch->getExternalPosition();

            if(epoch->chartAvail(ich)) {
                Epoch::Echogram* data = epoch->chart(ich);

                if(data == NULL) { continue; }

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += channel.localPosition_.z;
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= channel.localPosition_.z;
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
        Position pos = epoch->getPositionGNSS();

        // if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
        //     if(!_llaRef.isInit) {
        //         _llaRef = LLARef(pos.lla);
        //     }
        //     pos.LLA2NED(&_llaRef);
        // }

        if(pos.ned.isCoordinatesValid() && epoch->isAttAvail() && epoch->isUsblSolutionAvailable()) {
            double n = pos.ned.n, e = pos.ned.e;
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

void Dataset::clearBoatTrack() {
    lastBoatTrackEpoch_ = 0;
    _boatTrack.clear();
    selectedBoatTrackVertexIndices_.clear();
    boatTrackValidPosCounter_ = 0;
    emit dataUpdate();
}

void Dataset::updateBoatTrack(bool updateAll) {
    const int toSize = size();
    int fromIndx = 0;

    if (fromIndx > toSize) {
        return;
    }

    if (updateAll) {
        _boatTrack.clear();
        selectedBoatTrackVertexIndices_.clear();
        boatTrackValidPosCounter_ = 0;
    }
    else {
        fromIndx = lastBoatTrackEpoch_;
    }


    for (int i = fromIndx; i < toSize; ++i) {
        auto* ep = fromIndex(i);
        if (!ep) {
            continue;
        }

        auto pos = ep->getPositionGNSS().ned;

        bool appended = true;
        if (pos.isCoordinatesValid()) {
            _boatTrack.append(QVector3D(pos.n, pos.e, 0));
            //qDebug() << "add pos" << i << pos.n << pos.e << "type" << static_cast<int>(ep->getPositionDataType());
        }
        else {
            appended = false;
            //qDebug() << "fail add" << i << pos.n << pos.e << "type" << static_cast<int>(ep->getPositionDataType());
        }

        if (appended) {
            selectedBoatTrackVertexIndices_.insert(boatTrackValidPosCounter_, i);
            ++boatTrackValidPosCounter_;
        }
    }

    lastBoatTrackEpoch_ = toSize;

    emit boatTrackUpdated();
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

void Dataset::interpolateData(bool fromStart)
{
    interpolator_.interpolatePos(fromStart);
    interpolator_.interpolateYaw(fromStart);
}

void Dataset::onDistCompleted(int epIndx, const ChannelId& channelId, float dist)
{
    Epoch* epPtr = fromIndex(epIndx);

    if (!epPtr) {
        return;
    }

    if (epPtr->chartAvail(channelId)) {
        Epoch::Echogram* chart = epPtr->chart(channelId);
        if (chart) {
            chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
        }
    }
}

void Dataset::onLastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP)
{
    bottomTrackParam_ = btP;
    lastBottomTrackEpoch_ = val;

    emit dataUpdate();
    emit bottomTrackUpdated(channelId, bottomTrackParam_.indexFrom, bottomTrackParam_.indexTo);
}

void Dataset::validateChannelList(const ChannelId &channelId, uint8_t subChannelId)
{
    int16_t indx = -1;

    {
        QWriteLocker locker(&lock_);

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
    pool_.resize(pool_.size() + 1);
    auto* lastEpoch = last();
    return lastEpoch;
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

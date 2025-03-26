#include "plotcash.h"
#include <QPainterPath>

#include <core.h>
extern Core core;

Epoch::Epoch() : wasValidlyRenderedInEchogram_(false) {
    _charts.clear();
    flags.distAvail = false;
}

void Epoch::setEvent(int timestamp, int id, int unixt) {
    _eventTimestamp_us = timestamp;
    _eventUnix = unixt;
    _eventId = id;
    _time = DateTime(unixt, timestamp*1000);
    flags.eventAvail = true;
}

void Epoch::setChart(int16_t channel, QVector<uint8_t> data, float resolution, float offset) {
    _charts[channel].amplitude = data;
    _charts[channel].resolution = resolution;
    _charts[channel].offset = offset;
    _charts[channel].type = 1;
}

void Epoch::setRecParameters(int16_t channel, RecordParameters recParams)
{
    if (_charts.contains(channel)) {
        _charts[channel].recordParameters_ = recParams;
    }
}

// void Epoch::setComplexSignal16(int channel, QVector<Complex16> data) {
//     // _complex[channel].data = QByteArray((const char*)data.constData(), data.size()*sizeof(Complex16));
//     // _complex[channel].type = 2;
// }

void Epoch::setComplexF(int channel, ComplexSignal signal) {
    _complex[channel] = signal;
}
void Epoch::setDist(int dist) {
    _rangeFinders[0] = dist*0.001;
    flags.distAvail = true;
}

void Epoch::setRangefinder(int channel, float distance) {
    _rangeFinders[channel] = distance;
}

void Epoch::setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    for(uint16_t i = 0; i < cnt; i++) {
        _dopplerBeams[i] = beams[i];
    }
    //    setDist(beams[0].distance*1000.0f);
    _dopplerBeamCount = cnt;
}

void Epoch::setDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    _dvlSolution = dvlSolution;
    flags.isDVLSolutionAvail = true;
}

void Epoch::setPositionLLA(double lat, double lon, LLARef* ref, uint32_t unix_time, int32_t nanosec) {
    Q_UNUSED(ref);

    _positionGNSS.time = DateTime(unix_time, nanosec);
    _positionGNSS.lla.latitude = lat;
    _positionGNSS.lla.longitude = lon;

    flags.posAvail = true;
}

void Epoch::setPositionLLA(Position position) {
    _positionGNSS = position;
    flags.posAvail = true;
}

void Epoch::setExternalPosition(Position position) {
    _positionExternal = position;
}

void Epoch::setPositionRef(LLARef* ref) {
    if(ref != NULL && ref->isInit) {
        _positionGNSS.LLA2NED(ref);
    }
}

void Epoch::setGnssVelocity(double h_speed, double course) {
    _GnssData.hspeed = h_speed;
    _GnssData.course = course;
}

void Epoch::setTime(DateTime time) {
    Q_UNUSED(time);
}

void Epoch::setTime(int year, int month, int day, int hour, int min, int sec, int nanosec) {
    Q_UNUSED(year);
    Q_UNUSED(month);
    Q_UNUSED(day);
    Q_UNUSED(hour);
    Q_UNUSED(min);
    Q_UNUSED(sec);
    Q_UNUSED(nanosec);
}


void Epoch::setTemp(float temp_c) {
    m_temp_c = temp_c;
    flags.tempAvail = true;
}

void Epoch::setEncoders(float enc1, float enc2, float enc3) {
    _encoder.e1 = enc1;
    _encoder.e2 = enc2;
    _encoder.e3 = enc3;
}

void Epoch::setAtt(float yaw, float pitch, float roll) {
    _attitude.yaw = yaw;
    _attitude.pitch = pitch;
    _attitude.roll = roll;
}

void Epoch::setGNSSSec(time_t sec)
{
    _positionGNSS.time.sec = sec;
}

void Epoch::setGNSSNanoSec(int nanoSec)
{
    _positionGNSS.time.nanoSec = nanoSec;
}

void Epoch::doBottomTrack2D(Echogram &chart, bool is_update_dist) {
    Q_UNUSED(chart);
    Q_UNUSED(is_update_dist);
}

void Epoch::doBottomTrackSideScan(Echogram &chart, bool is_update_dist) {
    Q_UNUSED(chart);
    Q_UNUSED(is_update_dist);
}

void Epoch::moveComplexToEchogram(float offset_m, float levels_offset_db) {
    for (auto i = _complex.cbegin(), end = _complex.cend(); i != end; ++i) {
        // cout << qPrintable(i.key()) << ": " << i.value() << endl;
        QVector<ComplexF> data = i.value().data;


        int size = data.size();
        ComplexF* compelex_data = data.data();

        QVector<uint8_t> chart(size);
        uint8_t* chart_data = chart.data();

        for(int k  = 0; k < size; k++) {
            float amp = (compelex_data[k].logPow() + levels_offset_db)*2.5;

            if(amp < 0) {
                amp = 0;
            } else if(amp > 255) {
                amp = 255;
            }
            chart_data[k] = amp;
        }

        setChart(i.key(), chart, 1500.0f/i.value().sampleRate, offset_m);
    }
}

void Epoch::setInterpNED(NED ned)
{
    interpData_.ned = ned;
}

void Epoch::setInterpYaw(float yaw)
{
    interpData_.yaw = yaw;
}

void Epoch::setInterpFirstChannelDist(float dist)
{
    interpData_.distFirstChannel = dist;
}

void Epoch::setInterpSecondChannelDist(float dist)
{
    interpData_.distSecondChannel = dist;
}

NED Epoch::getInterpNED() const
{
    return interpData_.ned;
}

float Epoch::getInterpYaw() const
{
    return interpData_.yaw;
}

float Epoch::getInterpFirstChannelDist() const
{
    return interpData_.distFirstChannel;
}

float Epoch::getInterpSecondChannelDist() const
{
    return interpData_.distSecondChannel;
}

bool Epoch::getWasValidlyRenderedInEchogram() const
{
    return wasValidlyRenderedInEchogram_;
}

void Epoch::setWasValidlyRenderedInEchogram(bool state)
{
    wasValidlyRenderedInEchogram_ = state;
}

void Epoch::setResolution(int16_t channelId, uint16_t resolution)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.resol = resolution;
    }
}

void Epoch::setChartCount(int16_t channelId, uint16_t chartCount)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.count = chartCount;
    }
}

void Epoch::setOffset(int16_t channelId, uint16_t offset)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.offset = offset;
    }}

void Epoch::setFrequency(int16_t channelId, uint16_t frequency)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.freq = frequency;
    }
}

void Epoch::setPulse(int16_t channelId, uint8_t pulse)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.pulse = pulse;
    }
}

void Epoch::setBoost(int16_t channelId, uint8_t boost)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.boost = boost;
    }
}

void Epoch::setSoundSpeed(int16_t channelId, uint32_t soundSpeed)
{
    if (_charts.contains(channelId)) {
        _charts[channelId].recordParameters_.soundSpeed = soundSpeed;
    }
}

uint16_t Epoch::getResolution(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.resol;
    }
    return 0;
}

uint16_t Epoch::getChartCount(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.count;
    }
    return 0;
}

uint16_t Epoch::getOffset(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.offset;
    }
    return 0;
}

uint16_t Epoch::getFrequency(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.freq;
    }
    return 0;
}

uint8_t Epoch::getPulse(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.pulse;
    }
    return 0;
}

uint8_t Epoch::getBoost(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.boost;
    }
    return 0;}

uint32_t Epoch::getSoundSpeed(int16_t channelId) const
{
    if (auto echogarm = _charts.find(channelId); echogarm != _charts.end()) {
        return echogarm.value().recordParameters_.soundSpeed;
    }
    return 0;
}

Dataset::Dataset() :
    interpolator_(this),
    lastBoatTrackEpoch_(0),
    lastBottomTrackEpoch_(0),
    boatTrackValidPosCounter_(0),
    fixBlackStripesState_(false),
    fixBlackStripesWrCnt_(5)
{
    resetDataset();
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

void Dataset::getMaxDistanceRange(float *from, float *to, int channel1, int channel2) {
    const int sz = size();
    float channel1_max = 0;
    float channel2_max = 0;
    for(int iepoch = 0; iepoch < sz; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch != NULL) {
            if(epoch->chartAvail(channel1)) {
                float range = epoch->chart(channel1)->range();
                if(channel1_max < range) {
                    channel1_max = range;
                }
            }

            if(epoch->chartAvail(channel2)) {
                float range = epoch->chart(channel2)->range();
                if(channel2_max < range) {
                    channel2_max = range;
                }
            }
        }
    }

    if(channel1_max > 0) {
        if(channel2_max > 0) {
            *from = -channel1_max;
            *to = channel2_max;
        } else {
            *from = 0;
            *to = channel1_max;
        }

    } else {
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

    _pool[endIndex()].setEvent(timestamp, id, unixt);
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

void Dataset::setTranscSetup(int16_t channel, uint16_t freq, uint8_t pulse, uint8_t boost)
{
    usingRecordParameters_[channel].freq  = freq;
    usingRecordParameters_[channel].pulse = pulse;
    usingRecordParameters_[channel].boost = boost;
}

void Dataset::setSoundSpeed(int16_t channel, uint32_t soundSpeed)
{
    usingRecordParameters_[channel].soundSpeed  = soundSpeed;
}

void Dataset::setChartSetup(int16_t channel, uint16_t resol, uint16_t count, uint16_t offset)
{
    usingRecordParameters_[channel].resol  = resol;
    usingRecordParameters_[channel].count  = count;
    usingRecordParameters_[channel].offset = offset;

    if (ethData_.contains(channel)) {
        if (count < ethData_[channel].size()) {
            ethData_[channel].resize(count);
        }
    }
}

void Dataset::setFixBlackStripesState(bool state)
{
    fixBlackStripesState_ = state;
}

void Dataset::setFixBlackStripesRange(int val)
{
    fixBlackStripesWrCnt_ = val;
}

void Dataset::addChart(ChartParameters chartParams, QVector<uint8_t> data, float resolution, float offset)
{
    if (data.empty() || qFuzzyIsNull(resolution)) {
        return;
    }

    if (endIndex() < 0 ||
        _pool[endIndex()].chartAvail(chartParams.channelId)) {
        addNewEpoch();
    }

    const int dataSize = data.size();
    const int endIndx = endIndex();
    const auto& address = chartParams.address;
    const auto& channelId = chartParams.channelId;

    RecordParameters recParam;
    if (usingRecordParameters_.contains(address)) {
        recParam = usingRecordParameters_[address];
    }

    auto setChart = [&]() -> void {
        _pool[endIndx].setChart(channelId, data, resolution, offset);
        _pool[endIndx].setRecParameters(channelId, recParam);
        _pool[endIndx].setWasValidlyRenderedInEchogram(false);
    };    

    if (!fixBlackStripesState_) {
        setChart();
        validateChannelList(channelId);
        emit dataUpdate();
        return;
    }

    // fix black stripes
    auto& ethVec = ethData_[channelId];
    const auto& errList = chartParams.errList;

    QVector<uint8_t> errorMask(dataSize, 0);
    for (const auto& seg : errList) {
        const int start = std::max(static_cast<int>(seg.first), 0);
        const int end   = std::min(static_cast<int>(seg.second), dataSize);
        for (int i = start; i < end; ++i) {
            errorMask[i] = 1;
        }
    }

    int lastValidEthIndx = -1;
    for (int i = ethVec.size() - 1; i >= 0; --i) {
        if (ethVec.at(i).first) {
            lastValidEthIndx = i;
            break;
        }
    }

    int newDataSize = dataSize;
    if (dataSize < ethVec.size()) {
        if (lastValidEthIndx != -1) {
            newDataSize = std::max(dataSize, lastValidEthIndx + 1);
        }
    }

    if (ethVec.size() < newDataSize) {
        ethVec.resize(newDataSize);
    }
    if (data.size() < newDataSize) {
        data.resize(newDataSize);
    }

    for (int i = 0; i < newDataSize; ++i) {
        bool outOfOrigDataRange = (i >= dataSize);
        bool inErrorMask = (i < errorMask.size() && errorMask[i] != 0);

        if (outOfOrigDataRange || inErrorMask) {
            if (ethVec[i].first) {
                data[i] = ethVec[i].second;
                --ethVec[i].first;
            }
        }
        else {
            ethVec[i] = qMakePair(fixBlackStripesWrCnt_, data[i]);
        }
    }

    setChart();
    validateChannelList(channelId);
    emit dataUpdate();
}

void Dataset::rawDataRecieved(RawData raw_data) {
    Epoch* last_epoch = last();

    RawData::RawDataHeader header = raw_data.header;

    ComplexF* compelex_data = (ComplexF*)raw_data.data.data();
    int16_t* real16_data = (int16_t*)raw_data.data.data();

    int size = raw_data.data.size()/(header.dataSize + 1)/header.channelCount;

    if(header.localOffset == 0) {
        float offset_m = 0;
        // if(last_epoch->isUsblSolutionAvailable()) {
        //     offset_m = last_epoch->usblSolution().distance_m;
        //     offset_m -= (last_epoch->usblSolution().carrier_counter - header.globalOffset)*1500.0f/header.sampleRate;
        // }
        float offset_db = 0;
        offset_db = -86;

        last_epoch->moveComplexToEchogram(offset_m, offset_db);

        if(header.channelGroup == 0) {
            last_epoch = addNewEpoch();
        }

        ComplexSignals compex_signals = last_epoch->complexSignals();

        for(int ich = 0; ich < header.channelCount; ich++) {
            int ch_num = ich + (header.channelGroup*32);
            ComplexSignal signal = compex_signals[ch_num];
            signal.groupIndex = header.channelGroup;

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

            last_epoch->setComplexF(ch_num, signal);
            validateChannelList(ch_num);
        }
    } else {
        ComplexSignals compex_signals = last_epoch->complexSignals();

        for(int ich = 0; ich < header.channelCount; ich++) {
            int ch_num = ich + (header.channelGroup*32);

            ComplexSignal signal = compex_signals[ch_num];
            uint32_t inbuf_localOffset = signal.data.size();
            signal.data.resize(header.localOffset + size);

            // signal.data.fill(ComplexF(0,0))

            if(inbuf_localOffset == header.localOffset) {
                ComplexF* signal_data = signal.data.data() + inbuf_localOffset;
                if(header.dataType == 0) {
                    for(int i  = 0; i < size; i++) {
                        signal_data[i] = compelex_data[i*header.channelCount + ich];
                    }
                } else if(header.dataType == 1) {
                    for(int i  = 0; i < size; i++) {
                        signal_data[i] = ComplexF(real16_data[i*header.channelCount + ich], 0);
                    }
                }
            } else {
                qDebug("raw data has broken");
            }


            last_epoch->setComplexF(ch_num, signal);
        }
    }

    emit dataUpdate();
}

void Dataset::addDist(int dist) {
    int pool_index = endIndex();
    if(pool_index < 0 || _pool[pool_index].distAvail() == true) {
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDist(dist);
    emit dataUpdate();
}

void Dataset::addRangefinder(float distance) {
    Epoch* epoch = last();
    if(epoch->distAvail()) {
        epoch = addNewEpoch();
    }

    epoch->setDist(distance*1000);
    emit dataUpdate();
}

void Dataset::addUsblSolution(IDBinUsblSolution::UsblSolution data) {
    int pool_index = endIndex();
    if(pool_index < 0 || _pool[pool_index].isUsblSolutionAvailable() == true) {
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

    _pool[endIndex()].setAtt(data.usbl_yaw, data.usbl_pitch, data.usbl_roll);
    _pool[endIndex()].set(data);
    emit dataUpdate();
}

void Dataset::addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == true)) { //
        addNewEpoch();
    }

    pool_index = endIndex();

    _pool[endIndex()].setDopplerBeam(beams, cnt);
    emit dataUpdate();
}

void Dataset::addDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == false)) { //
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDVLSolution(dvlSolution);
    emit dataUpdate();
}

void Dataset::addAtt(float yaw, float pitch, float roll) {
    Epoch* last_epoch = last();
    if(last_epoch->isAttAvail()) {
    }

    last_epoch->setAtt(yaw, pitch, roll);

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

    emit dataUpdate();
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec) {
    Epoch* last_epoch = last();

    Position pos;
    pos.lla = LLA(lat, lon);
    pos.time = DateTime(unix_time, nanosec);

    if(pos.lla.isCoordinatesValid()) {
        if(last_epoch->getPositionGNSS().lla.isCoordinatesValid()) {
            last_epoch = addNewEpoch();
        }

        setLlaRef(LLARef(pos.lla), getCurrentLlaRefState());

        last_epoch->setPositionLLA(pos);
        last_epoch->setPositionRef(&_llaRef);
        _lastPositionGNSS = last_epoch->getPositionGNSS();
    }

    emit dataUpdate();
    updateBoatTrack();
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


    _pool[pool_index].setGnssVelocity(h_speed, course);
    emit dataUpdate();
}

void Dataset::addTemp(float temp_c) {

    lastTemperature = temp_c;

    int pool_index = endIndex();
    if(pool_index < 0) {
        addNewEpoch();
        pool_index = endIndex();
    }
    _pool[pool_index].setTemp(temp_c);
    emit dataUpdate();
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


void Dataset::resetDataset() {
    _pool.clear();
    _llaRef.isInit = false;
    _channelsSetup.clear();
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
    ethData_.clear();

    emit dataUpdate();
}

void Dataset::resetDistProcessing() {
    int pool_size = size();
    for(int i = 0; i < pool_size; i++) {
//        Epoch* dataset = fromIndex(i);
//        dataset->resetDistProccesing();
    }
}

void Dataset::bottomTrackProcessing(int channel1, int channel2)
{
    if(bottomTrackParam_.indexFrom < 0 || bottomTrackParam_.indexTo < 0) { return; }

    int epoch_min_index = bottomTrackParam_.indexFrom - bottomTrackParam_.windowSize/2;

    if(epoch_min_index < 0) {
        epoch_min_index = 0;
    }

    int epoch_max_index = bottomTrackParam_.indexTo + bottomTrackParam_.windowSize/2;
    if(epoch_max_index >= size()) {
        epoch_max_index = size();
    }

    QVector<int32_t> summ;

    float gain_slope = bottomTrackParam_.gainSlope;
    float threshold = bottomTrackParam_.threshold;

    int istart = 4;
    int init_win = 6;
    int scale_win = 20;

    int16_t c1 = -6, c2 = 6, c3 = 4, c4 = 2, c5 = 0;
    float s2 = 1.04f, s3 = 1.06f, s4 = 1.10f, s5 = 1.15f;
    float t1 = 1.07;



    if(bottomTrackParam_.preset == BottomTrackPreset::BottomTrackOneBeamNarrow) {
        istart = 4;
        init_win = 6;
        scale_win = 35;

        c1 = -3, c2 = 8, c3 = 5, c4 = -1, c5 = -1;
        s2 = 1.015f, s3 = 1.035f, s4 = 1.08f, s5 = 1.12f;
        t1 = 1.04;
    }


    if(bottomTrackParam_.preset == BottomTrackPreset::BottomTrackSideScan) {
        istart = 4;
        init_win = 6;
        scale_win = 12;

        c1 = -3, c2 = -3, c3 = 4, c4 = 2, c5 = 1;
        s2 = 1.1f, s3 = 1.2f, s4 = 1.3f, s5 = 1.4f;
        t1 = 1.2;
    }

    const int gain_slope_inv = 1000/(gain_slope);
    const int threshold_int = 10*gain_slope_inv*1000*threshold;

    typedef  struct {
        float min = NAN, max = NAN;
    } EpochConstrants;

    QVector<QVector<int32_t>> cash(bottomTrackParam_.windowSize);
    QVector<EpochConstrants> constr(bottomTrackParam_.windowSize);

    QVector<float> bottom_track(epoch_max_index - epoch_min_index);
    bottom_track.fill(NAN);

    int epoch_counter = 0;

    for(int iepoch = epoch_min_index; iepoch < epoch_max_index; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch == NULL || !epoch->chartAvail(channel1)) { continue; }

        epoch_counter++;

        Epoch::Echogram* chart = epoch->chart(channel1);

        uint8_t* data = (uint8_t*)chart->amplitude.constData();
        const int data_size = chart->amplitude.size();

        int cash_ind = (epoch_counter-1)%bottomTrackParam_.windowSize;

        int back_cash_ind = ((epoch_counter)%bottomTrackParam_.windowSize);
        int32_t* back_cash_data = (int32_t*)cash[back_cash_ind].constData();
        const int back_cash_size = cash[back_cash_ind].size();

        int32_t* summ_data = (int32_t*)summ.constData();

        if(epoch_counter >= bottomTrackParam_.windowSize) {
            for(int i = istart; i < back_cash_size; i++) { summ_data[i] -= back_cash_data[i]; }
        }

        if(cash[cash_ind].size() != data_size) {
            cash[cash_ind].resize(data_size);
        }

        int32_t* cash_data = (int32_t*)cash[cash_ind].constData();


        uint8_t* data_from = &data[istart];
        uint8_t* data_to = &data[(istart+init_win)];

        int data_acc = 0;
        for(int idata = istart; idata < (istart+init_win); idata++) {
            data_acc += data[idata];
        }

        int avrg_range = init_win;
        for(int idata = istart; (idata + avrg_range) < data_size; idata++) {
            data_acc -= *data_from; data_from++;
            data_acc += *data_to; data_to++;
            while((idata+(init_win*scale_win)) >= ((avrg_range = data_to - data_from)*scale_win)) {
                data_acc += *data_to; data_to++;
            }
            cash_data[idata] = 10*data_acc / (avrg_range);
        }

        const int data_conv_size = data_size - avrg_range;
        for(int idata = istart; ; idata++) {
            const float fidata = idata + init_win+1;
            int di1 =  idata;
            int di2 =  fidata*s2;
            int di3 =  fidata*s3;
            int di4 =  fidata*s4;
            int di5 =  fidata*s5;

            if(di5 >= data_conv_size) { break; }

            int calc = (c1*cash_data[di1] + c2*cash_data[di2] + c3*cash_data[di3] + c4*cash_data[di4] + c5*cash_data[di5]);
            calc = calc*gain_slope_inv + calc*(idata);

            cash_data[idata] = calc;
        }

        const int col_size = cash[cash_ind].size();
        if(summ.size() < col_size) { summ.resize(col_size); }
        summ_data = (int32_t*)summ.constData();
        for(int i = istart; i < col_size; i++) { summ_data[i] += cash_data[i]; }


        constr[cash_ind].min = chart->bottomProcessing.getMin();
        constr[cash_ind].max = chart->bottomProcessing.getMax();

        const int win_center_index = (epoch_counter - 1 + bottomTrackParam_.windowSize/2)%bottomTrackParam_.windowSize;

        float search_from_distance = bottomTrackParam_.minDistance;
        float search_to_distance = bottomTrackParam_.maxDistance;

        if(search_from_distance < constr[win_center_index].min) {
            search_from_distance = constr[win_center_index].min;
        }

        if(search_to_distance > constr[win_center_index].max) {
            search_to_distance = constr[win_center_index].max;
        }

        int start_search_index = search_from_distance/t1/chart->resolution;
        int end_search_index = search_to_distance/t1/chart->resolution;

        if(start_search_index < 0) { start_search_index = 0; }
        if(start_search_index > summ.size()) { start_search_index = summ.size(); }

        if(end_search_index < 0) { end_search_index = 0; }
        if(end_search_index > summ.size()) { end_search_index = summ.size(); }


        int max_val = threshold_int*bottomTrackParam_.windowSize;
        int max_ind = -1;
        for(int i = start_search_index; i < end_search_index ; i++) {
            if(max_val < summ_data[i]) {
                max_val = summ_data[i];
                max_ind = i;
            }
        }

        if(max_ind > 0) {
            float distance = ((max_ind+init_win+1)*t1)*chart->resolution;

            if(epoch_counter >= bottomTrackParam_.windowSize) {
                if(bottomTrackParam_.verticalGap > 0) {
                    int32_t* center_cash_data = (int32_t*)cash[win_center_index].constData();
                    const int center_cash_size = cash[win_center_index].size();

                    int start_gap_index = max_ind*(1.0f-bottomTrackParam_.verticalGap);
                    int end_gap_index = max_ind*(1.0f+bottomTrackParam_.verticalGap);

                    if(start_gap_index < start_search_index) { start_gap_index = start_search_index; }
                    if(start_gap_index > center_cash_size) { start_gap_index = center_cash_size; }

                    if(end_gap_index < 0) { end_gap_index = 0; }
                    if(end_gap_index > center_cash_size) { end_gap_index = center_cash_size; }
                    if(end_gap_index > end_search_index) { end_gap_index = end_search_index; }

                    int max_gap_val = 0;
                    int max_gap_ind = -1;
                    for(int i = start_gap_index; i < end_gap_index ; i++) {
                        if(max_gap_val < center_cash_data[i]) {
                            max_gap_val = center_cash_data[i];
                            max_gap_ind = i;
                        }
                    }


                    if(max_gap_ind > 0) {
                        distance = ((max_gap_ind+init_win+1)*t1)*chart->resolution;
                    }
                }

                bottom_track[iepoch - epoch_min_index - bottomTrackParam_.windowSize/2] = distance;
            } else {
                bottom_track[iepoch - epoch_min_index - epoch_counter/2] = distance;
            }
            }
    }


    int epoch_start_index = bottomTrackParam_.indexFrom;

    if(epoch_start_index < 0) {
        epoch_start_index = 0;
    }

    int epoch_stop_index = bottomTrackParam_.indexTo;
    if(epoch_stop_index >= size()) {
        epoch_stop_index = size();
    }

    for(int iepoch = epoch_start_index; iepoch < epoch_stop_index; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);

        if(epoch == NULL) { continue; }

        if(epoch->chartAvail(channel1)) {
            Epoch::Echogram* chart = epoch->chart(channel1);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }

        if(epoch->chartAvail(channel2)) {
            Epoch::Echogram* chart = epoch->chart(channel2);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }
    }

    setChannelOffset(channel1, bottomTrackParam_.offset.x, bottomTrackParam_.offset.y, bottomTrackParam_.offset.z);
    spatialProcessing();
    emit dataUpdate();
    lastBottomTrackEpoch_ = size();
    emit bottomTrackUpdated(epoch_min_index, epoch_max_index);
}

void Dataset::spatialProcessing() {
    QList<DatasetChannel> ch_list = channelsList().values();
    for (const auto& channel : ch_list) {
        int ich = channel.channel;

        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }

            Position ext_pos = epoch->getExternalPosition();

            if(epoch->chartAvail(ich)) {
                Epoch::Echogram* data = epoch->chart(ich);

                if(data == NULL) { continue; }

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += channel.localPosition.z;
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= channel.localPosition.z;
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

    emit dataUpdate();
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

    emitPositionsUpdated();
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

void Dataset::updateBoatTrack(bool update_all) {
    const int to_size = size();
    int from_index = 0;

    if(update_all) {
        _boatTrack.clear();
        selectedBoatTrackVertexIndices_.clear();
        boatTrackValidPosCounter_ = 0;
    } else {
        from_index = lastBoatTrackEpoch_;
    }

    QMap<int, DatasetChannel> ch_list = channelsList();

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* epoch = fromIndex(i);
        Position pos = epoch->getPositionGNSS();

        // if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
        //     if(!_llaRef.isInit) {
        //         _llaRef = LLARef(pos.lla);
        //     }
        //     pos.LLA2NED(&_llaRef);
        // }

        if(pos.ned.isCoordinatesValid()) {
            _boatTrack.append(QVector3D(pos.ned.n,pos.ned.e, 0));
            selectedBoatTrackVertexIndices_.insert(boatTrackValidPosCounter_++, i);
        }
    }

    lastBoatTrackEpoch_ = to_size;

    emit boatTrackUpdated();
}

QStringList Dataset::channelsNameList() {
    QStringList ch_names;
    QList<DatasetChannel> ch_list = channelsList().values();
    ch_names.append(QString(tr("None")));
    ch_names.append(QString(tr("First")));
    for (const auto& channel : ch_list) {
        ch_names.append(QString("%1").arg(channel.channel));
    }
    return ch_names;

}

void Dataset::interpolateData(bool fromStart)
{
    interpolator_.interpolateData(fromStart);
}

Dataset::Interpolator::Interpolator(Dataset *datasetPtr) :
    datasetPtr_(datasetPtr),
    lastInterpIndx_(0),
    firstChannelId_(CHANNEL_NONE),
    secondChannelId_(CHANNEL_FIRST)
{ }

void Dataset::Interpolator::interpolateData(bool fromStart)
{
    if (!updateChannelsIds()) {
        return;
    }

    int shift{ datasetPtr_->getBottomTrackParamPtr()->windowSize };
    int startEpochIndx{ fromStart ? 0 : lastInterpIndx_ };
    int endEpochIndx = datasetPtr_->size() - 1 - shift;
    if ((endEpochIndx - startEpochIndx) < 2) {
        return;
    }

    bool somethingInterp{ false };
    int firstValidIndex{ startEpochIndx };

    while (firstValidIndex < endEpochIndx) {
        while (firstValidIndex <= endEpochIndx) {
            auto* fEp = datasetPtr_->fromIndex(firstValidIndex);
            if (fEp->getPositionGNSS().ned.isCoordinatesValid() && isfinite(fEp->yaw()) &&
                (isfinite(fEp->distProccesing(firstChannelId_)) ||
                isfinite(fEp->distProccesing(secondChannelId_))) ) {
                break;
            }
            ++firstValidIndex;
        }
        int secondValidIndex = firstValidIndex + 1;
        while (secondValidIndex <= endEpochIndx) {
            auto* sEp = datasetPtr_->fromIndex(secondValidIndex);
            if (sEp->getPositionGNSS().ned.isCoordinatesValid() && isfinite(sEp->yaw()) &&
                (isfinite(sEp->distProccesing(firstChannelId_)) ||
                isfinite(sEp->distProccesing(secondChannelId_))) ) {
                break;
            }
            ++secondValidIndex;
        }
        if (secondValidIndex > endEpochIndx) {
            break;
        }

        int fromIndx = firstValidIndex + 1;
        int toIndx = secondValidIndex;
        int numInterpIndx = toIndx - firstValidIndex;
        if (numInterpIndx <= 0) {
            firstValidIndex = secondValidIndex;
            continue;
        }

        auto* startEpoch = datasetPtr_->fromIndex(firstValidIndex);
        auto* endEpoch = datasetPtr_->fromIndex(secondValidIndex);
        auto startPos = startEpoch->getPositionGNSS();
        auto endPos = endEpoch->getPositionGNSS();
        auto startYaw = startEpoch->yaw();
        auto endYaw = endEpoch->yaw();
        auto startFirstChannelDist = startEpoch->distProccesing(firstChannelId_);
        auto endFirstChannelDist = endEpoch->distProccesing(firstChannelId_);
        auto startSecondChannelDist = startEpoch->distProccesing(secondChannelId_);
        auto endSecondChannelDist = endEpoch->distProccesing(secondChannelId_);
        auto timeDiffNano = calcTimeDiffInNanoSecs(startEpoch->getPositionGNSS().time.sec,
                                                   startEpoch->getPositionGNSS().time.nanoSec,
                                                   endEpoch->getPositionGNSS().time.sec,
                                                   endEpoch->getPositionGNSS().time.nanoSec);
        auto timeOnStep = static_cast<quint64>(timeDiffNano * 1.0f / static_cast<float>(numInterpIndx));

        // time
        int cnt{ 1 };
        auto startTime = convertToNanosecs(startEpoch->getPositionGNSS().time.sec, startEpoch->getPositionGNSS().time.nanoSec);
        for (int j = fromIndx; j < toIndx; ++j) {
            auto pTime = convertFromNanosecs(startTime + cnt++ * timeOnStep);
            auto* interpEpoch = datasetPtr_->fromIndex(j);
            interpEpoch->setGNSSSec(pTime.first);
            interpEpoch->setGNSSNanoSec(pTime.second);
        }
        // data
        for (int j = fromIndx; j < toIndx; ++j) {
            auto* interpEpoch = datasetPtr_->fromIndex(j);
            auto currentTime = convertToNanosecs(interpEpoch->getPositionGNSS().time.sec, interpEpoch->getPositionGNSS().time.nanoSec);
            float progress = (currentTime - startTime) * 1.0f / static_cast<float>(timeDiffNano);
            interpEpoch->setInterpNED(interpNED(startPos.ned, endPos.ned, progress));
            interpEpoch->setInterpYaw(interpYaw(startYaw, endYaw, progress));

            float correctDist = 0.0f; //
            if (correctDist = interpDist(startFirstChannelDist, endFirstChannelDist, progress); !isfinite(correctDist)) {
                correctDist = interpDist(startSecondChannelDist, endSecondChannelDist, progress);
            }
            interpEpoch->setInterpFirstChannelDist(correctDist);
            interpEpoch->setInterpSecondChannelDist(correctDist);
        }

        // "interp" data to anchor epochs
        startEpoch->setInterpNED(startPos.ned);
        startEpoch->setInterpYaw(startYaw);
        float correctDist = isfinite(startFirstChannelDist) ? startFirstChannelDist : startSecondChannelDist;
        startEpoch->setInterpFirstChannelDist(correctDist);
        startEpoch->setInterpSecondChannelDist(correctDist);
        endEpoch->setInterpNED(endPos.ned);
        endEpoch->setInterpYaw(endYaw);
        correctDist = isfinite(endFirstChannelDist) ? endFirstChannelDist : endSecondChannelDist;
        endEpoch->setInterpFirstChannelDist(correctDist);
        endEpoch->setInterpSecondChannelDist(correctDist);

        somethingInterp = true;
        firstValidIndex = secondValidIndex;
        lastInterpIndx_ = toIndx;
    }

    if (somethingInterp) {
        emit datasetPtr_->updatedInterpolatedData(endEpochIndx);
    }
}

void Dataset::Interpolator::clear()
{
    lastInterpIndx_ = 0;
    firstChannelId_ = CHANNEL_NONE;
    secondChannelId_ = CHANNEL_FIRST;
}

bool Dataset::Interpolator::updateChannelsIds()
{
    bool retVal = false;

    firstChannelId_ = -1;
    secondChannelId_ = -1;

    if (datasetPtr_) {
        if (auto chList = datasetPtr_->channelsList(); !chList.empty()) {
            auto it = chList.begin();
            firstChannelId_ = it.key();

            if (++it != chList.end()) {
                secondChannelId_ = it.key();
            }

            retVal = true;
        }
    }

    return retVal;
}

float Dataset::Interpolator::interpYaw(float start, float end, float progress) const
{
    float delta = end - start;
    if (delta > 180.0f) {
        delta -= 360.0f;
    }
    else if (delta < -180.0f) {
        delta += 360.0f;
    }

    float interpolated = start + progress * delta;
    if (interpolated < 0.0f) {
        interpolated += 360.0f;
    }
    else if (interpolated >= 360.0f) {
        interpolated -= 360.0f;
    }
    return interpolated;
}

NED Dataset::Interpolator::interpNED(const NED &start, const NED &end, float progress) const
{
    NED result;
    result.n = (1.0 - progress) * start.n + progress * end.n;
    result.e = (1.0 - progress) * start.e + progress * end.e;
    result.d = (1.0 - progress) * start.d + progress * end.d;
    return result;
}

float Dataset::Interpolator::interpDist(float start, float end, float progress) const
{
    return (1.0 - progress) * start + progress * end;
}

qint64 Dataset::Interpolator::calcTimeDiffInNanoSecs(time_t startSecs, int startNanoSecs, time_t endSecs, int endNanoSecs) const
{
    qint64 startTimeInNanoseconds = static_cast<qint64>(startSecs) * 1000000000 + startNanoSecs;
    qint64 endTimeInNanoseconds = static_cast<qint64>(endSecs) * 1000000000 + endNanoSecs;

    return endTimeInNanoseconds - startTimeInNanoseconds;
}

qint64 Dataset::Interpolator::convertToNanosecs(time_t secs, int nanoSecs) const
{
    return static_cast<qint64>(secs) * 1000000000 + nanoSecs;
}

std::pair<time_t, int> Dataset::Interpolator::convertFromNanosecs(qint64 totalNanoSecs) const
{
    time_t seconds = static_cast<time_t>(totalNanoSecs / 1000000000);
    int nanoseconds = static_cast<int>(totalNanoSecs % 1000000000);

    return { seconds, nanoseconds };
}

#include "epoch.h"

#include <QPainterPath>
#include <core.h>
extern Core core;


Epoch::Epoch()
{
    charts_.clear();
    flags.distAvail = false;
}

void Epoch::setEvent(int timestamp, int id, int unixt)
{
    _eventTimestamp_us = timestamp;
    _eventUnix = unixt;
    _eventId = id;
    _time = DateTime(unixt, timestamp*1000);
    flags.eventAvail = true;
}

void Epoch::setChart(const ChannelId& channelId, const QVector<QVector<uint8_t>>& data, float resolution, float offset)
{
    auto& echograms = charts_[channelId];

    for (int i = 0; i < data.size(); ++i) {
        if (i >= echograms.size()) {
            echograms.resize(i + 1);
        }

        auto& echogram = charts_[channelId][i];

        echogram.amplitude = data[i];
        echogram.resolution = resolution;
        echogram.offset = offset;
        echogram.type = 1;
    }
}

void Epoch::setChartBySubChannelId(const ChannelId &channelId, uint8_t subChannelId, const QVector<uint8_t>& chartData, float resolution, float offset)
{
    if (!charts_.contains(channelId)) {
        return;
    }

    auto& allChartsByChannelId = charts_[channelId];
    if (subChannelId >= allChartsByChannelId.size()) {
        return;
    }

    auto& charts = allChartsByChannelId[subChannelId];
    charts.amplitude = chartData;
    charts.resolution = resolution;
    charts.offset = offset;
    charts.type = 1;
}

void Epoch::setRecParameters(const ChannelId& channelId, const RecordParameters& recParams)
{
    if (charts_.contains(channelId)) {
        auto& echograms =  charts_[channelId];

        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_ = recParams;
        }
    }
}

void Epoch::setChartParameters(const ChannelId& channelId, const ChartParameters& chartParams)
{
    if (charts_.contains(channelId)) {
        auto& echograms =  charts_[channelId];

        for (auto& iEchogram : echograms) {
            iEchogram.chartParameters_ = chartParams;
        }
    }
}

// void Epoch::setComplexSignal16(int channel, QVector<Complex16> data) {
//     // _complex[channel].data = QByteArray((const char*)data.constData(), data.size()*sizeof(Complex16));
//     // _complex[channel].type = 2;
// }

void Epoch::setComplexF(const ChannelId& channelId, int group, QVector<ComplexSignal> signal) {
    _complex[channelId][group] = signal;
}

void Epoch::setDist(const ChannelId& channelId, int dist) {
    rangefinders_[channelId] = dist * 0.001;
    flags.distAvail = true;
}

void Epoch::setRangefinder(const ChannelId& channelId, float distance) {
    rangefinders_[channelId] = distance;
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

void Epoch::setSonarPosition(Position val)
{
    sonarPosition_ = val;
    flags.sonarPosAvail = true;
}

void Epoch::setPositionLLA(const LLA &lla)
{
    _positionGNSS.lla = lla;
}

void Epoch::setPositionNED(const NED &ned)
{
    _positionGNSS.ned = ned;
}

void Epoch::setExternalPosition(Position position) {
    _positionExternal = position;
}

void Epoch::setPositionRef(LLARef* ref) {
    if(ref != NULL && ref->isInit) {
        _positionGNSS.LLA2NED(ref);
    }
}

void Epoch::setDepth(float depth)
{
    depth_ = depth;
}

float Epoch::getDepth()
{
    return depth_;
}

void Epoch::setPositionDataType(DataType dataType)
{
    _positionGNSS.dataType = dataType;
}

void Epoch::setSonarPositionDataType(DataType dataType)
{
    sonarPosition_.dataType = dataType;
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

int Epoch::chartSize(const ChannelId &channelId, uint8_t subChannelId)
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return -1;
    }

    const auto& v = it.value();
    return hasIndex(v, subChannelId) ? v.at(subChannelId).amplitude.size() : -1;
}

bool Epoch::chartAvail()
{
    return charts_.size() > 0;
}

bool Epoch::chartAvail(const ChannelId &channelId, uint8_t subChannelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return false;
    }

    const auto& echograms = it.value();
    return hasIndex(echograms, subChannelId) && !echograms.at(subChannelId).amplitude.isEmpty();
}

QList<ChannelId> Epoch::chartChannels()
{
    return charts_.keys();
}

Epoch::Echogram *Epoch::chart(const ChannelId &channelId, uint8_t subChannelId)
{
    if (chartAvail(channelId, subChannelId)) {
        return &charts_[channelId][subChannelId];
    }

    return nullptr;
}

Epoch::Echogram Epoch::chartCopy(const ChannelId &channelId, uint8_t subChannelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return {};
    }

    const auto& v = it.value();
    return hasIndex(v, subChannelId) ? v.at(subChannelId) : Epoch::Echogram{};
}

void Epoch::setAtt(float yaw, float pitch, float roll, DataType dataType) {
    _attitude.yaw = yaw;
    _attitude.pitch = pitch;
    _attitude.roll = roll;

    _attitude.dataType = dataType;
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

void Epoch::moveComplexToEchogram(ChannelId channel_id, int group_id, float offset_m, float levels_offset_db) {
    QVector<ComplexSignal> chls = _complex[channel_id][group_id];
    float sample_rate = chls[0].sampleRate;

    QVector<QVector<uint8_t>> chart;
    chart.resize(chls.size());

    for(int ch_i = 0; ch_i < chls.size(); ch_i++) {
        int ch_echo = ch_i;

        int data_size = chls[ch_i].data.size();

        chart[ch_echo].resize(chls[ch_i].data.size());
        uint8_t* chart_data = chart[ch_echo].data();
        ComplexF* compelex_data = chls[ch_i].data.data();

        for (int k  = 0; k < data_size; k++) {
            float amp = (compelex_data[k].logPow() + levels_offset_db)*2.5;

            if (amp < 0) {
                amp = 0;
            }
            else if(amp > 255) {
                amp = 255;
            }

            chart_data[k] = amp;
        }
    }

    setChart(ChannelId(channel_id.uuid, group_id), chart, 1500.0f/sample_rate, offset_m);
}

// write to all
void Epoch::setResolution(const ChannelId& channelId, uint16_t resolution)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.resol = resolution;
        }
    }
}

void Epoch::setChartCount(const ChannelId& channelId, uint16_t chartCount)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.count = chartCount;
        }
    }
}

void Epoch::setOffset(const ChannelId& channelId, uint16_t offset)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.offset = offset;
        }
    }
}

void Epoch::setFrequency(const ChannelId& channelId, uint16_t frequency)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.freq = frequency;
        }
    }
}

void Epoch::setPulse(const ChannelId& channelId, uint8_t pulse)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.pulse = pulse;
        }
    }
}

void Epoch::setBoost(const ChannelId& channelId, uint8_t boost)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.boost = boost;
        }
    }
}

void Epoch::setSoundSpeed(const ChannelId& channelId, uint32_t soundSpeed)
{
    if (charts_.contains(channelId)) {
        auto& echograms = charts_[channelId];
        for (auto& iEchogram : echograms) {
            iEchogram.recordParameters_.soundSpeed = soundSpeed;
        }
    }
}

// get from first
uint16_t Epoch::getResolution(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.resol;
    }

    return 0;
}

uint16_t Epoch::getChartCount(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.count;
    }

    return 0;
}

uint16_t Epoch::getOffset(const ChannelId& channelId) const
{    
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.offset ;
    }

    return 0;
}

uint16_t Epoch::getFrequency(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.freq ;
    }

    return 0;
}

uint8_t Epoch::getPulse(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.pulse ;
    }

    return 0;
}

uint8_t Epoch::getBoost(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.boost ;
    }

    return 0;
}

uint32_t Epoch::getSoundSpeed(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return 0;
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.recordParameters_.soundSpeed ;
    }

    return 0;
}

ChartParameters Epoch::getChartParameters(const ChannelId& channelId) const
{
    auto it = charts_.constFind(channelId);
    if (it == charts_.cend()) {
        return {};
    }

    const auto& echograms = it.value();

    for (const auto& iEchogram : echograms) {
        return iEchogram.chartParameters_;
    }

    return {};
}

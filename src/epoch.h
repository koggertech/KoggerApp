#pragma once

#include <math.h>
#include <stdint.h>
#include <time.h>
#include <QPixmap>
#include <QRectF>
#include <QVector>

#include "dataset_defs.h"
#include "id_binnary.h"


class Epoch
{
public:
    struct Contact {
        bool isValid() const {
            return !info.isEmpty() &&
                   cursorX != -1 &&
                   cursorY != -1;
        }
        void clear() {
            info.clear();
            lat      = 0.0f;
            lon      = 0.0f;
            echogramDistance = 0.0f;
            depth    = 0.0f;
            nedX     = 0.0f;
            nedY     = 0.0f;
            cursorX  = -1;
            cursorY  = -1;
            rectEcho = QRectF();
        }        

        QString info;
        float   lat = 0.0f;
        float   lon = 0.0f;
        float   echogramDistance = 0.0f;
        float   depth = 0.0f;
        float   nedX = 0.0f;
        float   nedY = 0.0f;
        int     cursorX = -1;
        int     cursorY = -1;
        QRectF  rectEcho;
    };

    struct DistProcessing {
        enum class DistanceSource {
            DistanceSourceNone = 0,
            DistanceSourceProcessing,
            DistanceSourceLoad,
            DistanceSourceConstrainHand,
            DistanceSourceDirectHand,
        };

        float distance = NAN;
        float min = NAN;
        float max = NAN;
        DistanceSource source = DistanceSource::DistanceSourceNone;

        Position bottomPoint;

        bool isDist() const { return isfinite(distance); }
        void setDistance(float dist, DistanceSource src = DistanceSource::DistanceSourceNone) { distance = dist; source = src; }
        void clearDistance(DistanceSource src = DistanceSource::DistanceSourceNone) { distance = NAN; source = src; }
        void resetDistance() { distance = NAN; source = DistanceSource::DistanceSourceNone; }
        float getDistance() const { return distance; }

        void setMin(float val, DistanceSource src = DistanceSource::DistanceSourceNone) {
            min = val;
            if(max != NAN && val + 0.05 > max) {
                max = val + 0.05;
            }
            source = src;
        }
        void setMax(float val, DistanceSource src = DistanceSource::DistanceSourceNone) {
            max = val;
            if(min != NAN && val - 0.05 < min) {
                min = val - 0.05;
            }
            source = src;
        }

        float getMax() const { return max; }
        float getMin() const { return min; }
    };

    struct Echogram {
        QVector<uint8_t> amplitude;
        float resolution = 0; // m
        float offset = 0; // m
        int type = 0;

        QVector<uint8_t> compensated;

        void updateCompesated() {
            int raw_size = amplitude.size();
            if(compensated.size() != raw_size) {
                compensated.resize(raw_size);
            }

            const uint8_t* src = amplitude.constData();
            uint8_t* procData = compensated.data();

            const float resol = resolution;

            float avrg = 255;
            for(int i = 0; i < raw_size; i ++) {
                float val = src[i];

                avrg += (val - avrg)*(0.05f + avrg*0.0006);
                val = (val - avrg*0.55f)*(0.85f +float(i*resol)*0.006f)*2.f;

                if(val < 0) { val = 0; }
                else if(val > 255) { val = 255; }

                procData[i] = val;
            }
        }

        DistProcessing bottomProcessing;
        Position sensorPosition;
        RecordParameters recordParameters_;
        ChartParameters chartParameters_;

        float range() const {
            return amplitude.size() * resolution;
        }

        bool isValid() const {
            return !amplitude.empty();
        }
    };

    Epoch();

    bool isValid() const {
        if (flags.eventAvail) return true;
        if (!charts_.isEmpty()) return true;
        if (flags.posAvail) return true;
        if (flags.tempAvail) return true;
        if (flags.distAvail) return true;

        return false;
    }

    bool operator==(const Epoch& other) const {
        return _eventId == other._eventId &&
               _eventTimestamp_us == other._eventTimestamp_us &&
               _eventUnix == other._eventUnix;
    }

    bool operator!=(const Epoch& other) const {
        return !(*this == other);
    }

    void setEvent(int timestamp, int id, int unixt);
    void setChart(const ChannelId& channelId, const QVector<QVector<uint8_t>>& chartData, float resolution, float offset);
    void setChartBySubChannelId(const ChannelId& channelId, uint8_t subChannelId, const QVector<uint8_t>& chartData, float resolution, float offset);

    void setRecParameters(const ChannelId& channelId, const RecordParameters& recParams);
    void setChartParameters(const ChannelId& channelId, const ChartParameters& chartParams);
    void setDist(const ChannelId& channelId, int dist);
    void setRangefinder(const ChannelId& channelId, float distance);
    void setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void setDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void setPositionLLA(double lat, double lon, LLARef* ref = NULL, uint32_t unix_time = 0, int32_t nanosec = 0);
    void setPositionLLA(Position position);
    void setSonarPosition(Position val);
    void setPositionLLA(const LLA& lla);
    void setPositionNED(const NED& ned);
    void setExternalPosition(Position position);
    void setPositionRef(LLARef* ref);

    void setDepth(float depth);
    float getDepth();
    bool isDepthAvail() { return isfinite(depth_); }


    void setPositionDataType(DataType dataType);
    DataType getPositionDataType() const { return _positionGNSS.dataType; };

    void setSonarPositionDataType(DataType dataType);
    DataType getSonarPositionDataType() const { return sonarPosition_.dataType; };
    void setComplexF(const ChannelId& channelId, int group, QVector<ComplexSignal> signal);
    ComplexSignals& complexSignals() { return _complex; }
    //ComplexSignal complexSignal(const ChannelId& channelId) { return _complex[channelId]; }
    bool isComplexSignalAvail() { return _complex.size() > 0; }

    void set(IDBinUsblSolution::UsblSolution data) { _usblSolution = data;  _isUsblSolutionAvailable = true; }

    void setGnssVelocity(double h_speed, double course);

    void setTime(DateTime time);
    void setTime(int year, int month, int day, int hour, int min, int sec, int nanosec = 0);

    void setTemp(float temp_c);
    void setAtt(float yaw, float pitch, float roll, DataType dataType = DataType::kRaw);
    DataType getAttDataType() const { return _attitude.dataType; };

    void setEncoders(float enc1, float enc2, float enc3);
    bool isEncodersSeted() { return _encoder.isSeted();}
    float encoder1() { return _encoder.e1; }
    float encoder2() { return _encoder.e2; }
    float encoder3() { return _encoder.e3; }

    void setDistProcessing(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setDistance(dist, DistProcessing::DistanceSource::DistanceSourceDirectHand);
            }
        }
    }

    void clearDistProcessing(const ChannelId& channelId) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.clearDistance(DistProcessing::DistanceSource::DistanceSourceDirectHand);
            }
        }
    }

    void setMinDistProc(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setMin(dist, DistProcessing::DistanceSource::DistanceSourceConstrainHand);
            }
        }
    }

    void setMaxDistProc(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setMax(dist, DistProcessing::DistanceSource::DistanceSourceConstrainHand);
            }
        }
    }

    void setMinMaxDistProc(const ChannelId& channelId, int min, int max,  bool isSave = true) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                float minsave = iEchogram.bottomProcessing.getMin();
                float maxsave = iEchogram.bottomProcessing.getMax();

                iEchogram.bottomProcessing.setMin(min);
                iEchogram.bottomProcessing.setMax(max);
                iEchogram.bottomProcessing.resetDistance();

                if (!isSave) {
                    iEchogram.bottomProcessing.setMin(minsave);
                    iEchogram.bottomProcessing.setMax(maxsave);
                }
            }
        }
    }

    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp_us; }
    int eventUnix() { return _eventUnix; }

    DateTime* time() { return &_time; }

    //QVector<uint8_t> chartData(const ChannelId& channelId = CHANNEL_NONE) {
    //    if(chartAvail(channelId)) {
    //        return charts_[channelId].amplitude;
    //    }
    //    return QVector<uint8_t>();
    //}
    int chartSize(const ChannelId& channelId = CHANNEL_NONE, uint8_t subChannelId = 0);
    bool chartAvail();
    bool chartAvail(const ChannelId& channelId, uint8_t subChannelId = 0) const;
    Echogram* chart(const ChannelId& channelId = CHANNEL_NONE, uint8_t subChannelId = 0);
    Echogram chartCopy(const ChannelId &channelId = CHANNEL_NONE, uint8_t subChannelId = 0) const;

    QList<ChannelId> chartChannels();

    // float chartsFullRange(const ChannelId& channelId = ChannelId()) {
    //     Q_UNUSED(channelId);
    //     QList<ChannelId> charts_channels = chartChannels();

    //     float full_range  = 0;
    //     for(uint16_t ch = 0; ch < charts_channels.size(); ch++) {
    //         int16_t channel = charts_channels[ch];
    //         full_range += _charts[channel].range();
    //     }

    //     return full_range;
    // }

    float getMaxRange(const ChannelId& channel = CHANNEL_NONE, const ChannelId& channel2 = CHANNEL_NONE)
    {
        float maxRange = NAN;

        if (channel == CHANNEL_NONE && channel2 == CHANNEL_NONE) {
            for (auto it = charts_.cbegin(), end = charts_.cend(); it != end; ++it) {
                const auto &echogramList = it.value();
                for (const auto& ech : echogramList) {
                    float r = ech.range();
                    if (std::isfinite(r) && (!std::isfinite(maxRange) || r > maxRange)) {
                        maxRange = r;
                        break;
                    }
                }
            }
        }
        else {
            auto extractMaxFromChannel = [this](const ChannelId& ch) -> float {
                float result = NAN;
                if (charts_.contains(ch)) {
                    const auto& chChartsCRef = charts_[ch];
                    for (const auto& ech : chChartsCRef) {
                        float r = ech.range();
                        if (std::isfinite(r) && (!std::isfinite(result) || r > result)) {
                            result = r;
                            break;
                        }
                    }
                }
                return result;
            };

            const float r1 = extractMaxFromChannel(channel);
            const float r2 = extractMaxFromChannel(channel2);

            if (std::isfinite(r1)) {
                maxRange = r1;
            }
            if (std::isfinite(r2) && (!std::isfinite(maxRange) || r2 > maxRange)) {
                maxRange = r2;
            }
        }

        if (!rangefinders_.isEmpty()) {
            float r3 = rangefinders_.first();
            if (std::isfinite(r3) && (!std::isfinite(maxRange) || r3 > maxRange)) {
                maxRange = r3;
            }
        }

        return maxRange;


    }

    bool distAvail() const
    {
        return flags.distAvail;
    }

    double distProccesing(const ChannelId& channelId = CHANNEL_NONE)
    {
        if (channelId == CHANNEL_NONE) {
            for (auto it = charts_.cbegin(); it != charts_.cend(); ++it) {
                for (const auto& iEchogram : it.value()) {
                    double distance = iEchogram.bottomProcessing.getDistance();
                    if (std::isfinite(distance)) {
                        return distance;
                    }
                }
            }
        } else if (charts_.contains(channelId)) {
            const auto& chart = charts_[channelId];
            for (const auto& ech : chart) {
                double distance = ech.bottomProcessing.getDistance();
                if (std::isfinite(distance)) {
                    return distance;
                }
            }
        }

        return NAN;
    }

    float rangeFinder() const {
        if(rangefinders_.size() > 0) {
            return rangefinders_.first();
        }

        return NAN;
    }

    float temperature() { return m_temp_c; }
    bool temperatureAvail() { return flags.tempAvail; }

    bool isAttAvail() { return _attitude.isAvail(); }
    float yaw() { return _attitude.yaw; }
    float pitch() { return _attitude.pitch; }
    float roll() { return _attitude.roll; }

    bool isDopplerAvail() { return doppler.isAvai; }
    float dopplerX() { return doppler.velocityX; }

    bool isDopplerBeamAvail() { return _dopplerBeamCount > 0; }
    bool isDopplerBeamAvail(uint16_t num) { return _dopplerBeamCount > num; }
    IDBinDVL::BeamSolution dopplerBeam(uint16_t num) { return _dopplerBeams[num]; }
    uint16_t dopplerBeamCount() {return _dopplerBeamCount; }

    IDBinDVL::DVLSolution dvlSolution() { return _dvlSolution; }
    bool isDVLSolutionAvail() {  return flags.isDVLSolutionAvail; }


    bool isUsblSolutionAvailable() { return _isUsblSolutionAvailable; }
    IDBinUsblSolution::UsblSolution usblSolution() { return _usblSolution; }


    double lat() { return _positionGNSS.lla.latitude; }
    double lon() { return _positionGNSS.lla.longitude; }

    Position getPositionGNSS() { return _positionGNSS; }
    Position getExternalPosition() { return _positionExternal; }
    Position getSonarPosition() { return sonarPosition_; }

    uint32_t positionTimeUnix() { return _positionGNSS.time.sec; }
    uint32_t positionTimeNano() { return _positionGNSS.time.nanoSec; }
    DateTime* positionTime() {return &_positionGNSS.time; }

    void setGNSSSec(time_t sec);
    void setGNSSNanoSec(int nanoSec);

    double relPosN() { return _positionGNSS.ned.n; }
    double relPosE() { return _positionGNSS.ned.e; }
    double relPosD() { return _positionGNSS.ned.d; }

    bool isPosAvail() { return flags.posAvail; }

    double gnssHSpeed() { return _GnssData.hspeed; }


    void doBottomTrack2D(Echogram &chart, bool is_update_dist = false);
    void doBottomTrackSideScan(Echogram &chart, bool is_update_dist = false);


    bool chartTo(const ChannelId& channelId, uint8_t subChannelId, float start, float end, int16_t* dst, int len, int imageType, bool reverse = false)
    {
        if (dst == nullptr) {
            return false;
        }

        ChannelId localChannelId = channelId;

        //if (localChannelId == CHANNEL_NONE && charts_.size() > 0) {
        //    localChannelId = charts_.firstKey();
        //}
        /*else*/ if (!charts_.contains(localChannelId)) {
            memset(dst, 0, len * 2);
            return false;
        }

        if (charts_[localChannelId][subChannelId].resolution == 0) {
            memset(dst, 0, len * 2);
            return false;
        }

        int rawSize = charts_[localChannelId][subChannelId].amplitude.size();

        if (rawSize == 0) {
            memset(dst, 0, len * 2);
            return false;
        }

        uint8_t* src = charts_[localChannelId][subChannelId].amplitude.data();

        if (imageType == 1) {
            if (charts_[localChannelId][subChannelId].compensated.size() == 0) {
                charts_[localChannelId][subChannelId].updateCompesated();
            }
            src = charts_[localChannelId][subChannelId].compensated.data();
        }

        if (rawSize == 0) {
            for (int iTo = 0; iTo < len; iTo++) {
                dst[iTo] = 0;
            }
        }

        start -= charts_[localChannelId][subChannelId].offset;
        end -= charts_[localChannelId][subChannelId].offset;

        float rawRangeF = charts_[localChannelId][subChannelId].range();
        float targetRangeF = static_cast<float>(end - start);
        float scaleFactor = (static_cast<float>(rawSize) / static_cast<float>(len)) * (targetRangeF / rawRangeF);
        int offset = start / charts_[localChannelId][subChannelId].resolution;

        int srcStart = offset;
        int dir = reverse ? -1 : 1;
        int off = reverse ? (len-1) : 0;
        if (scaleFactor >= 0.8f) {
            for (int iTo = 0; iTo < len; iTo++) {
                int srcEnd = static_cast<float>(iTo + 1) * scaleFactor + offset;

                int32_t val = 0;
                if (srcStart >= 0 && srcStart < rawSize) {
                    if (srcEnd > rawSize) {
                        srcEnd = rawSize;
                    }

                    val = src[srcStart];
                    for (int i = srcStart; i < srcEnd; i++) {
                        val += src[i];
                    }
                    val /= 1 + (srcEnd - srcStart);
                }

                srcStart = srcEnd;
                dst[off + dir * iTo] = val;
            }
        }
        else {
            for (int iTo = 0; iTo < len; iTo++) {
                float cellOffset = static_cast<float>(iTo) * scaleFactor + static_cast<float>(offset) + 0.5f;
                int srcStart = static_cast<int>(cellOffset);
                int srcEnd = srcStart + 1;

                int32_t val = 0;
                if (srcStart >= 0 && srcStart < rawSize) {
                    if (srcEnd >= rawSize) {
                        srcEnd = rawSize - 1;
                    }

                    float coef = cellOffset - floorf(cellOffset);
                    val = static_cast<float>(src[srcStart]) * (1 - coef) + static_cast<float>(src[srcEnd]) * coef;
                }

                dst[off + dir*iTo] = val;
            }
        }

        return true;
    }

    void moveComplexToEchogram(ChannelId channel_id, int group_id, float offset_m, float levels_offset_db);

    void setResolution      (const ChannelId& channelId, uint16_t resolution);
    void setChartCount      (const ChannelId& channelId, uint16_t chartCount);
    void setOffset          (const ChannelId& channelId, uint16_t offset);
    void setFrequency       (const ChannelId& channelId, uint16_t frequency);
    void setPulse           (const ChannelId& channelId, uint8_t pulse);
    void setBoost           (const ChannelId& channelId, uint8_t boost);
    void setSoundSpeed      (const ChannelId& channelId, uint32_t soundSpeed);
    uint16_t getResolution  (const ChannelId& channelId) const;
    uint16_t getChartCount  (const ChannelId& channelId) const;
    uint16_t getOffset      (const ChannelId& channelId) const;
    uint16_t getFrequency   (const ChannelId& channelId) const;
    uint8_t getPulse        (const ChannelId& channelId) const;
    uint8_t getBoost        (const ChannelId& channelId) const;
    uint32_t getSoundSpeed  (const ChannelId& channelId) const;
    ChartParameters getChartParameters(const ChannelId& channelId) const;
    Contact contact_; // TODO: private

    uint8_t getChartsSizeByChannelId(const ChannelId& channelId) const {
        if (charts_.contains(channelId)) {
            return static_cast<uint8_t>(charts_[channelId].size());
        }
        return 0;
    }
protected:
    QMap<ChannelId, QVector<Echogram>> charts_; // key - channelId, value - echograms for all addresses
    QMap<ChannelId, float> rangefinders_; // ???

    int _eventTimestamp_us = 0;
    int _eventUnix = 0;
    int _eventId = 0;

    DateTime _time;

    struct {
        float yaw = NAN, pitch = NAN, roll = NAN;

        DataType dataType;

        bool isAvail() {
            return isfinite(yaw) && isfinite(pitch) && isfinite(roll);
        }
    } _attitude;

    ComplexSignals _complex;

    IDBinDVL::BeamSolution _dopplerBeams[4];
    uint16_t _dopplerBeamCount = 0;

    IDBinDVL::DVLSolution _dvlSolution;

    IDBinUsblSolution::UsblSolution _usblSolution;
    bool _isUsblSolutionAvailable = false;

    Position _positionGNSS;
    Position _positionExternal;
    Position sonarPosition_;

    struct {
        double hspeed = NAN;
        double course = NAN;
    } _GnssData;

    float m_temp_c = NAN;

    struct {
        float e1 = NAN;
        float e2 = NAN;
        float e3 = NAN;
        bool isSeted() {
            return isfinite(e1) || isfinite(e2) || isfinite(e3);
        }
    } _encoder;

    struct {
        float velocityX = 0;
        float velocityY = 0;
        float velocityZ = 0;
        bool isAvai = false;
    } doppler;

    struct {
        bool encoderAvail = false;
        bool eventAvail = false;
        bool timestampAvail = false;
        bool distAvail = false;

        bool posAvail = false;
        bool sonarPosAvail = false;

        bool tempAvail = false;
        bool isDVLSolutionAvail = false;

    } flags;

    float depth_ = NAN;

};

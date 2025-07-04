#pragma once

#include <math.h>
#include <stdint.h>
#include <time.h>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QVector>
#include <QVector3D>
#include <QQmlEngine>
#include "dataset_defs.h"
#include "dsp_defs.h"
#include "id_binnary.h"
#include "usbl_view.h"


#if defined(Q_OS_ANDROID) || (defined Q_OS_LINUX)
#define MAKETIME(t) mktime(t)
#define GMTIME(t) gmtime(t)
#else
#define MAKETIME(t) _mkgmtime64(t)
#define GMTIME(t) _gmtime64(&sec);
#endif

#define CONSTANTS_RADIUS_OF_EARTH 6371000 /* meters (m) */
#define M_TWOPI_F 6.28318530717958647692f
#define M_PI_2_F  1.57079632679489661923f
#define M_RAD_TO_DEG 57.29577951308232087679f
#define M_DEG_TO_RAD 0.01745329251994329576f

class BlackStripesProcessor;

typedef struct NED NED;
typedef struct LLARef LLARef;

enum PositionSource {
    PositionSourceNone,
    PositionSourceGNSS,
    PositionSourceRTK,
    PositionSourcePPK,
    PositionSourceInterpolation,
    PositionSourceExtrapolation,
    PositionSourceNED,
    PositionSourceLLA,
};

enum AltitudeSource {
    AltitudeSourceNone,
    AltitudeSourceRTK,
    AltitudeSourcePPK
};

typedef struct LLA {
    double latitude = NAN, longitude = NAN;
    double altitude = NAN;

    PositionSource source = PositionSourceNone;
    AltitudeSource altSource = AltitudeSourceNone;

    LLA() {};
    LLA(double lat, double lon, double alt= NAN) {
        latitude = lat;
        longitude = lon;
        altitude = alt;
    }

    LLA(const LLA& other) {
        latitude = other.latitude;
        longitude = other.longitude;
        altitude = other.altitude;
    }

    LLA& operator=(const LLA& other) {
        if (this != &other) {
            latitude = other.latitude;
            longitude = other.longitude;
            altitude = other.altitude;
        }
        return *this;
    }

    inline LLA(const NED* ned, const LLARef* ref, bool spherical = true);

    bool isValid() const {
        return isfinite(latitude) && isfinite(longitude) && isfinite(altitude);
    }
    bool isCoordinatesValid() {
        return isfinite(latitude) && isfinite(longitude);
    }
} LLA;

typedef struct  LLARef {
    double refLatSin = NAN;
    double refLatCos = NAN;
    double refLatRad = NAN;
    double refLonRad = NAN;
    LLA refLla;
    bool isInit = false;

    LLARef() {}

    LLARef(LLA lla) {
        refLatRad = lla.latitude * M_DEG_TO_RAD;
        refLonRad= lla.longitude * M_DEG_TO_RAD;
        refLatSin = sin(refLatRad);
        refLatCos = cos(refLatRad);
        refLla = lla;
        isInit = true;
    }

    LLARef(const LLARef& other)
        : refLatSin(other.refLatSin),
        refLatCos(other.refLatCos),
        refLatRad(other.refLatRad),
        refLonRad(other.refLonRad),
        refLla(other.refLla),
        isInit(other.isInit) {}

    LLARef& operator=(const LLARef& other) {
        if (this != &other) {
            refLatSin = other.refLatSin;
            refLatCos = other.refLatCos;
            refLatRad = other.refLatRad;
            refLonRad = other.refLonRad;
            refLla = other.refLla;
            isInit = other.isInit;
        }
        return *this;
    }

    friend bool operator==(const LLARef& lhs, const LLARef& rhs) {
        if (!std::isfinite(lhs.refLla.latitude) || !std::isfinite(lhs.refLla.longitude) ||
            !std::isfinite(rhs.refLla.latitude) || !std::isfinite(rhs.refLla.longitude)) {
            return false;
        }
        return qFuzzyCompare(1.0 + lhs.refLla.latitude, 1.0 + rhs.refLla.latitude) &&
               qFuzzyCompare(1.0 + lhs.refLla.longitude, 1.0 + rhs.refLla.longitude);
    }

    friend bool operator!=(const LLARef& lhs, const LLARef& rhs) {
        return !(lhs == rhs);
    }
} LLARef;

typedef struct NED {
    double n = NAN, e = NAN, d = NAN;
    PositionSource source = PositionSourceNone;

    NED() {}
    NED(double _n, double _e, double _d) : n(_n), e(_e), d(_d) { };
    NED(LLA* lla, LLARef* ref, bool spherical = true) {
        if (spherical) {
            double lat_rad = lla->latitude * M_DEG_TO_RAD;
            double lon_rad = lla->longitude * M_DEG_TO_RAD;

            double sin_lat = sin(lat_rad);
            double cos_lat = cos(lat_rad);
            double cos_d_lon = cos(lon_rad - ref->refLonRad);

            double arg = ref->refLatSin * sin_lat + ref->refLatCos * cos_lat * cos_d_lon;

            if (arg > 1.0) {
                arg = 1.0;

            } else if (arg < -1.0) {
                arg = -1.0;
            }

            double c = acos(arg);
            double k = (fabs(c) < __DBL_EPSILON__) ? 1.0 : (c / sin(c));

            n = k * (ref->refLatCos * sin_lat - ref->refLatSin * cos_lat * cos_d_lon) * CONSTANTS_RADIUS_OF_EARTH;
            e = k * cos_lat * sin(lon_rad - ref->refLonRad) * CONSTANTS_RADIUS_OF_EARTH;
        }
        else { // merсator
            double R = CONSTANTS_RADIUS_OF_EARTH;

            double lat_rad = lla->latitude * M_DEG_TO_RAD;
            double ref_lat_rad = ref->refLla.latitude * M_DEG_TO_RAD;

            double y = R * log(tan(M_PI / 4.0 + lat_rad / 2.0));
            double y_ref = R * log(tan(M_PI / 4.0 + ref_lat_rad / 2.0));

            double delta_y = y - y_ref;

            double delta_lon = (lla->longitude - ref->refLla.longitude) * M_DEG_TO_RAD;
            double x = R * delta_lon;

            n = delta_y;
            e = x;
            d = -(lla->altitude - ref->refLla.altitude);
        }
    }


    bool isValid() {
        return isfinite(n) && isfinite(e) && isfinite(d);
    }

    bool isCoordinatesValid() const {
        return isfinite(n) && isfinite(e);
    }
} NED;

LLA::LLA(const NED* ned, const LLARef* ref, bool spherical)
{
    if (spherical) {
        if (!ned || !ref ||
            !std::isfinite(ned->n) ||
            !std::isfinite(ned->e) ||
            !std::isfinite(ned->d)) {
            return;
        }

        double R = CONSTANTS_RADIUS_OF_EARTH;
        double n = ned->n;
        double e = ned->e;

        double c = sqrt(n * n + e * e) / R;

        if (qFuzzyIsNull(c)) {
            latitude = ref->refLla.latitude;
            longitude = ref->refLla.longitude;
            altitude = ref->refLla.altitude - ned->d;
            return;
        }

        double lat0 = ref->refLatRad;
        double lon0 = ref->refLonRad;

        double sin_c = sin(c);
        double cos_c = cos(c);

        double alpha = atan2(e, n);

        double sin_lat = sin(lat0) * cos_c + cos(lat0) * sin_c * cos(alpha);
        double lat_rad = asin(sin_lat);

        double y = sin(alpha) * sin_c * cos(lat0);
        double x = cos_c - sin(lat0) * sin_lat;
        double lon_rad = lon0 + atan2(y, x);

        latitude = lat_rad * 180.0 / M_PI;
        longitude = lon_rad * 180.0 / M_PI;
        altitude = ref->refLla.altitude - ned->d;
    }
    else { // mercator
        if (!ned || !ref ||
            !std::isfinite(ned->n) ||
            !std::isfinite(ned->e) ||
            !std::isfinite(ned->d)) {
            return;
        }

        double R = CONSTANTS_RADIUS_OF_EARTH;

        double ref_lat_rad = ref->refLla.latitude * M_DEG_TO_RAD;
        double y_ref = R * log(tan(M_PI / 4.0 + ref_lat_rad / 2.0));

        double y = y_ref + ned->n;

        double lat_rad = 2.0 * atan(exp(y / R)) - M_PI / 2.0;

        double ref_lon_rad = ref->refLla.longitude * M_DEG_TO_RAD;
        double lon_rad = ref_lon_rad + (ned->e / R);

        latitude = lat_rad * M_RAD_TO_DEG;
        longitude = lon_rad * M_RAD_TO_DEG;
        altitude = ref->refLla.altitude - ned->d;
    }
}

typedef struct XYZ {
    double x = 0, y = 0, z = 0;
} XYZ;

typedef struct DateTime {
    time_t sec = 0;
    int nanoSec = 0;

    DateTime() {}

    DateTime(int64_t unix_sec, int32_t nanosec = 0) {

        sec = unix_sec;
        int s_dif = nanosec/1e9;
        nanosec = nanosec - s_dif*1e9;
        if(nanosec < 0) {
            s_dif--;
            nanosec += 1e9;
        }

        sec += s_dif;
        nanoSec = nanosec;
    }

    DateTime(int year, int month, int day, int hour, int min, int s, int nanosec = 0) {
//        if(year >= 2000) {
//            year -= 2000;
//        }

        tm  t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = s;

        sec = MAKETIME(&t);

        int s_dif = nanosec/1e9;
        nanosec = nanosec - s_dif*1e9;
        if(nanosec < 0) {
            s_dif--;
            nanosec += 1e9;
        }

        sec += s_dif;
        nanoSec = nanosec;
    }

    tm getDateTime() {
        return *GMTIME(&sec);
    }

    int32_t get_us_frac() {
        return nanoSec/1000;
    }

    int32_t get_ms_frac() {
        return nanoSec/1000000;
    }

    void addSecs(int add_secs) {
        sec += add_secs;
    }



} DateTime;

typedef struct {
    DateTime time;
    LLA lla;
    NED ned;

    void LLA2NED(LLARef* ref) {
        ned = NED(&lla, ref);
    }
} Position;

template <typename T>
inline bool hasIndex(const QVector<T>& vec, int i) {
    return i >= 0 && i < vec.size();
}

typedef struct DatasetChannel {
    Q_GADGET
   // Q_PROPERTY(int channelId  MEMBER channel)

public:
    ChannelId channelId_;
    uint8_t subChannelId_ = 0;
    int count_ = 0;
    double distanceFrom_ = NAN;
    double distanceTo_ = NAN;
    XYZ localPosition_;
    QString portName_;

    DatasetChannel()
    {}

    DatasetChannel(const ChannelId& channelId, uint8_t subChannelId)
        : channelId_(channelId),
        subChannelId_(subChannelId)
    {}

    void counter() {
        count_++;
    }
} DatasetChannel;
Q_DECLARE_METATYPE(DatasetChannel)

enum class BottomTrackPreset {
    BottomTrackOneBeam = 0,
    BottomTrackOneBeamNarrow,
    BottomTrackSideScan
};

struct BottomTrackParam {
    float gainSlope = 1.0;
    float threshold = 1.0;
    float verticalGap = 0;
    float minDistance = 0;
    float maxDistance = 1000;

    int indexFrom = 0;
    int indexTo = 0;
    int windowSize = 1;

    BottomTrackPreset preset = BottomTrackPreset::BottomTrackOneBeam;

    struct {
        float x = 0, y = 0, z = 0;
    } offset;
};

typedef struct ComplexSignal {
    uint32_t globalOffset = 0;
    float sampleRate = 0;
    bool isComplex = true;
    QVector<ComplexF> data;
} ComplexSignal;

struct RecordParameters {
    uint16_t resol      = 0;
    uint16_t count      = 0;
    uint16_t offset     = 0;
    uint16_t freq       = 0;
    uint8_t  pulse      = 0;
    uint8_t  boost      = 0;
    uint32_t soundSpeed = 0;

    bool isNull() const {
        if (resol      == 0 &&
            count      == 0 &&
            offset     == 0 &&
            freq       == 0 &&
            pulse      == 0 &&
            boost      == 0 &&
            soundSpeed == 0) {
            return true;
        }
        return false;
    }
};

typedef QMap<ChannelId, QMap<int, QVector<ComplexSignal>>> ComplexSignals;

class Epoch {
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
            distance = 0.0f;
            nedX     = 0.0f;
            nedY     = 0.0f;
            cursorX  = -1;
            cursorY  = -1;
            rectEcho = QRectF();
        }        

        QString info;
        float   lat = 0.0f;
        float   lon = 0.0f;
        float   distance = 0.0f;
        float   nedX = 0.0f;
        float   nedY = 0.0f;
        int     cursorX = -1;
        int     cursorY = -1;
        QRectF  rectEcho;
    };

    typedef struct {
        typedef enum {
            DistanceSourceNone = 0,
            DistanceSourceProcessing,
            DistanceSourceLoad,
            DistanceSourceConstrainHand,
            DistanceSourceDirectHand,
        } DistanceSource;

        float distance = NAN;
        float min = NAN;
        float max = NAN;
        DistanceSource source = DistanceSourceNone;

        Position bottomPoint;

        bool isDist() const { return isfinite(distance); }
        void setDistance(float dist, DistanceSource src = DistanceSourceNone) { distance = dist; source = src; }
        void clearDistance(DistanceSource src = DistanceSourceNone) { distance = NAN; source = src; }
        void resetDistance() { distance = NAN; source = DistanceSourceNone; }
        float getDistance() const { return distance; }

        void setMin(float val, DistanceSource src = DistanceSourceNone) {
            min = val;
            if(max != NAN && val + 0.05 > max) {
                max = val + 0.05;
            }
            source = src;
        }
        void setMax(float val, DistanceSource src = DistanceSourceNone) {
            max = val;
            if(min != NAN && val - 0.05 < min) {
                min = val - 0.05;
            }
            source = src;
        }

        float getMax() const { return max; }
        float getMin() const { return min; }
    } DistProcessing;

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
    };




    Epoch();
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
    void setExternalPosition(Position position);
    void setPositionRef(LLARef* ref);

    void setComplexF(const ChannelId& channelId, int group, QVector<ComplexSignal> signal);
    ComplexSignals& complexSignals() { return _complex; }
    //ComplexSignal complexSignal(const ChannelId& channelId) { return _complex[channelId]; }
    bool isComplexSignalAvail() { return _complex.size() > 0; }

    void set(IDBinUsblSolution::UsblSolution data) { _usblSolution = data;  _isUsblSolutionAvailable = true; }

    void setGnssVelocity(double h_speed, double course);

    void setTime(DateTime time);
    void setTime(int year, int month, int day, int hour, int min, int sec, int nanosec = 0);

    void setTemp(float temp_c);
    void setAtt(float yaw, float pitch, float roll);

    void setEncoders(float enc1, float enc2, float enc3);
    bool isEncodersSeted() { return _encoder.isSeted();}
    float encoder1() { return _encoder.e1; }
    float encoder2() { return _encoder.e2; }
    float encoder3() { return _encoder.e3; }

    void setDistProcessing(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setDistance(dist, DistProcessing::DistanceSourceDirectHand);
            }
        }
    }

    void clearDistProcessing(const ChannelId& channelId) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.clearDistance(DistProcessing::DistanceSourceDirectHand);
            }
        }
    }

    void setMinDistProc(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setMin(dist, DistProcessing::DistanceSourceConstrainHand);
            }
        }
    }

    void setMaxDistProc(const ChannelId& channelId, float dist) {
        if (charts_.contains(channelId)) {
            auto& charts = charts_[channelId];
            for (auto& iEchogram : charts) {
                iEchogram.bottomProcessing.setMax(dist, DistProcessing::DistanceSourceConstrainHand);
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
            for (const auto& echogramList : charts_) {
                for (const auto& ech : echogramList) {
                    float r = ech.range();
                    if (std::isfinite(r) && (!std::isfinite(maxRange) || r > maxRange)) {
                        maxRange = r;
                        break;
                    }
                }
            }
        } else {
            auto extractMaxFromChannel = [this](const ChannelId& ch) -> float {
                float result = NAN;
                if (charts_.contains(ch)) {
                    for (const auto& ech : charts_[ch]) {
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

    void setInterpNED(NED ned);
    void setInterpYaw(float yaw);
    void setInterpFirstChannelDist(float dist);
    void setInterpSecondChannelDist(float dist);
    NED   getInterpNED() const;
    float getInterpYaw() const;
    float getInterpFirstChannelDist() const;
    float getInterpSecondChannelDist() const;

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

        bool tempAvail = false;
        bool isDVLSolutionAvail = false;

    } flags;

private:
    struct {
        NED ned;
        float yaw = NAN;
        float distFirstChannel = NAN;
        float distSecondChannel = NAN;

        bool isValid() const {
            if (ned.isCoordinatesValid()  &&
                isfinite(yaw)) {
                return true;
            }
            return false;
        };
    } interpData_;
};

class Dataset : public QObject {
    Q_OBJECT
public:
    /*structures*/
    enum class DatasetState {
        kUndefined = 0,
        kFile,
        kConnection
    };
    enum class LlaRefState {
        kUndefined = 0,
        kSettings,
        kFile,
        kConnection
    };

    /*methods*/
    Dataset();
    ~Dataset();

    void setState(DatasetState state);

#if defined(FAKE_COORDS)
    void setActiveZeroing(bool state);
#endif

    DatasetState getState() const;
    LLARef getLlaRef() const;
    void setLlaRef(const LLARef& val, LlaRefState state);

    inline int size() const {

        return pool_.size();

    }

    Epoch* fromIndex(int index_offset = 0) {
        int index = validIndex(index_offset);
        if(index >= 0) {
            return &pool_[index];
        }

        return NULL;
    }

    Epoch* last() {
        if(size() > 0) {
            return fromIndex(endIndex());
        }
        return addNewEpoch();
    }

    Epoch* lastlast() {
        if(size() > 1) {
            return fromIndex(endIndex()-1);
        }
        return NULL;
    }

    int endIndex() const {
        return size() - 1;
    }

    int validIndex(int index_offset = 0) {
        int index = index_offset;
        if(index >= size()) { index = endIndex(); }
        else if(index < 0) { index = -1; }
        return index;
    }

    void getMaxDistanceRange(float* from, float* to, const ChannelId& channel, uint8_t subAddressCh1, const ChannelId& channel2 = CHANNEL_NONE, uint8_t subAddressCh2 = 0);

    QVector<DatasetChannel> channelsList() {
        return channelsSetup_;
    }

    bool isContainsChannelInChannelSetup(const ChannelId& channelId) const {
        for (int16_t i = 0; i < channelsSetup_.size(); ++i) {
            if (channelsSetup_.at(i).channelId_ == channelId) {
                return true;
            }
        }

        return false;
    }

    QVector<QVector3D> boatTrack() const;
    const QHash<int, int>& getSelectedIndicesBoatTrack() const;
    int getLastBottomTrackEpoch() const;

    float getLastYaw() {
        return _lastYaw;
    }

    Position getLastPosition() {
        return _lastPositionGNSS;
    }

    float getLastTemp() {
        return lastTemp_;
    }

    BottomTrackParam* getBottomTrackParamPtr() {
        return &bottomTrackParam_;
    }

public slots:
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float angle1_deg, float angle2_deg = NAN, float angle3_deg = NAN);
    void addTimestamp(int timestamp);

    //
    void setChartSetup (const ChannelId& channelId, uint16_t resol, uint16_t count, uint16_t offset);
    void setTranscSetup(const ChannelId& channelId, uint16_t freq, uint8_t pulse, uint8_t boost);
    void setSoundSpeed (const ChannelId& channelId, uint32_t soundSpeed);
    void setFixBlackStripesState(bool state);
    void setFixBlackStripesForwardSteps(int val);
    void setFixBlackStripesBackwardSteps(int val);
    void addChart(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);
    void rawDataRecieved(const ChannelId& channelId, RawData raw_data);
    void addDist(const ChannelId& channelId, int dist);
    void addRangefinder(const ChannelId& channelId, float distance);
    void addUsblSolution(IDBinUsblSolution::UsblSolution data);
    void addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void addDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void addAtt(float yaw, float pitch, float roll);
    void addPosition(double lat, double lon, uint32_t unix_time = 0, int32_t nanosec = 0);
    void addPositionRTK(Position position);

    void addGnssVelocity(double h_speed, double course);

//    void addDateTime(int year, );
    void addTemp(float temp_c);

    void mergeGnssTrack(QList<Position> track);

    void resetDataset();
    void resetDistProcessing();

    void setChannelOffset(const ChannelId& channelId, float x, float y, float z);

    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2);
    void spatialProcessing();

    void usblProcessing();
    QVector<QVector3D> beaconTrack() {
        return _beaconTrack;
    }

    QVector<QVector3D> beaconTrack1() {
        return _beaconTrack1;
    }

    void setScene3D(GraphicsScene3dView* scene3dViewPtr) { scene3dViewPtr_ = scene3dViewPtr; };

    void setRefPosition(int epoch_index);
    void setRefPosition(Epoch* ref_epoch);
    void setRefPosition(Position position);
    void setRefPositionByFirstValid();
    Epoch* getFirstEpochByValidPosition();

    void clearBoatTrack();
    void updateBoatTrack(bool update_all = false);

    QStringList channelsNameList();
    std::tuple<ChannelId, uint8_t, QString> channelIdFromName(const QString& name) const;


    void interpolateData(bool fromStart);

signals:
    void dataUpdate();
    void bottomTrackUpdated(const ChannelId& channelId, int lEpoch, int rEpoch);
    void boatTrackUpdated();
    void updatedInterpolatedData(int indx);
    void updatedLlaRef();
    void channelsUpdated();
    void redrawEpochs(const QSet<int>& indxs);

protected:

    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

#if defined(FAKE_COORDS)
    bool activeZeroing_ = false;
    uint64_t testTime_ = 1740466541;
#endif

    QVector<DatasetChannel> channelsSetup_;

    void validateChannelList(const ChannelId& channelId, uint8_t subChannelId);

    QVector<QVector3D> _boatTrack;
    QHash<int, int> selectedBoatTrackVertexIndices_; // first - vertice indx, second - epoch indx
    QVector<QVector3D> _beaconTrack;
    QVector<QVector3D> _beaconTrack1;

    QMap<int, UsblView::UsblObjectParams> tracks;

    //enum {
    //    AutoRangeNone,
    //    AutoRangeLast,
    //    AutoRangeMax,
    //    AutoRangeMaxVis
    //} _autoRange = AutoRangeLast;


    QVector<Epoch> pool_;

    float _lastYaw = 0, _lastPitch = 0, _lastRoll = 0;
    Position _lastPositionGNSS;
    float lastTemp_ = NAN;

    Epoch* addNewEpoch();

    GraphicsScene3dView* scene3dViewPtr_ = nullptr;

private:
    friend class Interpolator;

    /*structures*/
    class Interpolator {
    public:
        explicit Interpolator(Dataset* datasetPtr);
        void interpolateData(bool fromStart);
        void clear();
    private:
        bool updateChannelsIds();
        float interpYaw(float start, float end, float progress) const;
        NED interpNED(const NED& start, const NED& end, float progress) const;
        float interpDist(float start, float end, float progress) const;
        qint64 calcTimeDiffInNanoSecs(time_t startSecs, int startNanoSecs, time_t endSecs, int endNanoSecs) const;
        qint64 convertToNanosecs(time_t secs, int nanoSecs) const;
        std::pair<time_t, int> convertFromNanosecs(qint64 totalNanoSecs) const; // first - secs, second - nanosecs

        Dataset* datasetPtr_;
        int lastInterpIndx_;
        ChannelId firstChannelId_;
        ChannelId secondChannelId_;
    };

    /*methods*/
    LlaRefState getCurrentLlaRefState() const;
    bool shouldAddNewEpoch(const ChannelId& channelId, uint8_t numSubChannels) const;
    void updateEpochWithChart(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);

    /*data*/
    LLARef _llaRef;
    LlaRefState llaRefState_ = LlaRefState::kUndefined;
    DatasetState state_ = DatasetState::kUndefined;
    Interpolator interpolator_;
    int lastBoatTrackEpoch_;
    int lastBottomTrackEpoch_;
    BottomTrackParam bottomTrackParam_;
    uint64_t boatTrackValidPosCounter_;
    QMap<ChannelId, RecordParameters> usingRecordParameters_;
    BlackStripesProcessor* bSProc_;
    QMap<ChannelId, int> lastAddChartEpochIndx_;
    QSet<ChannelId> channelsToResizeEthData_;

    // for GUI
    QList<QString> channelsNames_;
    QList<ChannelId> channelsIds_;
    QList<uint8_t> subChannelIds_;
};

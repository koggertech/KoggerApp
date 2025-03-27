#ifndef PLOT_CASH_H
#define PLOT_CASH_H

#include <QObject>
#include <stdint.h>
#include <QVector>
#include <QImage>
#include <QPoint>
#include <QPixmap>
#include <QPainter>
#include "math.h"
#include <qvector3d.h>
#include <QQmlEngine>
#include <QMutex>

#include <DSP.h>

#include <3Plot.h>
#include <IDBinnary.h>

#include "time.h"

#include "usbl_view.h"

#if defined(Q_OS_ANDROID) || (defined Q_OS_LINUX)
#define MAKETIME(t) mktime(t)
#define GMTIME(t) gmtime(t)
#else
#define MAKETIME(t) _mkgmtime64(t)
#define GMTIME(t) _gmtime64(&sec);

#endif


#define CONSTANTS_RADIUS_OF_EARTH			6371000			/* meters (m)		*/
#define M_TWOPI_F 6.28318530717958647692f
#define M_PI_2_F  1.57079632679489661923f
#define M_RAD_TO_DEG 57.29577951308232087679f
#define M_DEG_TO_RAD 0.01745329251994329576f

typedef struct NED NED;
typedef struct LLARef LLARef;

typedef struct LLA {
    double latitude = NAN, longitude = NAN;
    double altitude = NAN;
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


const int CHANNEL_NONE = 0x8000;
const int CHANNEL_FIRST = 0x8000-1;

typedef struct DatasetChannel {
    Q_GADGET
    Q_PROPERTY(int channelId  MEMBER channel)

public:
    int channel = -1;
    int count = 0;
    double distance_from = NAN;
    double distance_to = NAN;

    XYZ localPosition;

    DatasetChannel() {
    }

    DatasetChannel(int ch) {
        channel = ch;
    }
    void counter() {
        count++;
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
    int groupIndex = 0;
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

typedef QMap<int, ComplexSignal> ComplexSignals;

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

        bool isDist() { return isfinite(distance); }
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

        float getMax() { return max; }
        float getMin() { return min; }
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

        float range() {
            return amplitude.size()*(resolution);
        }
    };




    Epoch();
    void setEvent(int timestamp, int id, int unixt);
    void setChart(int16_t channel, QVector<uint8_t> chartData, float resolution, float offset);
    void setRecParameters(int16_t address, RecordParameters recParams);
    void setDist(int dist);
    void setRangefinder(int channel, float distance);
    void setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void setDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void setPositionLLA(double lat, double lon, LLARef* ref = NULL, uint32_t unix_time = 0, int32_t nanosec = 0);
    void setPositionLLA(Position position);
    void setExternalPosition(Position position);
    void setPositionRef(LLARef* ref);

    void setComplexF(int channel, ComplexSignal signal);
    ComplexSignals complexSignals() { return _complex; }
    ComplexSignal complexSignal(int channel) { return _complex[channel]; }
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

    void setDistProcessing(int16_t channel, float dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setDistance(dist, DistProcessing::DistanceSourceDirectHand);
        }
    }

    void clearDistProcessing(int16_t channel) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.clearDistance(DistProcessing::DistanceSourceDirectHand);
        }
    }

    void setMinDistProc(int16_t channel, float dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setMin(dist, DistProcessing::DistanceSourceConstrainHand);
        }
    }

    void setMaxDistProc(int16_t channel, float dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setMax(dist, DistProcessing::DistanceSourceConstrainHand);
        }
    }

    void setMinMaxDistProc(int16_t channel, int min, int max,  bool is_save = true) {
        if(_charts.contains(channel)) {
            float minsave = _charts[channel].bottomProcessing.getMin();
            float maxsave = _charts[channel].bottomProcessing.getMax();

            _charts[channel].bottomProcessing.setMin(min);
            _charts[channel].bottomProcessing.setMax(max);
            _charts[channel].bottomProcessing.resetDistance();

            if(!is_save) {
                _charts[channel].bottomProcessing.setMin(minsave);
                _charts[channel].bottomProcessing.setMax(maxsave);
            }
        }
    }


    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp_us; }
    int eventUnix() { return _eventUnix; }

    DateTime* time() { return &_time; }

    QVector<uint8_t> chartData(int16_t channel = 0) {
        if(chartAvail(channel)) {
            return _charts[channel].amplitude;
        }
        return QVector<uint8_t>();
    }
    int chartSize(int16_t channel = 0) {
        if(chartAvail(channel)) {
            return _charts[channel].amplitude.size();
        }
        return -1;
    }
    bool chartAvail() { return _charts.size() > 0; }
    bool chartAvail(int16_t channel) {
        if(_charts.contains(channel)) {
            return _charts[channel].amplitude.size() > 0;
        }

        return false;
    }

    Echogram* chart(int16_t channel = 0) {
        if(_charts.contains(channel)) {
            return &_charts[channel];
        }

        return NULL;
    }

    QList<int16_t> chartChannels() {
        return _charts.keys();
    }

    float chartsFullRange(int16_t channel = -1) {
        Q_UNUSED(channel);
        QList<int16_t> charts_channels = chartChannels();

        float full_range  = 0;
        for(uint16_t ch = 0; ch < charts_channels.size(); ch++) {
            int16_t channel = charts_channels[ch];
            full_range += _charts[channel].range();
        }

        return full_range;
    }

    float getMaxRnage(int32_t channel = CHANNEL_FIRST, int32_t channel2 = CHANNEL_NONE) {
        float range = NAN;

        float range1 = NAN;
        float range2 = NAN;
        if(_charts.size() > 0 && channel == CHANNEL_FIRST) {
            range1 = _charts.first().range();
        } else if(_charts.contains(channel)) {
            range1 = _charts[channel].range();
        }

        if(_charts.size() > 0 && channel2 == CHANNEL_FIRST) {
            range2 = _charts.first().range();
        } else if(_charts.contains(channel2)) {
            range2 = _charts[channel2].range();
        }

        if(isfinite(range1)) {
            range = range1;
        }

        if(isfinite(range2)) {
            if(isfinite(range)) {
                if(range < range2) {
                    range = range2;
                }
            } else {
                range = range2;
            }
        }

        if(_rangeFinders.size() > 0) {
            float r1 = _rangeFinders.first();
            if(isfinite(r1) && (r1 > range || !isfinite(range))) {
                range = r1;
            }
        }

        return range;
    }

    bool distAvail() { return flags.distAvail; }

    double  distProccesing(int16_t channel) {
        if(channel == CHANNEL_FIRST) {
            QMapIterator<int16_t, Echogram> i(_charts);
             while (i.hasNext()) {
                 i.next();
                 double distance = i.value().bottomProcessing.getDistance();
                 if(isfinite(distance)) {
                     return distance;
                 }
             }
        } else if(_charts.contains(channel)) {
            return _charts[channel].bottomProcessing.getDistance();
        }

        return NAN;
    }


    float rangeFinder() {
        if(_rangeFinders.size() > 0) {
            return _rangeFinders.first();
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


    bool chartTo(int16_t channel, float start, float end, int16_t* dst, int len, int image_type, bool reverse = false) {
        if(dst == nullptr) {  return false; }

        if(channel == CHANNEL_FIRST && _charts.size() > 0) {
            channel =  _charts.firstKey();
        } else if(!_charts.contains(channel)) {
            memset(dst, 0, len*2);
            return false;
        }

        if(_charts[channel].resolution == 0) {
            memset(dst, 0, len*2);
            return false;
        }

        int raw_size = _charts[channel].amplitude.size();

        if(raw_size == 0) {
            memset(dst, 0, len*2);
            return false;
        }

        uint8_t* src = _charts[channel].amplitude.data();

        if(image_type == 1) {
            if(_charts[channel].compensated.size() == 0) {
                _charts[channel].updateCompesated();
            }
            src = _charts[channel].compensated.data();
        }

        if(raw_size == 0) {
            for(int i_to = 0; i_to < len; i_to++) {
                dst[i_to] = 0;
            }
        }

        start -= _charts[channel].offset;
        end -= _charts[channel].offset;

        float raw_range_f = _charts[channel].range();
        float target_range_f = (float)(end - start);
        float scale_factor = ((float)raw_size/(float)len)*(target_range_f/raw_range_f);
        int offset = start/_charts[channel].resolution;

        int src_start = offset;
        int dir = reverse ? -1 : 1;
        int off = reverse ? (len-1) : 0;
        if(scale_factor >= 0.8f) {
            for(int i_to = 0; i_to < len; i_to++) {
                int src_end = (float)(i_to + 1)*scale_factor + offset;

                int32_t val = 0;
                if(src_start >= 0 && src_start < raw_size) {
                    if(src_end > raw_size) { src_end = raw_size; }

                    val = src[src_start];
                    for(int i = src_start; i < src_end; i++) {
                        val += src[i];
                    }
                    val /= 1 + (src_end - src_start);
                }

                src_start = src_end;
                dst[off + dir*i_to] = val;
            }
        } else {
            for(int i_to = 0; i_to < len; i_to++) {
                float cell_offset = (float)(i_to)*scale_factor + (float)offset + 0.5f;
                int src_start = int(cell_offset);
                int src_end = src_start + 1;

                int32_t val = 0;
                if(src_start >= 0 && src_start < raw_size) {
                    if(src_end >= raw_size) { src_end = raw_size-1; }

                    float coef = cell_offset - floorf(cell_offset);
                    val = (float)src[src_start]*(1 - coef) + (float)src[src_end]*coef;
                }

                dst[off + dir*i_to] = val;
            }
        }

        return true;
    }

    void moveComplexToEchogram(float offset_m, float levels_offset_db);

    void setInterpNED(NED ned);
    void setInterpYaw(float yaw);
    void setInterpFirstChannelDist(float dist);
    void setInterpSecondChannelDist(float dist);
    NED   getInterpNED() const;
    float getInterpYaw() const;
    float getInterpFirstChannelDist() const;
    float getInterpSecondChannelDist() const;

    bool getWasValidlyRenderedInEchogram() const;
    void setWasValidlyRenderedInEchogram(bool state);

    void setResolution(int16_t channelId, uint16_t resolution);
    void setChartCount(int16_t channelId, uint16_t chartCount);
    void setOffset(int16_t channelId, uint16_t offset);
    void setFrequency(int16_t channelId, uint16_t frequency);
    void setPulse(int16_t channelId, uint8_t pulse);
    void setBoost(int16_t channelId, uint8_t boost);
    void setSoundSpeed(int16_t channelId, uint32_t soundSpeed);
    uint16_t getResolution(int16_t channelId) const;
    uint16_t getChartCount(int16_t channelId) const;
    uint16_t getOffset(int16_t channelId) const;
    uint16_t getFrequency(int16_t channelId) const;
    uint8_t getPulse(int16_t channelId) const;
    uint8_t getBoost(int16_t channelId) const;
    uint32_t getSoundSpeed(int16_t channelId) const;

    Contact contact_; // TODO: private

protected:
    QMap<int16_t, Echogram> _charts; // channels not addr
    QMap<int16_t, float> _rangeFinders;

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

    float m_temp_c = 0;

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
    bool wasValidlyRenderedInEchogram_;
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

    void setState(DatasetState state);

#if defined(FAKE_COORDS)
    void setActiveZeroing(bool state);
#endif

    DatasetState getState() const;
    LLARef getLlaRef() const;
    void setLlaRef(const LLARef& val, LlaRefState state);

    inline int size() const {

        return _pool.size();

    }

    Epoch* fromIndex(int index_offset = 0) {
        int index = validIndex(index_offset);
        if(index >= 0) {
            return &_pool[index];
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

    int endIndex() {
        return size() - 1;
    }

    int validIndex(int index_offset = 0) {
        int index = index_offset;
        if(index >= size()) { index = endIndex(); }
        else if(index < 0) { index = -1; }
        return index;
    }

    void getMaxDistanceRange(float* from, float* to, int channel1, int channel2 = CHANNEL_NONE);

    QMap<int, DatasetChannel> channelsList() {
        return _channelsSetup;
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

    BottomTrackParam* getBottomTrackParamPtr() {
        return &bottomTrackParam_;
    }

public slots:
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float angle1_deg, float angle2_deg = NAN, float angle3_deg = NAN);
    void addTimestamp(int timestamp);

    //
    void setChartSetup(int16_t channel, uint16_t resol, uint16_t count, uint16_t offset);
    void setTranscSetup(int16_t channel, uint16_t freq, uint8_t pulse, uint8_t boost);
    void setSoundSpeed(int16_t channel, uint32_t soundSpeed);
    void setFixBlackStripesState(bool state);
    void setFixBlackStripesRange(int val);
    void addChart(ChartParameters, QVector<uint8_t> data, float resolution, float offset);
    void rawDataRecieved(RawData raw_data);
    void addDist(int dist);
    void addRangefinder(float distance);
    void addUsblSolution(IDBinUsblSolution::UsblSolution data);
    void addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void addDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void addAtt(float yaw, float pitch, float roll);
    void addPosition(double lat, double lon, uint32_t unix_time = 0, int32_t nanosec = 0);

    void addGnssVelocity(double h_speed, double course);

//    void addDateTime(int year, );
    void addTemp(float temp_c);

    void mergeGnssTrack(QList<Position> track);

    void resetDataset();
    void resetDistProcessing();

    void setChannelOffset(int channal, float x, float y, float z) {
        if(_channelsSetup.contains(channal)) {
            _channelsSetup[channal].localPosition.x = x;
            _channelsSetup[channal].localPosition.y = y;
            _channelsSetup[channal].localPosition.z = z;
        }
    }

    void bottomTrackProcessing(int channel1, int channel2);
    void spatialProcessing();
    void emitPositionsUpdated() {
        emit bottomTrackUpdated(0, endIndex());
        emit boatTrackUpdated();
    }


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
    void interpolateData(bool fromStart);

signals:
    void channelsListUpdates(QList<DatasetChannel> channels);
    void dataUpdate();
    void bottomTrackUpdated(int lEpoch, int rEpoch);
    void boatTrackUpdated();
    void updatedInterpolatedData(int indx);
    void updatedLlaRef();
    void channelsUpdated();

protected:
    using EthalonVec = QVector<QPair<uint8_t, uint8_t>>;
    QMap<int16_t, EthalonVec> ethData_; // first - channelId, sec - vec,
    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

#if defined(FAKE_COORDS)
    bool activeZeroing_ = false;
    uint64_t testTime_ = 1740466541;
#endif

    QMap<int, DatasetChannel> _channelsSetup;

    void validateChannelList(int ch) {
        bool isNewChannel = !_channelsSetup.contains(ch);

        _channelsSetup[ch].channel = ch;
        _channelsSetup[ch].counter();

        if (isNewChannel) {
            emit channelsUpdated();
        }
    }

    QVector<QVector3D> _boatTrack;
    QHash<int, int> selectedBoatTrackVertexIndices_; // first - vertice indx, second - epoch indx
    QVector<QVector3D> _beaconTrack;
    QVector<QVector3D> _beaconTrack1;

    QMap<int, UsblView::UsblObjectParams> tracks;

    enum {
        AutoRangeNone,
        AutoRangeLast,
        AutoRangeMax,
        AutoRangeMaxVis
    } _autoRange = AutoRangeLast;


    QVector<Epoch> _pool;

    float lastTemperature = 0;

    float _lastYaw = 0, _lastPitch = 0, _lastRoll = 0;
    Position _lastPositionGNSS;

    Epoch* addNewEpoch() {
        _pool.resize(_pool.size() + 1);
        auto* lastEpoch = last();
        return lastEpoch;
    }

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
        int firstChannelId_;
        int secondChannelId_;
    };

    /*methods*/
    LlaRefState getCurrentLlaRefState() const;

    /*data*/
    LLARef _llaRef;
    LlaRefState llaRefState_ = LlaRefState::kUndefined;
    DatasetState state_ = DatasetState::kUndefined;
    Interpolator interpolator_;
    int lastBoatTrackEpoch_;
    int lastBottomTrackEpoch_;
    BottomTrackParam bottomTrackParam_;
    uint64_t boatTrackValidPosCounter_;
    QMap<int16_t, RecordParameters> usingRecordParameters_;
    bool fixBlackStripesState_;
    int fixBlackStripesWrCnt_;
};

#endif // PLOT_CASH_H

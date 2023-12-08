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

#include <3Plot.h>
#include <IDBinnary.h>
#include <bottomtrackprovider.h>

#include "time.h"

#if defined(Q_OS_ANDROID)
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


typedef struct {
    double latitude = NAN, longitude = NAN;
    double altitude = NAN;
    bool isValid() {
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
    bool isInit = false;

    LLARef() {}

    LLARef(LLA lla) {
        refLatRad = lla.latitude * M_DEG_TO_RAD;
        refLonRad= lla.longitude * M_DEG_TO_RAD;
        refLatSin = sin(refLatRad);
        refLatCos = cos(refLatRad);
        isInit = true;
    }
} LLARef;

typedef struct NED {
    double n = NAN, e = NAN, d = NAN;
    NED() {}

    NED(LLA* lla, LLARef* ref) {
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

    bool isValid() {
        return isfinite(n) && isfinite(e) && isfinite(d);
    }

    bool isCoordinatesValid() {
        return isfinite(n) && isfinite(e);
    }
} NED;

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

typedef enum BottomTrackPreset {
    BottomTrackOneBeam,
    BottomTrackOneBeamNarrow,
    BottomTrackSideScan
} BottomTrackPreset;

typedef struct {
    float gainSlope = 1.0;
    float threshold = 1.0;
    float verticalGap = 0;
    float minDistance = 0;
    float maxDistance = 1000;

    int indexFrom = 0;
    int indexTo = 0;
    int windowSize = 1;

    BottomTrackPreset preset = BottomTrackOneBeam;

    struct {
        float x = 0, y = 0, z = 0;
    } offset;
} BottomTrackParam;

class Epoch {
public:
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

    typedef struct {
        QVector<uint8_t> amplitude;
        float resolution = 0; // m
        float offset = 0; // m
        int type = 0;

        QVector<int16_t> visual;

        DistProcessing bottomProcessing;
        Position sensorPosition;

        float range() {
            return amplitude.size()*(resolution);
        }

    } Echogram;



    Epoch();
    void setEvent(int timestamp, int id, int unixt);
    void setEncoder(float encoder);
    void setChart(int16_t channel, QVector<uint8_t> chartData, float resolution, int offset);
    void setIQ(QByteArray data, uint8_t type);
    void setDist(int dist);
    void setRangefinder(int channel, float distance);
    void setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void setDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void setPositionLLA(double lat, double lon, LLARef* ref = NULL, uint32_t unix_time = 0, int32_t nanosec = 0);
    void setExternalPosition(Position position);

    void setGnssVelocity(double h_speed, double course);

    void setTime(DateTime time);
    void setTime(int year, int month, int day, int hour, int min, int sec, int nanosec = 0);

    void setTemp(float temp_c);
    void setEncoders(int16_t enc1, int16_t enc2 = 0xFFFF, int16_t enc3 = 0xFFFF, int16_t = 0xFFFF, int16_t = 0xFFFF, int16_t enc6 = 0xFFFF);
    void setAtt(float yaw, float pitch, float roll);

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

    QByteArray iqData() { return _iq;}
    bool isIqAvail() { return flags.iqAvail; }


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

    double lat() { return _positionGNSS.lla.latitude; }
    double lon() { return _positionGNSS.lla.longitude; }

    Position getPositionGNSS() { return _positionGNSS; }
    Position getExternalPosition() { return _positionExternal; }

    uint32_t positionTimeUnix() { return _positionGNSS.time.sec; }
    uint32_t positionTimeNano() { return _positionGNSS.time.nanoSec; }
    DateTime* positionTime() {return &_positionGNSS.time; }

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

        uint8_t* src = NULL;

        if(image_type == 1 && _charts[channel].visual.size() > 0) {
//            src = _charts[channel].visual.data();
            src = _charts[channel].amplitude.data();
        } else {
            src = _charts[channel].amplitude.data();
        }

        if(raw_size == 0) {
            for(int i_to = 0; i_to < len; i_to++) {
                dst[i_to] = 0;
            }
        }

//        if(m_chartResol == 0) { m_chartResol = 1; }

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




protected:

    QMap<int16_t, Echogram> _charts;
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

    QByteArray _iq;
    uint8_t _iq_type = 0;

    IDBinDVL::BeamSolution _dopplerBeams[4];
    uint16_t _dopplerBeamCount = 0;

    IDBinDVL::DVLSolution _dvlSolution;

    Position _positionGNSS;
    Position _positionExternal;

    struct {
        double hspeed = NAN;
        double course = NAN;
    } _GnssData;

    float m_temp_c = 0;

    struct {
        uint16_t validMask = 0;
        int16_t e1 = 0;
        int16_t e2 = 0;
        int16_t e3 = 0;
        int16_t e4 = 0;
        int16_t e5 = 0;
        int16_t e6 = 0;
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
        bool iqAvail;
        bool isDVLSolutionAvail = false;

    } flags;
};

class Dataset : public QObject {
    Q_OBJECT
public:
    Dataset();

    inline int size() const { return _pool.size(); }

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
        return NULL;
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

    void setBottomTrackProvider(std::shared_ptr <BottomTrackProvider> bottomTrackProvider);

public slots:
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float encoder);
    void addTimestamp(int timestamp);
    void addChart(int16_t channel, QVector<uint8_t> data, float resolution, float offset);
    void addIQ(QByteArray data, uint8_t type);
    void addDist(int dist);
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

    void bottomTrackProcessing(int channel1, int channel2, BottomTrackParam param);
    void spatialProcessing();

//    void set3DRender(FboInSGRenderer* render) {
//        _render3D = render;
//    }
//    void updateRender3D() {
        // deprecated
//        if(_render3D != NULL) {
//            _render3D->updateBottomTrack(_bottomTrack);
//        }
//    }

    void clearTrack();
    void updateTrack(bool update_all = false);

    QStringList channelsNameList();


private:
    std::shared_ptr <BottomTrackProvider> mpBottomTrackProvider;

signals:
    void channelsListUpdates(QList<DatasetChannel> channels);
    void dataUpdate();

protected:
    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

    QMap<int, DatasetChannel> _channelsSetup;

    void validateChannelList(int ch) {
        _channelsSetup[ch].channel = ch;
        _channelsSetup[ch].counter();
    }

    LLARef _llaRef;

    int _lastTrackEpoch = 0;

    QMap<int, QVector<QVector3D>> _bottomTracks;

    QVector<QVector3D> _boatTrack;


//    FboInSGRenderer* _render3D;

    enum {
        AutoRangeNone,
        AutoRangeLast,
        AutoRangeMax,
        AutoRangeMaxVis
    } _autoRange = AutoRangeLast;


    QVector<Epoch> _pool;

    float lastTemperature = 0;

    float _lastYaw = 0, _lastPitch = 0, _lastRoll = 0;


    void makeNewEpoch() {
        _pool.resize(_pool.size() + 1);
    }
};

#endif // PLOT_CASH_H

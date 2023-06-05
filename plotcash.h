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

#define CONSTANTS_RADIUS_OF_EARTH			6371000			/* meters (m)		*/
#define M_TWOPI_F 6.28318530717958647692f
#define M_PI_2_F  1.57079632679489661923f
#define M_RAD_TO_DEG 57.29577951308232087679f
#define M_DEG_TO_RAD 0.01745329251994329576f

typedef struct  {
    double refLatSin;
    double refLatCos;
    double refLatRad;
    double refLonRad;
    bool isInit = false;
} LLARef;


typedef struct {
    enum {
        invisible = 0,
        forward,
        reverse
    } direction;
} VisualChannelSetup;

class PoolDataset {
public:
    typedef struct {
        typedef enum {
            DistanceSourceNone = 0,
            DistanceSourceProcessing,
            DistanceSourceHand,
            DistanceSourceLoad
        } DistanceSource;

        QVector<int16_t> data;
        float distance = NAN;
        float min = NAN;
        float max = NAN;
        DistanceSource source = DistanceSourceNone;

        bool isData() const { return data.size() > 0; }
        bool isDist() { return isfinite(distance); }
        void setDistance(float dist, DistanceSource src = DistanceSourceNone) { distance = dist; source = src; }
        void resetDistance() { distance = NAN; source = DistanceSourceNone; }
        float getDistance() { return distance; }

        void setMin(float val) {
            min = val;
            if(max != NAN && val + 0.05 > max) {
                max = val + 0.05;
            }
        }
        void setMax(float val) {
            max = val;
            if(min != NAN && val - 0.05 < min) {
                min = val - 0.05;
            }
        }

        float getMax() { return max; }
        float getMin() { return min; }
    } DistProcessing;

    typedef struct {
        QVector<int16_t> data;
        int resolution = 0;
        int offset = 0;
        int type = 0;
        QVector<int16_t> visual;

        DistProcessing bottomProcessing;
    } DataChart;

    PoolDataset();
    void setEvent(int timestamp, int id, int unixt);
    void setEncoder(float encoder);
    void setChart(int16_t channel, QVector<int16_t> chartData, int resolution, int offset);
    void setIQ(QByteArray data, uint8_t type);
    void setDist(int dist);
    void setRangefinder(int channel, float distance);
    void setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void setDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void setPositionLLA(double lat, double lon, LLARef* ref = NULL, uint32_t unix_time = 0, int32_t nanosec = 0);
    void setTemp(float temp_c);
    void setEncoders(int16_t enc1, int16_t enc2 = 0xFFFF, int16_t enc3 = 0xFFFF, int16_t = 0xFFFF, int16_t = 0xFFFF, int16_t enc6 = 0xFFFF);
    void setAtt(float yaw, float pitch, float roll);

    void setDistProcessing(int16_t channel, float dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setDistance(dist, DistProcessing::DistanceSourceHand);
        }
    }

    void setMinDistProc(int16_t channel, int dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setMin(dist);
        }
        doBottomTrack(-1, false);
    }

    void setMaxDistProc(int16_t channel, int dist) {
        if(_charts.contains(channel)) {
            _charts[channel].bottomProcessing.setMax(dist);
        }
        doBottomTrack(-1, false);
    }

    void setMinMaxDistProc(int16_t channel, int min, int max,  bool is_save = true) {
        if(_charts.contains(channel)) {
            float minsave = _charts[channel].bottomProcessing.getMin();
            float maxsave = _charts[channel].bottomProcessing.getMax();

            _charts[channel].bottomProcessing.setMin(min);
            _charts[channel].bottomProcessing.setMax(max);
            _charts[channel].bottomProcessing.resetDistance();

            doBottomTrack(-1, false);

            if(!is_save) {
                _charts[channel].bottomProcessing.setMin(minsave);
                _charts[channel].bottomProcessing.setMax(maxsave);
            }
        }
    }

    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp; }
    int eventUnix() { return _eventUnix; }

    QVector<int16_t> chartData(int16_t channel = 0) {
        if(chartAvail(channel)) {
            return _charts[channel].data;
        }
        return QVector<int16_t>();
    }
    bool chartAvail() { return _charts.size() > 0; }
    bool chartAvail(int16_t channel) {
        if(_charts.contains(channel)) {
            return _charts[channel].data.size() > 0;
        }

        return false;
    }

    QList<int16_t> chartChannels() {
        return _charts.keys();
    }

    QByteArray iqData() { return _iq;}
    bool isIqAvail() { return flags.iqAvail; }


    int distData() { return m_dist; }
    bool distAvail() { return flags.distAvail; }

    float distProccesing(int16_t channel) {
        if(_charts.contains(channel)) {
            return _charts[channel].bottomProcessing.getDistance();
        }

        return NAN;
    }
    bool distProccesingAvail(int16_t channel) {
        if(_charts.contains(channel)) {
            return _charts[channel].bottomProcessing.isData();
        }

        return false;
    }

    float temperature() { return m_temp_c; }
    bool temperatureAvail() { return flags.tempAvail; }

    bool isAttAvail() { return _attitude.is_avail; }

    bool isDopplerAvail() { return doppler.isAvai; }
    float dopplerX() { return doppler.velocityX; }

    bool isDopplerBeamAvail() { return _dopplerBeamCount > 0; }
    IDBinDVL::BeamSolution dopplerBeam(uint16_t num) { return _dopplerBeams[num]; }
    uint16_t dopplerBeamCount() {return _dopplerBeamCount; }

    IDBinDVL::DVLSolution dvlSolution() { return _dvlSolution; }
    bool isDVLSolutionAvail() {  return flags.isDVLSolutionAvail; }

    double lat() { return m_position.lat; }
    double lon() { return m_position.lon; }

    uint32_t positionTimeUnix() { return m_position.unixTime; }
    uint32_t positionTimeNano() { return m_position.nanoSec; }

    double relPosN() { return m_position.N; }
    double relPosE() { return m_position.E; }
    double relPosD() { return (double)0*0.001; } //!ERROR add dist

    bool isPosAvail() { return flags.posAvail; }


    void doBottomTrack(int track_type, bool is_update_dist) {
//        if(track_type >= 0) {
//            _procDistType = track_type;
//        }

//        QMutableHashIterator<int16_t, DataChart> i(_charts);
//        while (i.hasNext()) {
//            i.next();

//            if(_procDistType == 0) {
//                doBottomTrack2D(i.value(), is_update_dist);
//            } else if(_procDistType == 1) {
//                doBottomTrackSideScan(i.value(), is_update_dist);
//            }
//        }

    }


    void doBottomTrack2D(DataChart &chart, bool is_update_dist = false);
    void doBottomTrackSideScan(DataChart &chart, bool is_update_dist = false);

    bool edgeProcAvail = false;

    void doEdgeProccesing(DataChart &chart) {
//        int raw_size = _chartData.size();
//        int16_t* src = _chartData.data();

//        if(raw_size != 0 && !edgeProcAvail) {
//            m_processingEdgeData.resize(raw_size);
//            int16_t* procData = m_processingEdgeData.data();

//            float max_of_start = 0;
//            for(int i = 0; i < 10; i ++) {
//                float val = src[i];

//                if(val > max_of_start) {
//                    max_of_start = val;
//                }

//                procData[i] = val;

//                if(procData[i] < 0) {
//                    procData[i] = 0;
//                } else if(procData[i] > 255) {
//                    procData[i] = 255;
//                }
//            }

//            float avrg = max_of_start*1.f;
//            for(int i = 0; i < raw_size; i ++) {
//                float val = src[i];

//                avrg += (val - avrg)*(0.05f + avrg*0.0006);
//                procData[i] = (val - avrg*0.55f)*(0.9f +float(i*_chartResol)*0.000025f)*1.4f;

//                if(procData[i] < 0) {
//                    procData[i] = 0;
//                } else if(procData[i] > 255) {
//                    procData[i] = 255;
//                }
//            }

//            edgeProcAvail = true;
//        }
    }

    void resetDistProccesing() {
//        flags.processDistAvail = false;
    }

    void nedProcessing(LLARef* ref) {
        double lat_rad = m_position.lat * M_DEG_TO_RAD;
        double lon_rad = m_position.lon * M_DEG_TO_RAD;

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

        m_position.N = k * (ref->refLatCos * sin_lat - ref->refLatSin * cos_lat * cos_d_lon) * CONSTANTS_RADIUS_OF_EARTH;
        m_position.E = k * cos_lat * sin(lon_rad - ref->refLonRad) * CONSTANTS_RADIUS_OF_EARTH;
    }

    void chartTo(int16_t channel, int start, int end, int16_t* dst, int len, int image_type, bool reverse = false) {
        if(dst == nullptr) {  return; }
        if(!_charts.contains(channel)) { return;}
        if(_charts[channel].resolution == 0) { return; }

        int raw_size = _charts[channel].data.size();

        int16_t* src;

        if(image_type == 1 && !edgeProcAvail) {
//            doEdgeProccesing();
        }

        if(image_type == 1 && _charts[channel].visual.size() > 0) {
            src = _charts[channel].visual.data();
        } else {
            src = _charts[channel].data.data();
        }

        if(raw_size == 0) {
            for(int i_to = 0; i_to < len; i_to++) {
                dst[i_to] = 0;
            }
        }

//        if(m_chartResol == 0) { m_chartResol = 1; }

        float raw_range_f = (float)(raw_size*_charts[channel].resolution);
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

    }

protected:




    QHash<int16_t, DataChart> _charts;
    QHash<int16_t, float> _rangeFinders;

    int m_dist;
    int _eventTimestamp = 0;
    int _eventUnix = 0;
    int _eventId = 0;

    struct {
        float yaw = 0, pitch = 0, roll = 0;
        bool is_avail = false;
    } _attitude;

    QByteArray _iq;
    uint8_t _iq_type = 0;

    IDBinDVL::BeamSolution _dopplerBeams[4];
    uint16_t _dopplerBeamCount = 0;

    IDBinDVL::DVLSolution _dvlSolution;

    struct {
        uint32_t unixTime = 0;
        int32_t nanoSec = 0;
        double lat = 0, lon = 0;
        float N = 0, E = 0, D = 0;
    } m_position;

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

class PlotCash : public QObject {
    Q_OBJECT
public:
    PlotCash();

    enum ThemeId {
        ClassicTheme,
        SepiaTheme,
        WRGBDTheme,
        WBTheme,
        BWTheme
    };

    Q_PROPERTY(int themeId WRITE setThemeId)
    Q_PROPERTY(int imageType WRITE setImageType)
    Q_PROPERTY(int bottomTrackTheme WRITE setBottomTrackTheme)


    int poolSize();

    PoolDataset* fromPool(int index_offset = 0) {
        int index = poolIndex(index_offset);
//        qInfo("pool index %u", index);
        return &_pool[index];
    }

    QImage getImage(QSize size);

    //! Установить указатель на модель 3D - сцены
    void set3DSceneModel(const ModelPointer pModel);

public slots:
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float encoder);
    void addTimestamp(int timestamp);
    void addChart(int16_t channel, QVector<int16_t> data, int resolution, int offset);
    void addIQ(QByteArray data, uint8_t type);
    void addDist(int dist);
    void addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void addDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void addAtt(float yaw, float pitch, float roll);
    void addPosition(double lat, double lon, uint32_t unix_time = 0, int32_t nanosec = 0);
    void addTemp(float temp_c);
    void setStartLevel(int level);
    void setStopLevel(int level);
    void setTimelinePosition(double position);
    void scrollTimeline(int delta);
    void verZoom(int delta);
    void verScroll(int delta);
    void setMouse(int x, int y);
    void setMouseMode(int mode);
    void setChartVis(bool visible);
    void setOscVis(bool visible);
    void setDistVis(bool visible);
    void setDistProcVis(bool visible);
    void setEncoderVis(bool visible);
    void setVelocityVis(bool visible);
    void setVelocityRange(float range);
    void setDopplerBeamVis(bool visible, int beamFilter, bool is_mode_visible, bool is_amp_visible);
    void setDopplerInstrumentVis(bool visible);
    void setGridNumber(int number);
    void setImageType(int image_type);
    void setBottomTrackTheme(int bottomThemetrack_type);
    void setAHRSVis(bool visible);
    void updateImage(bool update_value = false);
    void renderValue();
    void resetValue();
    void resetDataset();
    void doDistProcessing();
    void doDistProcessing(int source_type, int window_size, float vertical_gap, float range_min, float range_max);
    void resetDistProcessing();

    void setThemeId(int theme_id);

    void set3DRender(FboInSGRenderer* render) {
        _render3D = render;
        _render3D->setModel(mp3DSceneModel);
    }
    void updateRender3D() {
        if(_render3D != NULL) {
            _render3D->updateBottomTrack(_bottomTrack);
        }
    }
    void updateBottomTrack(bool update_all = false);

private:

    //! Указатель на модель 3D - сцены
    ModelPointer mp3DSceneModel;

signals:
    void updatedImage();

protected:
    int m_verticalGridNum = 20;
    float m_legendMultiply = 0.001f;
    int m_range = 2000;
    float _velocityRange = 2.0f;
    int m_offset = 0;
    int m_startLevel = 10;
    int m_stopLevel = 100;
    int m_offsetLine = 0;
    bool m_chartVis = true;
    bool m_oscVis = false;
    bool m_distSonarVis = true;
    bool m_distProcessingVis = true;
    bool m_TemperatureVis = true;
    bool m_DopplerVis = true;
    bool m_distCalcVis = true;
    bool _is_attitudeVis = false;
    bool _is_encoderVis = false;
    bool _is_velocityVis = false;
    bool _isDopplerInstrimentVis = false;
    bool _isDopplerBeamVis = false;
    int _dopplerBeamFilter = 0xF;
    bool _isDopplerBeamAmpitudeVisible = true;
    bool _isDopplerBeamModeVisible = true;
    bool _isDopplerBeamDistVisible = true;
    bool isDistProcessing = false;
    int _themId;
    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

    int _mouse_x = -1, _mouse_y = -1;
    int _mouse_mode = 1;

    int _imageType = 0;
    int _bottomTrackTheme = 0;

    int _bottomtrackType = -1;
    QVector<int32_t> _bottomTrackWindow;
    int _bottomTrackLastIndex = 0;
    int _bottomTrackLastProcessing = 0;
    int _bottomTrackWindowSize = 0;
    float _bottomTrackVerticalGap = 0;
    float _bottomTrackMinRange = 0;
    float _bottomTrackMaxRange = 0;



    QList<VisualChannelSetup> _channelsSetup;

    LLARef _llaRef;

    QVector<uint32_t> _gnssTrackIndex;
    QVector<QVector3D> _bottomTrack;
    QVector<QVector3D> _boatTrack;
    FboInSGRenderer* _render3D;

    enum {
        AutoRangeNone,
        AutoRangeLast,
        AutoRangeMax,
        AutoRangeMaxVis
    } _autoRange = AutoRangeLast;


    QVector<PoolDataset> _pool;


    typedef struct {
        QVector<int16_t> chartData;
        int distData = -1;
        int processingDistData = -1;
        int poolIndex = -1;
        bool poolIndexUpdate = true;

    } ValueCash;
    int m_valueCashStart = 0;

    QVector<ValueCash> m_valueCash;
    ValueCash m_prevValueCash;
    int m_valueIndex;
    QVector<QColor> m_colorMap;
    uint16_t m_colorHashMap[256];
    uint16_t m_colorDist = 0xFFFF;
    uint16_t m_colorDistProc = 0x3FF0;
    QImage m_image;
    uint16_t m_dataImage[3000*3000];
    int m_prevLineWidth = 30;

    float lastTemperature = 0;

    float _lastYaw = 0, _lastPitch = 0, _lastRoll = 0;

    struct  {
        bool resetValue;
        bool renderValue;
        bool renderImage;
    } flags;

    void updateValueMap(int width, int height);
    void updateImage(int width, int height);

    void poolAppend() {
        _pool.resize(_pool.size() + 1);
    }

    int poolLastIndex() {
        return poolSize() - 1;
    }

    int poolIndex(int index_offset = 0) {
        int index = index_offset;
        if(index >= poolSize()) { index = poolLastIndex(); }
        else if(index < 0) { index = -1; }
        return index;
    }

    void setColorScheme(QVector<QColor> coloros, QVector<int> levels);
};

#endif // PLOT_CASH_H

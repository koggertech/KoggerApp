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


class PoolDataset {
public:
    PoolDataset();
    void setEvent(int timestamp, int id, int unixt);
    void setEncoder(float encoder);
    void setChart(QVector<int16_t> chartData, int resolution, int offset);
    void setIQ(QByteArray data, uint8_t type);
    void setDist(int dist);
    void setRangefinder(int channel, float distance);
    void setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void setDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void setPositionLLA(double lat, double lon, LLARef* ref = NULL, uint32_t unix_time = 0, int32_t nanosec = 0);
    void setTemp(float temp_c);
    void setEncoders(int16_t enc1, int16_t enc2 = 0xFFFF, int16_t enc3 = 0xFFFF, int16_t = 0xFFFF, int16_t = 0xFFFF, int16_t enc6 = 0xFFFF);
    void setAtt(float yaw, float pitch, float roll);

    void setDistProcessing(int dist) {
        flags.processDistAvail = true;
        m_processingDist = dist;
    }

    void setMinDistProc(int dist) {
        _procMinDist = dist;
        if(dist + 50 > _procMaxDist) {
            _procMaxDist = dist + 50;
        }
        flags.processDistAvail = false;
        doBottomTrack(-1, false);
    }

    void setMaxDistProc(int dist) {
        _procMaxDist = dist;
        if(dist - 50 < _procMinDist) {
            _procMinDist = dist - 50;
        }
        flags.processDistAvail = false;
        doBottomTrack(-1, false);
    }

    void setMinMaxDistProc(int min, int max,  bool is_save = true) {
        int minsave = _procMinDist;
        int maxsave = _procMaxDist;

        _procMinDist = min;
        _procMaxDist = max;

        flags.processDistAvail = false;
        doBottomTrack(-1, false);

        if(!is_save) {
            _procMinDist = minsave;
            _procMaxDist = maxsave;
        }
    }

    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp; }
    int eventUnix() { return _eventUnix; }

    QVector<int16_t> chartData() { return m_chartData; }
    bool chartAvail() { return flags.chartAvail; }

    QByteArray iqData() { return _iq;}
    bool isIqAvail() { return flags.iqAvail; }


    int distData() { return m_dist; }
    bool distAvail() { return flags.distAvail; }

    int distProccesing() { return m_processingDist; }
    bool distProccesingAvail() { return flags.processDistAvail; }

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
    double relPosD() { return (double)m_processingDist*0.001; }

    bool isPosAvail() { return flags.posAvail; }


    void doBottomTrack(int track_type, bool is_update_dist) {
        if(track_type >= 0) {
            _procDistType = track_type;
        }
        if(_procDistType == 0) {
            doBottomTrack2D(is_update_dist);
        } else if(_procDistType == 1) {
            doBottomTrackSideScan(is_update_dist);
        }
    }
    void doBottomTrack2D(bool is_update_dist = false);
    void doBottomTrackSideScan(bool is_update_dist = false);

    bool edgeProcAvail = false;

    void doEdgeProccesing() {
        int raw_size = m_chartData.size();
        int16_t* src = m_chartData.data();

        if(raw_size != 0 && !edgeProcAvail) {
            m_processingEdgeData.resize(raw_size);
            int16_t* procData = m_processingEdgeData.data();

            float max_of_start = 0;
            for(int i = 0; i < 10; i ++) {
                float val = src[i];

                if(val > max_of_start) {
                    max_of_start = val;
                }

                procData[i] = val;

                if(procData[i] < 0) {
                    procData[i] = 0;
                } else if(procData[i] > 255) {
                    procData[i] = 255;
                }
            }

            float avrg = max_of_start*1.f;
            for(int i = 0; i < raw_size; i ++) {
                float val = src[i];

                avrg += (val - avrg)*(0.05f + avrg*0.0006);
                procData[i] = (val - avrg*0.55f)*(0.9f +float(i*m_chartResol)*0.000025f)*1.4f;

                if(procData[i] < 0) {
                    procData[i] = 0;
                } else if(procData[i] > 255) {
                    procData[i] = 255;
                }
            }

            edgeProcAvail = true;
        }
    }

    void resetDistProccesing() {
        flags.processDistAvail = false;
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

    float dopplerProcessing(const int32_t w_size, const int32_t w_size2, int decm) {
        const int data_size = iqData().size()/4;
        const int16_t* iq_data = (const int16_t*)(iqData().constData());

        const int chart_size = data_size - w_size2*2;

        float amp_max = 0;
        float ci_max = 0, cq_max = 0, c2q_max = 0, c2i_max = 0;

        for (int row = w_size2*2; row < chart_size; row+=1) {
            const uint32_t row_ind = row*2;
            float ci = 0, cq = 0, c2i = 0, c2q = 0;

            for(uint16_t i = 0; i < w_size-16; i+=1) {
                const uint32_t r_ind = row_ind + i*2;
                int32_t r11i = iq_data[r_ind], r11q = iq_data[r_ind+1];
                int32_t r12i = iq_data[r_ind+w_size*2], r12q = iq_data[r_ind+1+w_size*2];
                int32_t r21i = iq_data[r_ind+w_size2*2], r21q = iq_data[r_ind+1+w_size2*2];
                int32_t r22i = iq_data[r_ind+(w_size2+w_size)*2], r22q = iq_data[r_ind+1+(w_size2+w_size)*2];

                ci += (int64_t)(r11i*r12i) + (int64_t)(r11q*r12q);
                cq += (int64_t)(r11q*r12i) - (int64_t)(r11i*r12q);
                ci += (int64_t)(r21i*r22i) + (int64_t)(r21q*r22q);
                cq += (int64_t)(r21q*r22i) - (int64_t)(r21i*r22q);

                c2i += (int64_t)(r11i*r21i) + (int64_t)(r11q*r21q);
                c2q += (int64_t)(r11q*r21i) - (int64_t)(r11i*r21q);
                c2i += (int64_t)(r12i*r22i) + (int64_t)(r12q*r22q);
                c2q += (int64_t)(r12q*r22i) - (int64_t)(r12i*r22q);
            }

            float amp = ci*ci+cq*cq + c2i*c2i+c2q*c2q;

            if(amp_max < amp) {
                amp_max = amp;
                ci_max = ci;
                cq_max = cq;
                c2i_max = c2i;
                c2q_max = c2q;
            }
        }

        float velocity = NAN;
        if(amp_max > 10000000000) {
            float speed = atan2f(cq_max, ci_max)*1500.0f/(4.0f*3.141592f*float(w_size*decm));
            float speed2 = atan2f(c2q_max, c2i_max)*1500.0f/(4.0f*3.141592f*float(w_size2*decm));

            float speed_dif = speed - speed2;
            const float resolver[] = {1.30208333f, -1.822916666, -0.5208333333, 0.78125, -1.30208333f, 1.822916666, 0.5208333333, -0.78125};
            const float corrector[] = {1.30208333f, 1.30208333f, 1.30208333f, 1.30208333f, -1.30208333f, -1.30208333f, -1.30208333f, -1.30208333f};

            float min = fabs(speed_dif);
            int32_t min_ind = -1;
            for(uint32_t i = 0; i < sizeof(resolver)/4; i++) {
                float absdif = fabs(speed_dif - resolver[i]);
                if(min > absdif) {
                    min = absdif;
                    min_ind = i;
                }
            }

            if(min_ind >= 0) {
                speed2 += corrector[min_ind];
            }

            velocity = speed2;
        }

        return velocity;
    }

    void chartTo(int start, int end, int16_t* dst, int len, int image_type) {
        if(dst == nullptr) {  return; }
        if(m_chartResol == 0) { return; }
        int raw_size = m_chartData.size();

        int16_t* src;

        if(image_type == 1 && !edgeProcAvail) {
            doEdgeProccesing();
        }

        if(image_type == 1 && edgeProcAvail) {
            src = m_processingEdgeData.data();
        } else {
            src = m_chartData.data();
//            src = m_processingDistData.data();
        }

        if(raw_size == 0) {
            for(int i_to = 0; i_to < len; i_to++) {
                dst[i_to] = 0;
            }
        }

//        if(m_chartResol == 0) { m_chartResol = 1; }

        float raw_range_f = (float)(raw_size*m_chartResol);
        float target_range_f = (float)(end - start);
        float scale_factor = ((float)raw_size/(float)len)*(target_range_f/raw_range_f);
        int offset = start/m_chartResol;

        int src_start = offset;

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
                dst[i_to] = val;

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

                dst[i_to] = val;

            }
        }

    }

    QVector<int16_t> m_processingDistData;
    QVector<int16_t> m_processingEdgeData;

protected:
    QVector<int16_t> m_chartData;
    int m_chartResol;
    int m_chartOffset;

    QMap<int, float> _rangeFinders;

    int m_dist;
    int m_processingDist = 0;
    int _procMinDist = 0;
    int _procMaxDist = INT32_MAX;
    int _procDistType = 0;

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
        bool chartAvail = false;
        bool distAvail = false;

        bool posAvail = false;

        bool tempAvail = false;

        bool processDistAvail = false;
        bool processChartAvail = false;
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

public slots:
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float encoder);
    void addTimestamp(int timestamp);
    void addChart(QVector<int16_t> data, int resolution, int offset);
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

    void set3DRender(FboInSGRenderer* render) { _render3D = render; }
    void updateRender3D() {
        if(_render3D != NULL) {
            _render3D->updateBottomTrack(_bottomTrack);
        }
    }
    void updateBottomTrack(bool update_all = false);

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
        float temperature = 0;
        float dopplerX = NAN;
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

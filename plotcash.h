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

class PoolDataset {
public:
    PoolDataset();
    void setEvent(int timestamp, int id);
    void setEncoder(float encoder);
    void setChart(QVector<int16_t> chartData, int resolution, int offset);
    void setDist(int dist);
    void setPosition(uint32_t date, uint32_t time, double lat, double lon);
    void setEncoders(int16_t enc1, int16_t enc2 = 0xFFFF, int16_t enc3 = 0xFFFF, int16_t = 0xFFFF, int16_t = 0xFFFF, int16_t enc6 = 0xFFFF);
    void setAtt(float yaw, float pitch, float roll);

    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp; }


    QVector<int16_t> chartData() { return m_chartData; }
    bool chartAvail() { return flags.chartAvail; }

    int distData() { return m_dist; }
    bool distAvail() { return flags.distAvail; }

    int distProccesing() { return m_processingDist; }
    bool distProccesingAvail() { return flags.processDistAvail; }

    bool isAttAvail() { return _attitude.is_avail; }

    double lat() { return m_position.lat; }
    double lon() { return m_position.lon; }
    bool isPosAvail() { return flags.posAvail; }

    void doDistProccesing() {
        const int raw_size = m_chartData.size();
        const int16_t* src = m_chartData.data();

        if(raw_size != 0 && !distProccesingAvail()) {
            m_processingDistData.resize(raw_size);
            m_processingDistData.fill(0);
            int16_t* procData = m_processingDistData.data();

            for(int i = 6; i < raw_size - 11; i ++) {
                procData[i] = ((src[i+2] + src[i+3]*2 + src[i+4]*3 + src[i+5]*3 + src[i+6]*4 + src[i+7]*4 + src[i+7]*3 + src[i+8]*3 + src[i+9]*2 + src[i+10]) + (((float)(i*m_chartResol)*0.001 + 1.0)) - (src[i] + src[i-1]*2 + src[i-2]*2 + src[i-3]*2 + src[i-4]*2 + src[i-5]*2 + src[i-6]))/16;
            }

            int index_max = 0;
            int16_t val_max = 0;
            for(int i = 0; i < raw_size; i ++) {
                if(procData[i] > val_max) {
                    val_max = procData[i];
                    index_max = i;
                }
            }

            flags.processDistAvail = true;
            flags.processChartAvail = true;
            m_processingDist = (index_max + m_chartOffset)*m_chartResol;
        }
    }

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

    void chartTo(int start, int end, int16_t* dst, int len, int image_type) {
        if(dst == nullptr) {  return; }
        int raw_size = m_chartData.size();

        int16_t* src;

        if(image_type == 1 && !edgeProcAvail) {
            doEdgeProccesing();
        }

        if(image_type == 1 && edgeProcAvail) {
            src = m_processingEdgeData.data();
        } else {
            src = m_chartData.data();
        }

        if(raw_size == 0) {
            for(int i_to = 0; i_to < len; i_to++) {
                dst[i_to] = 0;
            }
        }

        float raw_range_f = (float)(raw_size*m_chartResol);
        float target_range_f = (float)(end - start);
        float scale_factor = ((float)raw_size/(float)len)*(target_range_f/raw_range_f);
        int offset = start/m_chartResol;

        int src_start = offset;
        for(int i_to = 0; i_to < len; i_to++) {
            int src_end = (float)(i_to + 1)*scale_factor + offset;

            int16_t val = 0;
            if(src_start >= 0 && src_start < raw_size) {
                if(src_end > raw_size) {
                    src_end = raw_size;
                }

                val = src[src_start];
                for(int i = src_start + 1; i < src_end; i++) {
                    if(src[i] > val) {
                        val = src[i];
                    }
                }
            } else {
                val = 0;
            }
            src_start = src_end;
            dst[i_to] = val;

        }
    }

    QVector<int16_t> m_processingDistData;
    QVector<int16_t> m_processingEdgeData;

protected:
    QVector<int16_t> m_chartData;
    int m_chartResol;
    int m_chartOffset;

    int m_dist;
    int m_processingDist = 0;

    int _eventTimestamp = 0;
    int _eventId = 0;

    float _encoder;

    struct {
        float yaw = 0, pitch = 0, roll = 0;
        bool is_avail = false;
    } _attitude;



    struct {
        uint32_t date = 0;
        uint32_t time = 0;
        double lat = 0;
        double lon = 0;
    } m_position;

    struct {
        bool valid = false;
        int16_t e1 = 0;
        int16_t e2 = 0;
        int16_t e3 = 0;
        int16_t e4 = 0;
        int16_t e5 = 0;
        int16_t e6 = 0;
    } encoder;

    struct {
        bool encoderAvail = false;
        bool eventAvail = false;
        bool timestampAvail = false;
        bool chartAvail = false;
        bool distAvail = false;

        bool posAvail = false;

        bool processDistAvail = false;
        bool processChartAvail = false;
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
    Q_PROPERTY(int distProcessing WRITE doDistProcessing)

    int poolSize();

    PoolDataset* fromPool(int index_offset = 0) {
        int index = poolIndex(index_offset);
//        qInfo("pool index %u", index);
        return &m_pool[index];
    }

    QImage getImage(QSize size);

public slots:
    void addEvent(int timestamp, int id);
    void addEncoder(float encoder);
    void addTimestamp(int timestamp);
    void addChart(QVector<int16_t> data, int resolution, int offset);
    void addDist(int dist);
    void addAtt(float yaw, float pitch, float roll);
    void addPosition(uint32_t date, uint32_t time, double lat, double lon);
    void setStartLevel(int level);
    void setStopLevel(int level);
    void setTimelinePosition(double position);
    void scrollTimeline(int delta);
    void verZoom(int delta);
    void verScroll(int delta);
    void setChartVis(bool visible);
    void setOscVis(bool visible);
    void setDistVis(bool visible);
    void setDistProcVis(bool visible);
    void setEncoderVis(bool visible);
    void setImageType(int image_type);
    void setAHRSVis(bool visible);
    void updateImage(bool update_value = false);
    void renderValue();
    void resetValue();
    void resetDataset();
    void doDistProcessing(bool processing);

    void setThemeId(int theme_id);

signals:
    void updatedImage();

protected:
    int m_verticalGridNum = 5;
    float m_legendMultiply = 0.001f;
    int m_range = 2000;
    int m_offset = 0;
    int m_startLevel = 10;
    int m_stopLevel = 100;
    int m_offsetLine = 0;
    bool m_chartVis = true;
    bool m_oscVis = false;
    bool m_distSonarVis = true;
    bool m_distProcessingVis = true;
    bool m_distCalcVis = true;
    bool _is_attitudeVis = false;
    bool _is_encoderVis = false;
    bool isDistProcessing = false;
    int _themId;
    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

    int _imageType = 0;

    enum {
        AutoRangeNone,
        AutoRangeLast,
        AutoRangeMax,
        AutoRangeMaxVis
    } _autoRange = AutoRangeLast;


    QVector<PoolDataset> m_pool;


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
    uint16_t m_dataImage[2600*2000];
    int m_prevLineWidth = 30;

    float _lastYaw = 0, _lastPitch = 0, _lastRoll = 0;

    struct  {
        bool resetValue;
        bool renderValue;
        bool renderImage;
    } flags;

    void updateValueMap(int width, int height);
    void updateImage(int width, int height);

    void poolAppend() {
        m_pool.resize(m_pool.size() + 1);
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

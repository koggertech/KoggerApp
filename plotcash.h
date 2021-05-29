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
    void setChart(QVector<int16_t> chartData, int resolution, int offset);
    void setDist(int dist);
    void setPosition(uint32_t date, uint32_t time, double lat, double lon);
    void setEncoders(int16_t enc1, int16_t enc2 = 0xFFFF, int16_t enc3 = 0xFFFF, int16_t = 0xFFFF, int16_t = 0xFFFF, int16_t enc6 = 0xFFFF);

    bool eventAvail() { return flags.eventAvail; }
    int eventID() { return _eventId; }
    int eventTimestamp() {return _eventTimestamp; }


    QVector<int16_t> chartData() { return m_chartData; }
    bool chartAvail() { return flags.chartAvail; }

    int distData() { return m_dist; }
    bool distAvail() { return flags.distAvail; }

    int distProccesing() { return m_processingDist; }
    bool distProccesingAvail() { return flags.processDistAvail; }

    void doDistProccesing() {
        int raw_size = m_chartData.size();
        int16_t* src = m_chartData.data();

        if(raw_size != 0 && !distProccesingAvail()) {
            m_processingDistData.resize(raw_size);
            int16_t* procData = m_processingDistData.data();

            float avrg = src[2]*3;
            for(int i = 0; i < raw_size; i ++) {
                float val = src[i];

                procData[i] = (val*1.2 - avrg)*(((float)(i*m_chartResol)*0.00003 + 1.0));
                if(procData[i] < 0) {
                    procData[i] = 0;
                } else if(procData[i] > 255) {
                    procData[i] = 255;
                }

                if(avrg > val) {
                    avrg = avrg*0.2f + val*0.8;
                } else {
                    avrg = avrg*0.95f + val*0.05;
                }
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

            float avrg = max_of_start*3.0f;
            for(int i = 0; i < raw_size; i ++) {
                float val = src[i];

                avrg += (val - avrg)*0.05f;
                procData[i] = (val - avrg*0.5f)*(0.9f +float(i*m_chartResol)*0.00005f)*1.2f;

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

    void chartTo(int start, int end, int16_t* dst, int len, bool addition = false) {
        if(dst == nullptr) {  return; }
        int raw_size = m_chartData.size();

        int16_t* src;

//        if(!edgeProcAvail) {
//            doEdgeProccesing();
//        }

        if(edgeProcAvail) {
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
            if(addition) {
                dst[i_to] += val;
            } else {
                dst[i_to] = val;
            }
        }
    }

protected:
    QVector<int16_t> m_chartData;
    int m_chartResol;
    int m_chartOffset;

    int m_dist;
    int m_processingDist = 0;

    int _eventTimestamp = 0;
    int _eventId = 0;

    QVector<int16_t> m_processingDistData;
    QVector<int16_t> m_processingEdgeData;

    struct {
        uint32_t date;
        uint32_t time;
        double lat;
        double lon;
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
    void addTimestamp(int timestamp);
    void addChart(QVector<int16_t> data, int resolution, int offset);
    void addDist(int dist);
    void addPosition(uint32_t date, uint32_t time, double lat, double lon);
    void setStartLevel(int level);
    void setStopLevel(int level);
    void setTimelinePosition(double position);
    void setChartVis(bool visible);
    void setOscVis(bool visible);
    void setDistVis(bool visible);
    void setDistProcVis(bool visible);
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
    bool isDistProcessing = false;
    int _themId;
    int lastEventTimestamp = 0;
    int lastEventId = 0;



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
        return index;
    }

    void setColorScheme(QVector<QColor> coloros, QVector<int> levels);
};

#endif // PLOT_CASH_H

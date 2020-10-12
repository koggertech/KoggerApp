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
class PlotCash : public QObject {
    Q_OBJECT
public:
    PlotCash();

    int poolSize();
    QImage getImage(QSize size);

public slots:
    void addChart(QVector<int16_t> data, int resolution, int offset);
    void addDist(int dist);
    void addPosition(uint32_t date, uint32_t time, double lat, double lon);
    void setStartLevel(int level);
    void setStopLevel(int level);
    void setTimelinePosition(double position);
    void setChartVis(bool visible);
    void setDistVis(bool visible);
    void updateImage(bool update_value = false);
    void renderValue();
    void resetValue();
    void resetDataset();

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
    bool m_distSonarVis = true;
    bool m_distCalcVis = true;

    class PoolDataset {
    public:
        PoolDataset();
        void setChart(QVector<int16_t> chartData, int resolution, int offset);
        void setDist(int dist);
        void setPosition(uint32_t date, uint32_t time, double lat, double lon);

        QVector<int16_t> chartData() { return m_chartData; }
        bool chartAvail() { return flags.chartAvail; }

        int distData() { return m_dist; }
        bool distAvail() { return flags.distAvail; }

        void chartTo(int start, int end, int16_t* dst, int len, bool addition = false) {
            if(dst == nullptr) {  return; }

            int raw_size = m_chartData.size();
            int16_t* src = m_chartData.data();
//            int16_t* procData = m_processingData.data();
//            if(!processing) {
//                processing = true;
//                m_processingData.resize(raw_size);
//                procData = m_processingData.data();

//                float avrg = src[2]*3;
//                for(int i = 0; i < raw_size; i ++) {
//                    float val = src[i];

//                    procData[i] = (val*1.2 - avrg)*(((float)(i*m_chartResol)*0.00003 + 1.0));
//                    if(procData[i] < 0) {
//                        procData[i] = 0;
//                    } else if(procData[i] > 255) {
//                        procData[i] = 255;
//                    }

//                    if(avrg > val) {
//                        avrg = avrg*0.2f + val*0.8;
//                    } else {
//                        avrg = avrg*0.95f + val*0.05;
//                    }
//                }
//            }

//            src = procData;

            if(raw_size == 0) {
                for(int i_to = 0; i_to < len; i_to++) {
                    dst[i_to] = 0;
                }
            }

            float raw_range_f = (float)(raw_size*m_chartResol);
            float target_range_f = (float)(end - start);
            float scale_factor = ((float)raw_size/(float)len)*(target_range_f/raw_range_f);

            int src_start = 0;
            for(int i_to = 0; i_to < len; i_to++) {
                int src_end = (float)(i_to + 1)*scale_factor;

                int16_t val = 0;
                if(src_start < raw_size && src_start >= 0) {
                    if(src_end > raw_size) {
                        src_end = raw_size;
                    }

                    val = src[src_start];
                    for(int i = src_start + 1; i < src_end; i++) {
                        if(src[i] > val) {
                            val = src[i];
                        }
                    }
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

        QVector<int16_t> m_processingData;
        bool processing = false;

        struct {
            uint32_t date;
            uint32_t time;
            double lat;
            double lon;
        } m_position;

        struct {
            bool chartAvail = false;
            bool distAvail = false;
            bool posAvail = false;
        } flags;

    };

    QVector<PoolDataset> m_pool;
    PoolDataset* fromPool(int index_offset = 0) {
        int index = poolIndex(index_offset);
        qInfo("pool index %u", index);
        return &m_pool[index];
    }

    typedef struct {
        QVector<int16_t> chartData;
        int distData = -1;
        int poolIndex = -1;
        bool poolIndexUpdate = true;
    } ValueCash;
    int m_valueCashStart = 0;

    QVector<ValueCash> m_valueCash;
    ValueCash m_prevValueCash;
    int m_valueIndex;
    QVector<QColor> m_colorMap;
    int16_t m_colorHashMap[256];
    int16_t m_colorDist = 0xFFFF;
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

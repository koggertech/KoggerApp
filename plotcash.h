#ifndef PLOT_CASH_H
#define PLOT_CASH_H

#include <QObject>
#include <stdint.h>
#include <QVector>
#include <QImage>
#include <QPoint>
#include <QPixmap>
#include <QPainter>

class PlotCash : public QObject {
    Q_OBJECT
public:
    PlotCash();

    int getLineCount();
    void setLineCount(int line_count);

    QImage getImage(QSize size);

    protected:
    QVector<QColor> m_colorMap;

    class LineCash {
    public:
        LineCash();
        void setData(QVector<int16_t> data, int resolution, int offset);
        QImage* getImage(QSize size, int range, int offset, QVector<QColor> colorMap, int startLevel, int stopLevel, bool forceDraw = false);
    protected:
        QVector<int16_t> m_rawData;
        int m_dataResol;
        int m_dataOffset;
        int m_startLevel;
        int m_stopLevel;

        QImage m_cash;
        int m_cashResol;
        int m_cashOffset;

        bool m_isUpdated;

        inline int16_t rawDataRange(int16_t* data, int len, float start, float end) {
            int start_index = ((start - m_dataOffset)/(float)m_dataResol);
            int end_index = ((end - m_dataOffset)/(float)m_dataResol);

            int16_t val;

            if(start_index >= len || start_index < 0) {
                val = 0;
            } else {
                if(end_index > len) {
                    end_index = len;
                }

                val = data[start_index];
                for(int i = start_index + 1; i < end_index; i++) {
                    if(data[i] > val) {
                        val = data[i];
                    }
                }
            }

            return val;
        }
    };

    QVector<LineCash> Lines;
    int CurrentIndex;
    QImage m_cash;

    QImage* getLine(int index, QSize size);

    void nextIndex() {
        CurrentIndex++;
        if(CurrentIndex >= getLineCount()) {
            CurrentIndex = 0;
        }
    }

    int getIndex(int index_offset = 0) {
        int end_index = CurrentIndex - index_offset;
        if(end_index < 0) {
            end_index = getLineCount() + end_index;
        }
        return end_index;
    }

    void setColorScheme(QVector<QColor> coloros, QVector<int> levels);

public slots:
    void addData(QVector<int16_t> data, int resolution, int offset);
    void addDist(int dist);
    void setStartLevel(int level);
    void setStopLevel(int level);

private:
    bool m_isDataUpdate;
    int m_hGrid = 5;
    float m_legendMultiply = 0.001f;
    int m_range = 2000;
    int m_offset = 0;
    int m_startLevel = 10;
    int m_stopLevel = 100;
};

#endif // PLOT_CASH_H

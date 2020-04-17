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
        void setData(QVector<uint8_t> data, int resolution, int offset);
        QImage* getImage(QSize size, int range, int offset, QVector<QColor> colorMap, bool forceDraw = false);
    protected:
        QVector<uint8_t> m_rawData;
        int m_dataResol;
        int m_dataOffset;

        QImage m_cash;
        int m_cashResol;
        int m_cashOffset;

        bool m_isUpdated;

        uint8_t rawDataRange(float start, float end);
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

public slots:
    void addData(QVector<uint8_t> data, int resolution, int offset);

private:
    bool m_isDataUpdate;
    int m_hGrid = 5;
    float m_legendMultiply = 0.001f;
    int m_range = 2000;
    int m_offset = 0;
};

#endif // PLOT_CASH_H

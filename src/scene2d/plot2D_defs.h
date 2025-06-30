#pragma once

#include <cmath>
#include <stdint.h>
#include <vector>
#include <QLineF>
#include <QVector>
#include <QPainter>
#include "dataset_defs.h"


typedef enum {
    AutoRangeNone = -1,
    AutoRangeLastData,
    AutoRangeLastOnScreen,
    AutoRangeMaxOnScreen
} AutoRangeMode;

typedef enum {
    MouseToolNone = 0,
    MouseToolNothing = 1,
    MouseToolDistanceMin,
    MouseToolDistance,
    MouseToolDistanceMax,
    MouseToolDistanceErase,
} MouseTool;

typedef struct DatasetCursor {
    int selectEpochIndx = -1;
    int currentEpochIndx = -1;
    int lastEpochIndx = -1;
    ChannelId channel1;
    uint8_t subChannel1 = 0;
    QString firstChannelPortName;
    ChannelId channel2;
    uint8_t subChannel2 = 0;
    QString secondChannelPortName;
    int numZeroEpoch = 0;

    bool isChannelDoubled() {
        return (channel1.isValid() && channel2.isValid());
    }

    std::vector<int> indexes;

    inline int getIndex(int col) const {
        if(col < (int)indexes.size() && col >= 0) {
            return indexes[col];
        }
        return -1;
    }

    float position = 1;
    int last_dataset_size = 0;

    int mouseX = -1, mouseY = -1;
    int contactX = -1, contactY = -1;

    MouseTool _tool = MouseToolNothing;

    struct {
        float from = NAN;
        float to = NAN;
        AutoRangeMode mode = AutoRangeNone;


        void set(float f, float t)
        {
            from = f;
            to = t;
        }

        float range() const {
            return to - from;
        }

        bool isValid() {
            return std::isfinite(from) && std::isfinite(to);
        }
    } distance;

    struct {
        float from = NAN;
        float to = NAN;

        void set(float f, float t) {from = f; to = t;}

        bool isValid() {
            return std::isfinite(from) && std::isfinite(to);
        }
    } attitude;

    struct {
        float from = NAN;
        float to = NAN;

        void set(float f, float t) {from = f; to = t;}

        bool isValid() {
            return std::isfinite(from) && std::isfinite(to);
        }
    } velocity;

    bool isDistanceEqual(DatasetCursor cursor) {
        return (cursor.distance.from == distance.from && cursor.distance.to == distance.to);
    }

    bool isChannelsEqual(DatasetCursor cursor) {
        return (cursor.channel1 == channel1 && cursor.channel2 == channel2);
    }

    void setMouse(int x, int y) { mouseX = x; mouseY = y; }
    void setContactPos(int x, int y) { contactX = x; contactY = y; }

    void setTool(MouseTool tool) { _tool = tool; }
    MouseTool tool() { return _tool; }

} DatasetCursor;



typedef struct  PlotColor{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;

    PlotColor(int red = 255, int green = 255, int blue = 255, int alfa = 255) {
        r = red;
        g = green;
        b = blue;
        a = alfa;
    }
} PlotColor;

typedef struct PlotPen {
    PlotColor color = PlotColor(255, 255, 255, 255);
    int width = 1;
    typedef enum {
        LineStyleNone,
        LineStyleSolid,
        LineStylePoint
    } LineStyle;

    LineStyle lineStyle = LineStyleSolid;

    PlotPen(PlotColor line_color, int line_width, LineStyle line_style) {
        color = line_color;
        width = line_width;
        lineStyle = line_style;
    }

} PlotPen;

class Canvas {
public:
//    QImage _image;

    Canvas() {
    }

    void setSize(int width, int height, QPainter* painter) {
//        if(_width != width || _height != height) {
//            _image = QImage(width, height, QImage::Format_RGB555);
//        }

        _width = width;
        _height = height;
        _painter = painter;
    }

    void clear() {
//        _image.fill(0);
    }

    int width() const { return _width; }
    int height() const { return _height; }

    void drawY(QVector<float> y, const PlotPen& pen) {
        if(_painter == NULL) { return; }
        QPen qpen;
        qpen.setWidth(pen.width);
        qpen.setColor(QColor(pen.color.r, pen.color.g, pen.color.b, pen.color.a));
        qpen.setCapStyle(Qt::FlatCap);
//        qpen.setJoinStyle()

        _painter->setPen(qpen);

        if(pen.lineStyle == PlotPen::LineStyleSolid) {
            QVector<QLineF> lines(y.size()-1);
            for(int i = 0; i < lines.size(); i++) {

                lines[i] = QLineF(i, y[i], i+1, y[i+1]);
                if(std::isfinite(y[i+1])) {
                    lines[i] = QLineF(i, y[i], i+1, y[i+1]);
                } else {
                    lines[i] = QLineF(i, y[i], i+pen.width, y[i]);
                }
            }
           _painter->drawLines(lines);
        }

        if(pen.lineStyle == PlotPen::LineStylePoint) {
            QVector<QPointF> lines(y.size());
            for(int i = 0; i < lines.size(); i++) {
                lines[i] = QPointF(i, y[i]);
            }
           _painter->drawPoints(lines);
        }

    }

    QPainter* painter() {
        return _painter;
    }

protected:
    int _width = 0;
    int _height = 0;

    QPainter* _painter = NULL;
};

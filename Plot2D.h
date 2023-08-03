#ifndef H2PLOT_H
#define H2PLOT_H

#include <QObject>
#include <stdint.h>
#include <QVector>
#include <QImage>
#include <QPoint>
#include <QPixmap>
#include <QPainter>

#include "plotcash.h"

#include <vector>


typedef struct DatasetCursor {
    int channel1 = CHANNEL_NONE;
    int channel2 = CHANNEL_NONE;

    std::vector<int> indexes;

    inline int getIndex(int col) {
        if(col < (int)indexes.size() && col >= 0) {
            return indexes[col];
        }
        return -1;
    }

    float position = 0;

    struct {
        float from = NAN;
        float to = NAN;

        void set(float f, float t) {from = f; to = t;}

        float range() { return to - from;}

        bool isValid() {
            return isfinite(from) && isfinite(to);
        }
    } distance;

    struct {
        float from = NAN;
        float to = NAN;

        void set(float f, float t) {from = f; to = t;}

        bool isValid() {
            return isfinite(from) && isfinite(to);
        }
    } attitude;

    struct {
        float from = NAN;
        float to = NAN;

        void set(float f, float t) {from = f; to = t;}

        bool isValid() {
            return isfinite(from) && isfinite(to);
        }
    } velocity;

    bool isDistanceEqual(DatasetCursor cursor) {
        return (cursor.distance.from == distance.from && cursor.distance.to == distance.to);
    }

    bool isChannelsEqual(DatasetCursor cursor) {
        return (cursor.channel1 == channel1 && cursor.channel2 == channel2);
    }
} DatasetCursor;

typedef enum {
    MouseToolNone = 0,
    MouseToolNothing = 1,
    MouseToolDistanceMin,
    MouseToolDistance,
    MouseToolDistanceMax,
    MouseToolDistanceErase,
} MouseTool;

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
    Canvas() {
        _data.resize(10000000);
    }

    void setSize(int width, int height) {
        int data_size = width*height;
        if(data_size < _data.size()) {
            _data.resize(data_size);
        }
        _width = width;
        _height = height;
    }

    void clear() {
        _data.fill(0, _width*_height);
    }

    int width() { return _width; }
    int height() { return _height; }

    QImage getQImage() {
        return QImage((uint8_t*)_data.data(), _width, _height, _width*2, QImage::Format_RGB555);
    }


    void drawLineY(QVector<float> y, PlotPen pen) {
        QPen qpen;
        qpen.setWidth(pen.width);
        qpen.setColor(QColor(pen.color.r, pen.color.g, pen.color.b, pen.color.a));

        QImage qimage = getQImage();
        QPainter p(&qimage);
        p.setPen(qpen);


        QVector<QLineF> lines(y.size()-1);
        for(int i = 0; i < lines.size(); i++) {
            lines[i] = QLineF(i, y[i], i+1, y[i+1]);
        }

        p.drawLines(lines);
    }

    uint16_t* data() {
        return (uint16_t*)_data.constData();
    }
protected:
    int _width = 0;
    int _height = 0;
    QVector<int16_t> _data;
};

class PlotLayer {
public:
    PlotLayer() {}
    bool isVisible() { return _isVisible; }
    void setVisible(bool visible) { _isVisible = visible; }

    virtual bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) { return false; }
protected:
    bool _isVisible = false;
};

class Plot2DEchogram : public PlotLayer {
public:
    enum ThemeId {
        ClassicTheme,
        SepiaTheme,
        WRGBDTheme,
        WBTheme,
        BWTheme
    };

    Plot2DEchogram();
    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor);

    void setLowLevel(float low);
    void setHightLevel(float high);
    void setLevels(float low, float hight);

    void setColorScheme(QVector<QColor> coloros, QVector<int> levels);
    void setThemeId(int theme_id);

    int updateCash(Dataset* dataset, DatasetCursor cursor, int width, int height);
    void resetCash();

protected:
    typedef struct {
        typedef enum {
            CashStateNotValid,
            CashStateValid,
            CashStateEraced
        } CashState;

        int poolIndex = -1;
        CashState state = CashStateNotValid;
        QVector<int16_t> data;
    } CashLine;

    uint16_t _colorHashMap[65536];
    QVector<CashLine> _cash;
    int _cashPosition = 0;

    struct {
        bool resetCash = true;
    } _cashFlags;

    struct {
       float low = 100, high = 10;
    } _levels;

    DatasetCursor _lastCursor;
    int _lastWidth = -1;
    int _lastHeight = -1;

    bool getTriggerCashReset() {
        bool reset_cash = _cashFlags.resetCash;
        _cashFlags.resetCash = false;
        return reset_cash;
    }
};

class Plot2DLine : public PlotLayer {
public:
    Plot2DLine() {}

protected:
    bool drawLineY(Canvas& canvas, QVector<float> data, float value_from, float value_to, PlotPen pen) {
        if(canvas.width() != data.size()) { return false; }

        QVector<float> data_maped(data.size());

        const float canvas_height = canvas.height();
        float value_range = value_to - value_from;
        float value_scale = canvas_height/value_range;

        for(int i = 0; i < data_maped.size(); i++) {
            float y = (data[i] - value_from)*value_scale;
            if(y < 0) { y = 0; }
            else if(y > canvas_height) { y = canvas_height; }
            data_maped[i] = y;
        }

        canvas.drawLineY(data_maped, pen);

        return true;
    }
};

class Plot2DDVLBeamVelocity : public Plot2DLine {
public:
    Plot2DDVLBeamVelocity() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.velocity.isValid()) { return false; }

        QVector<float> beam(canvas.width());

        for(int ibeam = 0; ibeam < 4; ibeam++) {

            for(int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch* data = dataset->fromIndex(pool_index);

                if(data != NULL && data->isDopplerBeamAvail(ibeam)) {
                    beam[i] = data->dopplerBeam(ibeam).velocity;
                } else {
                    beam[i] = NAN;
                }
            }

            drawLineY(canvas, beam, cursor.velocity.from, cursor.velocity.to, _penBeam[ibeam]);
        }
        return true;
    }

    void setBeamFilter(int filter) { _beamFilter = filter; };

protected:
    int _beamFilter = 15;
    PlotPen _penBeam[4] = {PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
    PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
    PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
    PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};
};

class Plot2DAttitude : public Plot2DLine {
public:
    Plot2DAttitude() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.attitude.isValid()) { return false; }

        QVector<float> yaw(canvas.width());
        QVector<float> pitch(canvas.width());
        QVector<float> roll(canvas.width());

        for(int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch* data = dataset->fromIndex(pool_index);

            if(data != NULL && data->isAttAvail()) {
                yaw[i] = data->yaw();
                pitch[i] = data->pitch();
                roll[i] = data->roll();

            } else {
                yaw[i] = NAN;
                pitch[i] = NAN;
                roll[i] = NAN;
            }
        }

        drawLineY(canvas, yaw, cursor.attitude.from, cursor.attitude.to, _penYaw);
        drawLineY(canvas, pitch, cursor.attitude.from, cursor.attitude.to, _penPitch);
        drawLineY(canvas, roll, cursor.attitude.from, cursor.attitude.to, _penRoll);

        return true;
    }

protected:
    PlotPen _penYaw = PlotPen(PlotColor(255, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penPitch = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
    PlotPen _penRoll = PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid);
};

class Plot2DBottomProcessing : public Plot2DLine {
public:
    Plot2DBottomProcessing() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.distance.isValid()) { return false; }

        QVector<float> distance1(canvas.width());
        QVector<float> distance2(canvas.width());

        distance1.fill(NAN);
        distance2.fill(NAN);

        if(cursor.channel2 == CHANNEL_NONE) {
            for(int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch* data = dataset->fromIndex(pool_index);
                if(data != NULL) {
                    distance1[i] = data->distProccesing(cursor.channel1);
                }
            }
        } else {
            for(int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch* data = dataset->fromIndex(pool_index);
                if(data != NULL) {
                    distance1[i] = -data->distProccesing(cursor.channel1);
                    distance2[i] = data->distProccesing(cursor.channel2);
                }
            }
        }

        drawLineY(canvas, distance1, cursor.distance.from, cursor.distance.to, _penLine);
        if(cursor.channel2 != CHANNEL_NONE) {
            drawLineY(canvas, distance2, cursor.distance.from, cursor.distance.to, _penLine2);
        }

        return true;
    }

protected:
    PlotPen _penLine = PlotPen(PlotColor(50, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penLine2 = PlotPen(PlotColor(200, 200, 0), 2, PlotPen::LineStyleSolid);
};



class Plot2DGrid : public PlotLayer {
public:
    Plot2DGrid();
    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor);

    void setVetricalNumber(int grids) { _lines = grids; }
    void setVelocityVisible(bool visible) { _velocityVisible = visible; }
protected:
    bool _velocityVisible = true;
    int _lines = 20;
    int _lineWidth = 1;
    QColor _lineColor = QColor(255, 255, 255, 150);

};


class Plot2D {
public:
    Plot2D();

    void setDataset(Dataset* dataset) {
        _dataset = dataset;
    }

    QImage getImage(int width, int height);

    bool isHorizontal() { return _isHorizontal; }
    void setHorizontal(bool is_horizontal) { _isHorizontal = is_horizontal; }

    void setDataPosition(float position);
    void setDataChannel(int channel, int channel2 = CHANNEL_NONE);


    void setEchogramLowLevel(float low);
    void setEchogramHightLevel(float high);
    void setEchogramVisible(bool visible);
    void setEchogramTheme(int theme_id);

    void setBottomTrackVisible(bool visible);
    void setBottomTrackTheme(int theme_id);

    void setRangefinderVisible(bool visible);
    void setAttitudeVisible(bool visible);
    void setDopplerBeamVisible(bool visible, int beam_filter);
    void setDopplerInstrumentVisible(bool visible);

    void setGridVetricalNumber(int grids);
    void setVelocityVisible(bool visible);
    void setVelocityRange(float velocity);

    void setDistance(float from, float to);
    void zoomDistance(float ratio);
    void scrollDistance(float ratio);

    void setMousePosition(int x, int y);
    void setMouseTool(MouseTool tool);

    virtual void plotUpdate() {}

protected:
    Dataset* _dataset = NULL;
    Canvas _canvas;

    BottomTrackParam _bottomTrackParam;

    DatasetCursor _cursor;

    struct {
        int x = -1;
        int y = -1;
        MouseTool tool = MouseToolNone;
    } _mouse;

    bool _isHorizontal = true;


    Plot2DEchogram _echogram;
    Plot2DAttitude _attitude;
    Plot2DDVLBeamVelocity _DVLBeamVelocity;
    Plot2DBottomProcessing _bottomProcessing;
    Plot2DGrid _grid;

    Canvas image(int width, int height);

    void reindexingCursor();
};



#endif // H2PLOT_H

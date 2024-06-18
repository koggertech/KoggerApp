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
    int channel1 = CHANNEL_FIRST;
    int channel2 = CHANNEL_NONE;

    bool isChannelDoubled() {
        return (CHANNEL_NONE != channel1 && CHANNEL_NONE != channel2);
    }

    std::vector<int> indexes;

    inline int getIndex(int col) {
        if(col < (int)indexes.size() && col >= 0) {
            return indexes[col];
        }
        return -1;
    }

    float position = 1;
    int last_dataset_size = 0;

    int mouseX = -1, mouseY = -1;
    MouseTool _tool = MouseToolNothing;

    struct {
        float from = NAN;
        float to = NAN;
        AutoRangeMode mode = AutoRangeNone;


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

    void setMouse(int x, int y) { mouseX = x; mouseY = y;  }
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

    int width() { return _width; }
    int height() { return _height; }

    void drawY(QVector<float> y, PlotPen pen) {
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
                if(isfinite(y[i+1])) {
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

class PlotLayer {
public:
    PlotLayer() {}
    bool isVisible() { return _isVisible; }
    void setVisible(bool visible) { _isVisible = visible; }

    virtual bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor)
    {
        Q_UNUSED(canvas);
        Q_UNUSED(dataset);
        Q_UNUSED(cursor);
        return false;
    }

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

    void updateColors();

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
        bool isNeedUpdate = true;

        QVector<int16_t> data;

//        CashState stateColor = CashStateNotValid;
//        QVector<uint16_t> color;
    } CashLine;

    uint16_t _colorHashMap[256];
    QVector<CashLine> _cash;

    QVector<QRgb> _colorTable;
    QVector<QRgb> _colorLevels;
    QImage _image;
    QPixmap _pixmap;
    bool _flagColorChanged = true;


    struct {
        bool resetCash = true;
    } _cashFlags;

    struct {
       float low = 100, high = 10;
    } _levels;

    struct {
       float low = NAN, high = NAN;
    } _lastLevels;

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
    bool drawY(Canvas& canvas, QVector<float> data, float value_from, float value_to, PlotPen pen) {
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

        canvas.drawY(data_maped, pen);

        return true;
    }
};

class Plot2DDVLBeamVelocity : public Plot2DLine {
public:
    Plot2DDVLBeamVelocity() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.velocity.isValid()) { return false; }

        QVector<float> beam_velocity(canvas.width());
        beam_velocity.fill(NAN);
        QVector<float> beam_amp(canvas.width());
        beam_amp.fill(NAN);
        QVector<float> beam_mode(canvas.width());
        beam_mode.fill(NAN);
        QVector<float> beam_coh(canvas.width());
        beam_coh.fill(NAN);
        QVector<float> beam_dist(canvas.width());
        beam_dist.fill(NAN);

        for(int ibeam = 0; ibeam < 4; ibeam++) {
            if(((_beamFilter >> ibeam)&1) == 0) { continue; }

            for(int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch* data = dataset->fromIndex(pool_index);

                if(data != NULL &&  data->isDopplerBeamAvail(ibeam)) {
                    IDBinDVL::BeamSolution beam = data->dopplerBeam(ibeam);
                    beam_velocity[i] = beam.velocity;
                    beam_amp[i] = beam.amplitude*0.1f;
                    beam_mode[i] = (beam.mode*6+ibeam)*2;
                    beam_coh[i] = beam.coherence[0];
                    beam_dist[i] = beam.distance;
                }
            }


            drawY(canvas, beam_velocity, cursor.velocity.from, cursor.velocity.to, _penBeam[ibeam]);
//            drawY(canvas, beam_amp, 0, 100, _penAmp[ibeam]);
//            drawY(canvas, beam_coh, 0, 1000, _penAmp[ibeam]);
            drawY(canvas, beam_mode, canvas.height(), 0, _penMode[ibeam]);
            drawY(canvas, beam_dist, cursor.distance.from, cursor.distance.to, _penAmp[ibeam]);
        }


        return true;
    }

    void setBeamFilter(int filter) { _beamFilter = filter; };

protected:
    int _beamFilter = 15;
    PlotPen _penBeam[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};

    PlotPen _penMode[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStylePoint)};

    PlotPen _penAmp[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};
};

class Plot2DDVLSolution : public Plot2DLine {
public:
    Plot2DDVLSolution() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.velocity.isValid()) { return false; }

        QVector<float> velocityX(canvas.width());
        velocityX.fill(NAN);

        QVector<float> velocityY(canvas.width());
        velocityY.fill(NAN);

        QVector<float> velocityZ(canvas.width());
        velocityZ.fill(NAN);

        QVector<float> velocityA(canvas.width());
        velocityA.fill(NAN);

        QVector<float> distance(canvas.width());
        distance.fill(NAN);

        for(int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch* data = dataset->fromIndex(pool_index);

            if(data != NULL &&  data->isDVLSolutionAvail()) {
                IDBinDVL::DVLSolution sol = data->dvlSolution();
                velocityX[i] = sol.velocity.x;
                velocityY[i] = sol.velocity.y;
                velocityZ[i] = sol.velocity.z;
                velocityA[i] = sqrt(sol.velocity.x*sol.velocity.x + sol.velocity.y*sol.velocity.y + sol.velocity.z*sol.velocity.z);
                distance[i] = sol.distance.z;
            }
        }


        drawY(canvas, velocityX, cursor.velocity.from, cursor.velocity.to, _penVelocity[0]);
        drawY(canvas, velocityY, cursor.velocity.from, cursor.velocity.to, _penVelocity[1]);
        drawY(canvas, velocityZ, cursor.velocity.from, cursor.velocity.to, _penVelocity[2]);
        drawY(canvas, velocityA, cursor.velocity.from, cursor.velocity.to, _penVelocity[3]);
        drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penDist);

        return true;
    }


protected:
    PlotPen _penVelocity[4] = {
        PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 255, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 0, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 170, 155), 2, PlotPen::LineStyleSolid)};

    PlotPen _penDist = PlotPen(PlotColor(155, 155, 155), 3, PlotPen::LineStylePoint);


//    PlotPen _penMode[4] = {
//        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStylePoint)};

//    PlotPen _penAmp[4] = {
//        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};
};

class Plot2DUSBLSolution : public Plot2DLine {
public:
    Plot2DUSBLSolution() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.distance.isValid()) { return false; }

        QVector<float> azimuth(canvas.width());
        azimuth.fill(NAN);

        QVector<float> elevation(canvas.width());
        elevation.fill(NAN);

        QVector<float> distance(canvas.width());
        distance.fill(NAN);

        for(int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch* data = dataset->fromIndex(pool_index);

            if(data != NULL &&  data->isUsblSolutionAvailable()) {
                IDBinUsblSolution::UsblSolution sol = data->usblSolution();

                azimuth[i] = sol.azimuth_deg;
                elevation[i] = sol.elevation_deg;
                distance[i] = sol.distance_m;
            }
        }

        drawY(canvas, azimuth, cursor.attitude.from, cursor.attitude.to, _penAngle[0]);
        drawY(canvas, elevation, cursor.attitude.from, cursor.attitude.to, _penAngle[1]);
        drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penDist);

        return true;
    }


protected:
    PlotPen _penAngle[4] = {
                               PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 255, 0), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 0, 255), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 170, 155), 2, PlotPen::LineStyleSolid)};

    PlotPen _penDist = PlotPen(PlotColor(155, 155, 155), 3, PlotPen::LineStylePoint);

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

        drawY(canvas, yaw, cursor.attitude.from, cursor.attitude.to, _penYaw);
        drawY(canvas, pitch, cursor.attitude.from, cursor.attitude.to, _penPitch);
        drawY(canvas, roll, cursor.attitude.from, cursor.attitude.to, _penRoll);

        return true;
    }

protected:
    PlotPen _penYaw = PlotPen(PlotColor(255, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penPitch = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
    PlotPen _penRoll = PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid);
};

class Plot2DGNSS : public Plot2DLine {
public:
    Plot2DGNSS() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.velocity.isValid()) { return false; }

        QVector<float> h_speed(canvas.width());
        h_speed.fill(NAN);

        for(int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch* data = dataset->fromIndex(pool_index);

            if(data != NULL) {
                h_speed[i] = data->gnssHSpeed();
            }
        }

        drawY(canvas, h_speed, cursor.velocity.from, cursor.velocity.to, _penHSpeed);

        return true;
    }

protected:
    PlotPen _penHSpeed = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
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

        drawY(canvas, distance1, cursor.distance.from, cursor.distance.to, _penLine);
        if(cursor.channel2 != CHANNEL_NONE) {
            drawY(canvas, distance2, cursor.distance.from, cursor.distance.to, _penLine2);
        }

        return true;
    }

protected:
    PlotPen _penLine = PlotPen(PlotColor(50, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penLine2 = PlotPen(PlotColor(200, 200, 0), 2, PlotPen::LineStyleSolid);
};


class Plot2DRangefinder : public Plot2DLine {
public:
    Plot2DRangefinder() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if(!isVisible() || !cursor.distance.isValid()) { return false; }

        if(_themeId < 1) { return true;}

        QVector<float> distance(canvas.width());

        distance.fill(NAN);

        for(int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch* data = dataset->fromIndex(pool_index);
            if(data != NULL) {
                distance[i] = data->rangeFinder();
            }
        }

        if(_themeId == 1) {
            drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penLine);
        }

        if(_themeId == 2) {
            drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penPoint);
        }


        return true;
    }

    void setTheme(int theme_id) { _themeId = theme_id; }

protected:
    PlotPen _penPoint = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStylePoint);
    PlotPen _penLine = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStyleSolid);
    int _themeId = 0;
};

class Plot2DQuadrature : public Plot2DLine {
public:
    Plot2DQuadrature() {}


    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor)
    {
        Q_UNUSED(dataset);

        if(!isVisible() || !cursor.distance.isValid()) { return false; }

        QVector<float> real1(canvas.width());
        QVector<float> imag1(canvas.width());
        real1.fill(NAN);
        imag1.fill(NAN);

        QVector<float> real2(canvas.width());
        QVector<float> imag2(canvas.width());
        real2.fill(NAN);
        imag2.fill(NAN);

        QVector<float> real3(canvas.width());
        QVector<float> imag3(canvas.width());
        real3.fill(NAN);
        imag3.fill(NAN);

        QVector<float> real4(canvas.width());
        QVector<float> imag4(canvas.width());
        real4.fill(NAN);
        imag4.fill(NAN);


        // for(int i = 0; i < canvas.width(); i++) {
        //     int pool_index = cursor.getIndex(i);
        //     Epoch* data = dataset->fromIndex(pool_index);
        //     if(data != NULL) {
        //         QByteArray raw = data->complexSignalData();
        //         if(raw.size() > 0) {
        //             const int16_t* data = (int16_t*)raw.data();
        //             real1[i] = data[0+50];
        //             imag1[i] = data[1+50];

        //             real2[i] = data[2+50];
        //             imag2[i] = data[3+50];

        //             real3[i] = data[4+50];
        //             imag3[i] = data[5+50];

        //             real4[i] = data[6+50];
        //             imag4[i] = data[7+50];
        //         }
        //     }
        // }

        // drawY(canvas, real1, -3200+1500, 3200+1500, _penReal);
        // drawY(canvas, imag1, -3200+1500, 3200+1500, _penImag);

        // drawY(canvas, real2, -3200+768, 3200+768, _penReal);
        // drawY(canvas, imag2, -3200+768, 3200+768, _penImag);

        // drawY(canvas, real3, -3200-768, 3200-768, _penReal);
        // drawY(canvas, imag3, -3200-768, 3200-768, _penImag);

        // drawY(canvas, real4, -3200-1500, 3200-1500, _penReal);
        // drawY(canvas, imag4, -3200-1500, 3200-1500, _penImag);


        return true;
    }

protected:
    PlotPen _penReal = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penImag = PlotPen(PlotColor(0, 100, 250), 2, PlotPen::LineStyleSolid);
};


class Plot2DGrid : public PlotLayer {
public:
    Plot2DGrid();
    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor);

    void setAngleVisibility(bool state);
    void setVetricalNumber(int grids) { _lines = grids; }
    void setVelocityVisible(bool visible) { _velocityVisible = visible; }
    void setRangeFinderVisible(bool visible) { _rangeFinderLastVisible = visible; }
protected:
    bool angleVisibility_;
    bool _velocityVisible = true;
    bool _rangeFinderLastVisible = true;
    int _lines = 20;
    int _lineWidth = 1;
    QColor _lineColor = QColor(255, 255, 255, 255);

};

class Plot2DAim : public PlotLayer {
public:
    Plot2DAim() {}
    bool draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
        if((cursor.mouseX < 0 || cursor.mouseY < 0) && (cursor.selectEpochIndx == -1) ) {
            return false;
        }

        if (cursor.selectEpochIndx != -1) {
            auto selectedEpoch = dataset->fromIndex(cursor.selectEpochIndx);
            int offsetX = 0;
            int halfCanvas = canvas.width() / 2;
            int withoutHalf = dataset->size() - halfCanvas;
            if (cursor.selectEpochIndx >= withoutHalf)
                offsetX = cursor.selectEpochIndx - withoutHalf;
            if (const auto keys{ dataset->channelsList().keys() }; !keys.empty()) {
                if (const auto chartPtr{ selectedEpoch->chart(keys.at(0)) }; chartPtr) {
                    const int x = canvas.width() / 2 + offsetX;
                    const int y = keys.size() == 2 ? canvas.height() / 2 - canvas.height() * (chartPtr->bottomProcessing.distance / cursor.distance.range()) :
                                      canvas.height() * (chartPtr->bottomProcessing.distance / cursor.distance.range());
                    cursor.setMouse(x, y);
                }
            }
        }

        QPen pen;
        pen.setWidth(_lineWidth);
        pen.setColor(_lineColor);


        QPainter* p = canvas.painter();
        p->setPen(pen);
        QFont font = QFont("Asap", 14, QFont::Normal);
        font.setPixelSize(18);
        p->setFont(font);

        const int image_height = canvas.height();
        const int image_width = canvas.width();

        if (cursor._tool == MouseToolNothing || beenEpochEvent_) {
            p->drawLine(0, cursor.mouseY, image_width, cursor.mouseY);
            p->drawLine(cursor.mouseX, 0, cursor.mouseX, image_height);
        }


        const float canvas_height = canvas.height();
        float value_range = cursor.distance.to - cursor.distance.from;
        float value_scale = float(cursor.mouseY)/canvas_height;
        float cursor_distance = value_scale*value_range + cursor.distance.from;

        if(cursor.mouseY > 60) {
            p->drawText(cursor.mouseX+20, cursor.mouseY-20, QString("%1 m").arg(cursor_distance, 0, 'g', 4));
        } else {
            p->drawText(cursor.mouseX+20, cursor.mouseY+40, QString("%1 m").arg(cursor_distance, 0, 'g', 4));
        }

        return true;
    }

    void setEpochEventState(bool state) {
        beenEpochEvent_ = state;
    }

protected:
    bool beenEpochEvent_ = false;
    int _lineWidth = 1;
    QColor _lineColor = QColor(255, 255, 255, 255);
};


class Plot2D
{
public:
    Plot2D();

    void setDataset(Dataset* dataset) {
        _dataset = dataset;
    }

    bool getImage(int width, int height, QPainter* painter, bool is_horizontal);

    bool isHorizontal() { return _isHorizontal; }
    void setHorizontal(bool is_horizontal) { _isHorizontal = is_horizontal; }

    void setAimEpochEventState(bool state);
    void setTimelinePosition(float position);
    void setTimelinePositionSec(float position);
    void setTimelinePositionByEpoch(int epochIndx);

    float timelinePosition() { return _cursor.position;}
    void scrollPosition(int columns);

    void setDataChannel(int channel, int channel2 = CHANNEL_NONE);


    void setEchogramLowLevel(float low);
    void setEchogramHightLevel(float high);
    void setEchogramVisible(bool visible);
    void setEchogramTheme(int theme_id);

    void setBottomTrackVisible(bool visible);
    void setBottomTrackTheme(int theme_id);

    void setRangefinderVisible(bool visible);
    void setRangefinderTheme(int theme_id);
    void setAttitudeVisible(bool visible);
    void setDopplerBeamVisible(bool visible, int beam_filter);
    void setDopplerInstrumentVisible(bool visible);

    void setGNSSVisible(bool visible, int flags);

    void setGridVetricalNumber(int grids);
    void setAngleVisibility(bool state);
    void setAngleRange(int angleRange);

    void setVelocityVisible(bool visible);
    void setVelocityRange(float velocity);
    void setDistanceAutoRange(int auto_range_type);

    void setDistance(float from, float to);
    void zoomDistance(float ratio);
    void scrollDistance(float ratio);

    void setMousePosition(int x, int y);
    void setMouseTool(MouseTool tool);

    void resetCash();

    virtual void plotUpdate() {}

    virtual void sendSyncEvent(int epoch_index) {
        Q_UNUSED(epoch_index);
    }

protected:
    Dataset* _dataset = NULL;
    Canvas _canvas;

    BottomTrackParam _bottomTrackParam;

    DatasetCursor _cursor;

//    struct {
//        int x = -1;
//        int y = -1;

//    } _mouse;

    bool _isHorizontal = true;


    Plot2DEchogram _echogram;
    Plot2DAttitude _attitude;
    Plot2DDVLBeamVelocity _DVLBeamVelocity;
    Plot2DDVLSolution _DVLSolution;
    Plot2DUSBLSolution _usblSolution;
    Plot2DRangefinder _rangeFinder;
    Plot2DBottomProcessing _bottomProcessing;
    Plot2DGNSS _GNSS;
    Plot2DQuadrature _quadrature;
    Plot2DGrid _grid;
    Plot2DAim _aim;

    Canvas image(int width, int height);

    void reindexingCursor();
    void reRangeDistance();
};



#endif // H2PLOT_H

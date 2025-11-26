#pragma once

#include "plot2D_plot_layer.h"


class Plot2DGrid : public PlotLayer
{
public:
    Plot2DGrid();
    bool draw(Plot2D* parent, Dataset* dataset);

    void setAngleVisibility(bool state);
    void setVetricalNumber(int grids) { _lines = grids; }
    void setVelocityVisible(bool visible) { _velocityVisible = visible; }
    void setRangeFinderVisible(bool visible) { _rangeFinderLastVisible = visible; }
    void setTemperatureVisible(bool state) { temperatureVisible_ = state; };
    bool isFillWidth() const { return fillWidth_; }
    void setFillWidth(bool state) { fillWidth_ = state; }
    bool isInvert() const { return invert_; }
    void setInvert(bool state) { invert_ = state; }

protected:
    bool angleVisibility_;
    bool _velocityVisible = true;
    bool _rangeFinderLastVisible = true;
    bool temperatureVisible_ = true;
    int _lines = 20;
    int _lineWidth = 1;
    QColor _lineColor = QColor(255, 255, 255, 255);
    bool fillWidth_ = false;
    bool invert_ = false;
};

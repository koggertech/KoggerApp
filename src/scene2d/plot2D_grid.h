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
protected:
    bool angleVisibility_;
    bool _velocityVisible = true;
    bool _rangeFinderLastVisible = true;
    int _lines = 20;
    int _lineWidth = 1;
    QColor _lineColor = QColor(255, 255, 255, 255);
};

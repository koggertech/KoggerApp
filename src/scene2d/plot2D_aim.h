#pragma once

#include "plot2D_plot_layer.h"


class Plot2DAim : public PlotLayer {
public:
    Plot2DAim();
    bool draw(Plot2D* parent, Dataset* dataset);
    void setEpochEventState(bool state);

protected:
    bool beenEpochEvent_;
    int lineWidth_;
    QColor lineColor_;
    int scaleFactor_;
};


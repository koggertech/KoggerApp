#pragma once

#include "plot2D_plot_layer.h"


class Plot2D;
class Plot2DTemperature : public PlotLayer
{
public:
    Plot2DTemperature();
    bool draw(Plot2D* parent, Dataset* dataset) override;
};

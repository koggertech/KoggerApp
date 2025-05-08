#pragma once

#include "plot2D_plot_layer.h"
#include "plot2D_defs.h"
#include <QVector>


class Plot2DLine : public PlotLayer {
public:
    Plot2DLine();

protected:
    bool drawY(Canvas& canvas, QVector<float> data, float value_from, float value_to, const PlotPen& pen);
};

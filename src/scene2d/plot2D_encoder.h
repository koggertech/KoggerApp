#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DEncoder : public Plot2DLine {
public:
    Plot2DEncoder();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen penYaw_ = PlotPen(PlotColor(255, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen penPitch_ = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
};

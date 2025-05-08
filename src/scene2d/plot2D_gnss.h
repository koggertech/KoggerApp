#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DGNSS : public Plot2DLine {
public:
    Plot2DGNSS();
    bool draw(Plot2D* parent, Dataset *dataset);

protected:
    PlotPen penHSpeed_ = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
};

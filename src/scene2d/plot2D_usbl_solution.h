#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DUSBLSolution : public Plot2DLine {
public:
    Plot2DUSBLSolution();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen penAngle_[4] = {   PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 255, 0), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 0, 255), 2, PlotPen::LineStyleSolid),
                               PlotPen(PlotColor(0, 170, 155), 2, PlotPen::LineStyleSolid)};

    PlotPen penDist_ = PlotPen(PlotColor(155, 155, 155), 3, PlotPen::LineStylePoint);
};

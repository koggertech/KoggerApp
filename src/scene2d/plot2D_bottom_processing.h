#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DBottomProcessing : public Plot2DLine {
public:
    Plot2DBottomProcessing();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen _penLine = PlotPen(PlotColor(50, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penLine2 = PlotPen(PlotColor(200, 200, 0), 2, PlotPen::LineStyleSolid);
};


#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DDepth : public Plot2DLine {
public:
    Plot2DDepth();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen _penDepth = PlotPen(PlotColor(0, 255, 255), 2, PlotPen::LineStyleSolid);
};

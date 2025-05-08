#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DQuadrature : public Plot2DLine {
public:
    Plot2DQuadrature();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen _penReal = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penImag = PlotPen(PlotColor(0, 100, 250), 2, PlotPen::LineStyleSolid);
};

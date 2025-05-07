#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DAttitude : public Plot2DLine {
public:
    Plot2DAttitude();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen _penYaw = PlotPen(PlotColor(255, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penPitch = PlotPen(PlotColor(255, 0, 255), 2, PlotPen::LineStyleSolid);
    PlotPen _penRoll = PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid);
};

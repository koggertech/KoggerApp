#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DDVLSolution : public Plot2DLine {
public:
    Plot2DDVLSolution();
    bool draw(Plot2D* parent, Dataset* dataset);

protected:
    PlotPen _penVelocity[4] = {
        PlotPen(PlotColor(255, 0, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 255, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 0, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 170, 155), 2, PlotPen::LineStyleSolid)};

    PlotPen _penDist = PlotPen(PlotColor(155, 155, 155), 3, PlotPen::LineStylePoint);

//    PlotPen _penMode[4] = {
//        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStylePoint),
//        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStylePoint)};

//    PlotPen _penAmp[4] = {
//        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
//        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};
};


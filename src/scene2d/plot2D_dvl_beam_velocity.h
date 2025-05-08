#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DDVLBeamVelocity : public Plot2DLine {
public:
    Plot2DDVLBeamVelocity();
    bool draw(Plot2D* parent, Dataset* dataset);
    void setBeamFilter(int filter);

protected:
    int _beamFilter = 15;
    PlotPen _penBeam[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};

    PlotPen _penMode[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStylePoint)};

    PlotPen _penAmp[4] = {
        PlotPen(PlotColor(255, 0, 150), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(0, 155, 255), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(255, 175, 0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(75, 205, 55), 2, PlotPen::LineStyleSolid)};
};

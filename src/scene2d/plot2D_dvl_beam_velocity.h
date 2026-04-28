#pragma once

#include "plot2D_line.h"

class QColor;
class QPainter;
class Plot2D;
class Plot2DDVLBeamVelocity : public Plot2DLine {
public:
    Plot2DDVLBeamVelocity();
    bool draw(Plot2D* parent, Dataset* dataset) override;
    void setBeamFilter(int filter);
    int drawLegend(Canvas& canvas, int x, int y);
    int boxWidth(Canvas& canvas) const;
    bool hasData() const { return _hasData; }
    int countLegendItems() const;


protected:
    // 3 bits per beam: bit(i*3)=V, bit(i*3+1)=A, bit(i*3+2)=M
    int _beamFilter = 0xFFF;
    bool _hasData = false;
    // M (mode): brightest — base saturated color
    PlotPen _penMode[4] = {
        PlotPen(PlotColor(255,   0, 150), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(  0, 155, 255), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor(255, 175,   0), 2, PlotPen::LineStylePoint),
        PlotPen(PlotColor( 75, 205,  55), 2, PlotPen::LineStylePoint)};

    // V (velocity): 67% of M
    PlotPen _penBeam[4] = {
        PlotPen(PlotColor(171,   0, 100), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(  0, 104, 171), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(171, 117,   0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor( 50, 137,  37), 2, PlotPen::LineStyleSolid)};

    // A (distance/depth): 67% of V = ~45% of M
    PlotPen _penAmp[4] = {
        PlotPen(PlotColor(115,   0,  67), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(  0,  70, 115), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor(115,  78,   0), 2, PlotPen::LineStyleSolid),
        PlotPen(PlotColor( 34,  92,  25), 2, PlotPen::LineStyleSolid)};
};

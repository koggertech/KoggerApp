#pragma once

#include "plot2D_line.h"

class QColor;
class QPainter;
class Plot2D;
class Plot2DDVLSolution : public Plot2DLine {
public:
    Plot2DDVLSolution();
    bool draw(Plot2D* parent, Dataset* dataset) override;
    void setLineFilter(int filter) { _lineFilter = filter; }
    int drawLegend(Canvas& canvas, int x, int y);
    int boxWidth(Canvas& canvas) const;
    bool hasData() const { return _hasData; }
    int countLegendItems() const;


    // Bit flags for setLineFilter
    enum LineFlag {
        LineX   = 1 << 0,
        LineY   = 1 << 1,
        LineZ   = 1 << 2,
        LineAbs = 1 << 3,
        LineDst = 1 << 4,
    };

protected:
    int _lineFilter = LineX | LineY | LineZ | LineAbs | LineDst;
    bool _hasData = false;

    PlotPen _penVelocity[4] = {
        PlotPen(PlotColor(255,   0,   0), 2, PlotPen::LineStyleSolid),  // X: red
        PlotPen(PlotColor(180,   0, 255), 2, PlotPen::LineStyleSolid),  // Y: violet
        PlotPen(PlotColor(  0, 210, 210), 2, PlotPen::LineStyleSolid),  // Z: cyan
        PlotPen(PlotColor(255, 220,   0), 2, PlotPen::LineStyleSolid)}; // AbsV: yellow

    PlotPen _penDist = PlotPen(PlotColor(160, 160, 160), 3, PlotPen::LineStylePoint); // Dst: gray

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


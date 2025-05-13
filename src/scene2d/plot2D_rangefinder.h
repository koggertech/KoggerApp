#pragma once

#include "plot2D_line.h"


class Plot2D;
class Plot2DRangefinder : public Plot2DLine {
public:
    Plot2DRangefinder();
    bool draw(Plot2D* parent, Dataset* dataset);
    void setTheme(int theme_id);

protected:
    PlotPen penPoint_ = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStylePoint);
    PlotPen penLine_ = PlotPen(PlotColor(250, 100, 0), 2, PlotPen::LineStyleSolid);
    int themeId_ = 0;
};

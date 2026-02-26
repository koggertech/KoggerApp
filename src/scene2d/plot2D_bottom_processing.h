#pragma once

#include "plot2D_line.h"

class QColor;
class QPainter;
class QString;

class Plot2D;
class Plot2DBottomProcessing : public Plot2DLine {
public:
    Plot2DBottomProcessing();
    bool draw(Plot2D* parent, Dataset* dataset);
    void setDepthTextVisible(bool state) { drawDepthText_ = state; }

protected:
    PlotPen _penLine = PlotPen(PlotColor(50, 255, 0), 2, PlotPen::LineStyleSolid);
    PlotPen _penLine2 = PlotPen(PlotColor(200, 200, 0), 2, PlotPen::LineStyleSolid);
    bool drawDepthText_ = true;

private:
    QString formatDepthText(float distance) const;
    void drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor) const;
};

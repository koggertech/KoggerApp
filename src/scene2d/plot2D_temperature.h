#pragma once

#include "plot2D_plot_layer.h"

class QColor;
class QPainter;
class QString;

class Plot2D;
class Plot2DTemperature : public PlotLayer
{
public:
    Plot2DTemperature();
    bool draw(Plot2D* parent, Dataset* dataset) override;

private:
    QString formatTemperatureText(float temp) const;
    void drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor) const;
};

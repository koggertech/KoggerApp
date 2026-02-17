#include "plot2D_temperature.h"
#include "plot2D.h"
#include "math_defs.h"
#include <cmath>


Plot2DTemperature::Plot2DTemperature()
{}

bool Plot2DTemperature::draw(Plot2D* parent, Dataset* dataset)
{
    if (!isVisible() || !parent || !dataset) {
        return false;
    }

    auto& canvas = parent->canvas();
    QPainter* p = canvas.painter();
    if (!p) {
        return false;
    }

    float temp = dataset->getLastTemp();

    if (!isfinite(temp)) {
        Epoch* lastEpoch = dataset->last();
        Epoch* preLastEpoch = dataset->lastlast();

        if (lastEpoch && lastEpoch->temperatureAvail()) {
            temp = lastEpoch->temperature();
        }
        else if (preLastEpoch && preLastEpoch->temperatureAvail()) {
            temp = preLastEpoch->temperature();
        }
    }

    if (!isfinite(temp)) {
        return false;
    }

    QPen pen(QColor(255, 220, 0));
    p->setPen(pen);
    p->setFont(QFont("Asap", 30, QFont::Normal));

    const int imageHeight = canvas.height();
    const float val = round(temp * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    const QString tempText = QString::number(val, 'f', isInteger ? 0 : 1) + QString(QChar(0x00B0));
    const int x = 70;
    p->drawText(x, imageHeight - 15, tempText);

    return true;
}

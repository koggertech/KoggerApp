#include "plot2D_temperature.h"
#include "plot2D.h"
#include "math_defs.h"
#include "themes.h"
#include <cmath>
#include <QFontMetrics>

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

    const double s = renderScale();
    const int imageHeight = canvas.height();
    const QString tempText = formatTemperatureText(temp);
    const int x = qRound(70 * s);
    drawValueWithBackdrop(p, x, imageHeight - qRound(15 * s), tempText, QColor(255, 220, 0));

    return true;
}

QString Plot2DTemperature::formatTemperatureText(float temp) const
{
    const float val = std::round(temp * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 1) + QString(QChar(0x00B0));
}

void Plot2DTemperature::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor) const
{
    if (!painter) {
        return;
    }

    const double s = renderScale();
    const QFont font("Asap", qRound(30 * s), QFont::Normal);
    const QFontMetrics fm(font);
    const int padX = qRound(8 * s);
    const int padY = qRound(4 * s);
    const int radius = qRound(5 * s);
    const QRect bgRect(x - padX,
                       baselineY - fm.ascent() - padY,
                       fm.horizontalAdvance(text) + padX * 2,
                       fm.height() + padY * 2);

    const auto prevMode = painter->compositionMode();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 115));
    painter->drawRoundedRect(bgRect, radius, radius);

    painter->setFont(font);
    painter->setPen(QPen(textColor));
    painter->drawText(x, baselineY, text);
    painter->setCompositionMode(prevMode);
}

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
    const bool vertical = !parent->isHorizontal();
    const int imageHeight = canvas.height();
    const QString tempText = formatTemperatureText(temp);
    const int x = vertical ? qRound(15 * s) : qRound(70 * s);
    drawValueWithBackdrop(p, x, imageHeight - qRound(15 * s), tempText, QColor(255, 220, 0), vertical);

    return true;
}

QString Plot2DTemperature::formatTemperatureText(float temp) const
{
    const float val = std::round(temp * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 1) + QString(QChar(0x00B0));
}

void Plot2DTemperature::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor, bool vertical) const
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
    const int textW = fm.horizontalAdvance(text);

    const auto prevMode = painter->compositionMode();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Vertical mode: canvas globally rotated -90 (Plot2D::getImage); counter-rotate
    // +90 to keep the value upright, right-aligned so it stays inside the deep edge.
    painter->save();
    if (vertical) {
        painter->translate(x, baselineY);
        painter->rotate(90);
    }
    const int ax = vertical ? -textW : x;
    const int ay = vertical ? 0 : baselineY;
    const QRect bgRect(ax - padX,
                       ay - fm.ascent() - padY,
                       textW + padX * 2,
                       fm.height() + padY * 2);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 115));
    painter->drawRoundedRect(bgRect, radius, radius);
    painter->setFont(font);
    painter->setPen(QPen(textColor));
    painter->drawText(ax, ay, text);
    painter->restore();
    painter->setCompositionMode(prevMode);
}

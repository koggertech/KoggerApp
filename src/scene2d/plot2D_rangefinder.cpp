#include "plot2D_rangefinder.h"
#include "plot2D.h"
#include "math_defs.h"
#include "themes.h"
#include <cmath>
#include <QFontMetrics>

Plot2DRangefinder::Plot2DRangefinder()
{}

bool Plot2DRangefinder::draw(Plot2D* parent, Dataset* dataset)
{
    if (!parent || !dataset) {
        return false;
    }

    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible() || !cursor.distance.isValid()) {
        return false;
    }

    const bool needDrawLine = (themeId_ == 1 || themeId_ == 2);

    if (needDrawLine) {
        QVector<float> distance(canvas.width());
        distance.fill(NAN);

        for (int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch *data = dataset->fromIndex(pool_index);
            if (data != nullptr) {
                distance[i] = data->rangeFinder();
            }
        }

        if (themeId_ == 1) {
            drawY(canvas, distance, cursor.distance.from, cursor.distance.to, penLine_);
        }
        else {
            drawY(canvas, distance, cursor.distance.from, cursor.distance.to, penPoint_);
        }
    }

    return true;
}

bool Plot2DRangefinder::drawDepthValue(Plot2D* parent, Dataset* dataset)
{
    if (!parent || !dataset || !isVisible()) {
        return false;
    }

    const float rangefinderDepth = dataset->getLastRangefinderDepth();
    if (!drawDepthText_ || !std::isfinite(rangefinderDepth)) {
        return false;
    }

    auto& canvas = parent->canvas();
    QPainter* p = canvas.painter();
    if (!p) {
        return false;
    }

    const double s = renderScale();
    const bool vertical = !parent->isHorizontal();
    const QString rangefinderText = formatDepthText(rangefinderDepth);
    const int y = canvas.height() - qRound(15 * s);
    int x;
    if (vertical) {
        const int slot = parent->hasTemperatureValue() ? 1 : 0;
        x = qRound((15 + slot * 64) * s);
    } else {
        x = qRound(220 * s);
        if (!parent->hasTemperatureValue()) {
            x -= qRound(150 * s);
        }
    }
    drawValueWithBackdrop(p, x, y, rangefinderText, QColor(250, 100, 0), vertical);
    return true;
}

void Plot2DRangefinder::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

QString Plot2DRangefinder::formatDepthText(float distance) const
{
    const float val = std::round(distance * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
}

void Plot2DRangefinder::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor, bool vertical) const
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

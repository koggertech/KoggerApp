#include "plot2D_bottom_processing.h"
#include "plot2D.h"
#include "math_defs.h"
#include "themes.h"
#include <cmath>
#include <QFontMetrics>

Plot2DBottomProcessing::Plot2DBottomProcessing()
{}

bool Plot2DBottomProcessing::draw(Plot2D* parent, Dataset* dataset)
{
    if (!parent || !dataset) {
        return false;
    }

    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (!isVisible() || !cursor.distance.isValid()) {
        return false;
    }

    const bool needDrawLine = (themeId_ == 1 || themeId_ == 2);
    if (needDrawLine) {
        QVector<float> distance1(canvas.width());
        QVector<float> distance2(canvas.width());

        distance1.fill(NAN);
        distance2.fill(NAN);

        if (!cursor.channel2.isValid()) {
            for (int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch *data = dataset->fromIndex(pool_index);
                if (data != nullptr) {
                    distance1[i] = data->distProccesing(cursor.channel1);
                }
            }
        } else {
            for (int i = 0; i < canvas.width(); i++) {
                int pool_index = cursor.getIndex(i);
                Epoch *data = dataset->fromIndex(pool_index);
                if (data != nullptr) {
                    distance1[i] = -data->distProccesing(cursor.channel1);
                    distance2[i] = data->distProccesing(cursor.channel2);
                }
            }
        }

        const PlotPen::LineStyle lineStyle = (themeId_ == 2)
            ? PlotPen::LineStylePoint
            : PlotPen::LineStyleSolid;
        PlotPen linePen1 = _penLine;
        PlotPen linePen2 = _penLine2;
        linePen1.lineStyle = lineStyle;
        linePen2.lineStyle = lineStyle;

        drawY(canvas, distance1, cursor.distance.from, cursor.distance.to, linePen1);
        if (cursor.channel2.isValid()) {
            drawY(canvas, distance2, cursor.distance.from, cursor.distance.to, linePen2);
        }
    }

    return true;
}

bool Plot2DBottomProcessing::drawDepthValue(Plot2D* parent, Dataset* dataset)
{
    if (!parent || !dataset || !isVisible()) {
        return false;
    }

    const float bottomTrackDepth = dataset->getLastBottomTrackDepth();
    if (!drawDepthText_ || !std::isfinite(bottomTrackDepth)) {
        return false;
    }

    auto& canvas = parent->canvas();
    QPainter* p = canvas.painter();
    if (!p) {
        return false;
    }

    const double s = renderScale();
    const bool vertical = !parent->isHorizontal();
    const QString depthText = formatDepthText(bottomTrackDepth);
    int x;
    if (vertical) {
        const int slot = (parent->hasTemperatureValue() ? 1 : 0) + (parent->hasRangefinderDepthTextValue() ? 1 : 0);
        x = qRound((15 + slot * 64) * s);
    } else {
        x = qRound(370 * s);
        if (!parent->hasTemperatureValue()) {
            x -= qRound(150 * s);
        }
        if (!parent->hasRangefinderDepthTextValue()) {
            x -= qRound(150 * s);
        }
    }
    const int y = canvas.height() - qRound(15 * s);
    drawValueWithBackdrop(p, x, y, depthText, QColor(50, 255, 0), vertical);
    return true;
}

void Plot2DBottomProcessing::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

QString Plot2DBottomProcessing::formatDepthText(float distance) const
{
    const float val = std::round(distance * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
}

void Plot2DBottomProcessing::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor, bool vertical) const
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

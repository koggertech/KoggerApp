#include "plot2D_grid.h"
#include "plot2D.h"
#include "math_defs.h"
#include "themes.h"
#include <QFontMetrics>
#include <QPainterPath>
#include <QStringList>

namespace {

const QColor kLabelCasingColor(0, 0, 0);
const QColor kLabelPlateColor(0, 0, 0, 115);
constexpr double kLabelCasingBasePx = 2.0;
constexpr double kLabelPlatePadXPx = 8.0;
constexpr double kLabelPlatePadYPx = 4.0;
constexpr double kLabelPlateRadiusPx = 5.0;

double labelCasingWidth(double scale)
{
    return qMax(kLabelCasingBasePx, kLabelCasingBasePx * scale);
}

} // namespace

Plot2DGrid::Plot2DGrid() : angleVisibility_(false)
{}

int Plot2DGrid::labelLeftBleed(double scale) const
{
    return labelCasing_ ? qRound(labelCasingWidth(scale) * 0.5)
                        : qRound(kLabelPlatePadXPx * scale);
}

void Plot2DGrid::drawLabel(QPainter* painter, int x, int baselineY, const QString& text, bool vertical, bool rightAlign) const
{
    if (!painter || text.isEmpty()) {
        return;
    }

    const QFontMetrics fm(painter->font());
    const double s = renderScale();
    const QStringList lines = text.split(QChar('\n'));
    const int lineH = fm.height();
    int textW = 0;
    for (const QString& ln : lines) {
        textW = qMax(textW, fm.horizontalAdvance(ln));
    }

    const QColor coreColor = painter->pen().color();
    const bool prevAntialias = painter->testRenderHint(QPainter::Antialiasing);

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Vertical mode: the canvas is globally rotated -90 (Plot2D::getImage), so
    // counter-rotate +90 around the anchor to keep the label upright/readable.
    if (vertical) {
        painter->translate(x, baselineY);
        painter->rotate(90);
    }
    const int ax = vertical ? (rightAlign ? -textW : 0) : x;
    const int ay = vertical ? 0 : baselineY;

    if (labelCasing_) {
        painter->setRenderHint(QPainter::Antialiasing, true);

        QPainterPath glyphs;
        for (int li = 0; li < lines.size(); ++li) {
            glyphs.addText(ax, ay + li * lineH, painter->font(), lines[li]);
        }

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(kLabelCasingColor, labelCasingWidth(s), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(glyphs);

        painter->setPen(Qt::NoPen);
        painter->setBrush(coreColor);
        painter->drawPath(glyphs);
    }
    else {
        const int padX = qRound(kLabelPlatePadXPx * s);
        const int padY = qRound(kLabelPlatePadYPx * s);
        const int radius = qRound(kLabelPlateRadiusPx * s);
        const QRect plateRect(ax - padX,
                              ay - fm.ascent() - padY,
                              textW + padX * 2,
                              lineH * int(lines.size()) + padY * 2);

        painter->setPen(Qt::NoPen);
        painter->setBrush(kLabelPlateColor);
        painter->drawRoundedRect(plateRect, radius, radius);

        painter->setPen(coreColor);
        for (int li = 0; li < lines.size(); ++li) {
            painter->drawText(ax, ay + li * lineH, lines[li]);
        }
    }

    painter->restore();
    painter->setRenderHint(QPainter::Antialiasing, prevAntialias);
}

bool Plot2DGrid::draw(Plot2D* parent, Dataset* dataset)
{
    Q_UNUSED(dataset);
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible())
        return false;

    const double s = renderScale();
    const bool vertical = !parent->isHorizontal();

    QPen pen(_lineColor);
    pen.setWidth(qMax(1, qRound(_lineWidth * s)));

    QPainter* p = canvas.painter();
    p->setPen(pen);
    p->setFont(QFont("Asap", qRound(14 * s), QFont::Normal));
    QFontMetrics fm(p->font());

    const int imageHeight{ canvas.height() }, imageWidth{ canvas.width() },
        linesCount{ _lines }, textXOffset{ qRound(30 * s) }, textYOffset{ qRound(10 * s) };
    const int gridLineLen = qRound(180 * s);

    lastRightTextX_ = imageWidth; // reset to rightmost, updated to min during draw

    // линии
    for (int i = 1; i < linesCount; ++i) {
        const int posY = i * imageHeight / linesCount;

        QString velText, angleText, depthText;

        if (_velocityVisible && cursor.velocity.isValid()) { // velocity
            const float velFrom{ cursor.velocity.from }, velTo{ cursor.velocity.to },
                velRange{ velTo - velFrom }, attVal{ velRange * i / linesCount + velFrom };
            velText = QString::number(attVal, 'f', 2) + QObject::tr(" m/s");
        }
        if (angleVisibility_ && cursor.attitude.isValid()) { // angle
            const float attFrom{ cursor.attitude.from }, attTo{ cursor.attitude.to },
                attRange{ attTo - attFrom }, attVal{ attRange * i / linesCount + attFrom };
            angleText = QString::number(attVal, 'f', 0) + QStringLiteral("°");
        }
        if (cursor.distance.isValid()) { // depth
            const float distFrom{ cursor.distance.from }, distTo{ cursor.distance.to },
                distRange{ distTo - distFrom }, rangeVal{ distRange * i / linesCount + distFrom };
            depthText = QString::number(rangeVal, 'f', 2) + QObject::tr(" m");
        }

        QString lineText;
        if (vertical) {
            lineText = depthText;
            if (!angleText.isEmpty())
                lineText += (lineText.isEmpty() ? QString() : QStringLiteral("\n")) + angleText;
            if (!velText.isEmpty())
                lineText += (lineText.isEmpty() ? QString() : QStringLiteral("\n")) + velText;
        } else {
            if (!velText.isEmpty())
                lineText = velText;
            if (!angleText.isEmpty())
                lineText += (lineText.isEmpty() ? QString() : QStringLiteral("    ")) + angleText;
            if (!depthText.isEmpty())
                lineText += (lineText.isEmpty() ? QString() : QStringLiteral("    ")) + depthText;
        }

        int textW = 0;
        for (const QString& ln : lineText.split(QChar('\n')))
            textW = qMax(textW, fm.horizontalAdvance(ln));

        if (isFillWidth()) {
            p->drawLine(0, posY, imageWidth, posY);
        }
        else {
            const int lineLen = vertical ? gridLineLen : (textW + textXOffset);
            if (invert_) {
                p->drawLine(0, posY, lineLen, posY);
            }
            else {
                p->drawLine(imageWidth - lineLen, posY, imageWidth, posY);
            }
        }

        if (!lineText.isEmpty()) {
            const int nLines = (depthText.isEmpty() ? 0 : 1) + (angleText.isEmpty() ? 0 : 1) + (velText.isEmpty() ? 0 : 1);
            const int textX = vertical
                ? (invert_ ? (qRound(15 * s) + (nLines - 1) * fm.height()) : (imageWidth - textXOffset))
                : (invert_ ? textXOffset : (imageWidth - textW - textXOffset));
            if (!invert_)
                lastRightTextX_ = qMin(lastRightTextX_, textX - labelLeftBleed(s));
            drawLabel(p, textX, vertical ? (posY + textYOffset) : (posY - textYOffset), lineText, vertical);
        }
    }

    // глубина графика
    if (cursor.distance.isValid()) {
        p->setFont(QFont("Asap", qRound(30 * s), QFont::Normal));
        QFontMetrics fm2(p->font());
        float val{ cursor.distance.to };
        bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
        QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
        const int w = fm2.horizontalAdvance(rangeText);
        int x;
        if (vertical) {
            x = imageWidth - gridLineLen;
        } else {
            x = imageWidth - textXOffset / 2 - w;
        }
        drawLabel(p, x, imageHeight - qRound(15 * s), rangeText, vertical, true);
    }

    return true;
}

void Plot2DGrid::setAngleVisibility(bool state)
{
    angleVisibility_ = state;
}

#include "plot2D_grid.h"
#include "plot2D.h"
#include "math_defs.h"


Plot2DGrid::Plot2DGrid() : angleVisibility_(false)
{}

bool Plot2DGrid::draw(Plot2D* parent, Dataset* dataset)
{
    Q_UNUSED(dataset);
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (!isVisible()) {
        return false;
    }

    QPen pen(_lineColor);
    pen.setWidth(_lineWidth);

    QPainter* p = canvas.painter();
    p->setPen(pen);
    p->setFont(QFont("Asap", 14, QFont::Normal));
    QFontMetrics fm(p->font());

    const int imageHeight = canvas.height();
    const int imageWidth = canvas.width();
    const int linesCount = _lines;
    const int textXOffset = 30;
    const int textYOffset = 10;

    for (int i = 1; i < linesCount; ++i) {
        const int posY = i * imageHeight / linesCount;
        QString lineText;

        if (_velocityVisible && cursor.velocity.isValid()) {
            const float velFrom = cursor.velocity.from;
            const float velTo = cursor.velocity.to;
            const float velRange = velTo - velFrom;
            const float attVal = velRange * i / linesCount + velFrom;
            lineText.append(QString::number(attVal, 'f', 2) + QObject::tr(" m/s    "));
        }
        if (angleVisibility_ && cursor.attitude.isValid()) {
            const float attFrom = cursor.attitude.from;
            const float attTo = cursor.attitude.to;
            const float attRange = attTo - attFrom;
            const float attVal = attRange * i / linesCount + attFrom;
            lineText.append(QString::number(attVal, 'f', 0) + QStringLiteral(" deg    "));
        }
        if (cursor.distance.isValid()) {
            const float distFrom = cursor.distance.from;
            const float distTo = cursor.distance.to;
            const float distRange = distTo - distFrom;
            const float rangeVal = distRange * i / linesCount + distFrom;
            lineText.append(QString::number(rangeVal, 'f', 2) + QObject::tr(" m"));
        }

        const int textW = fm.horizontalAdvance(lineText);

        if (isFillWidth()) {
            p->drawLine(0, posY, imageWidth, posY);
        }
        else if (invert_) {
            p->drawLine(0, posY, textW + textXOffset, posY);
        }
        else {
            p->drawLine(imageWidth - textW - textXOffset, posY, imageWidth, posY);
        }

        if (!lineText.isEmpty()) {
            const int textX = invert_ ? textXOffset : (imageWidth - textW - textXOffset);
            p->drawText(textX, posY - textYOffset, lineText);
        }
    }

    if (cursor.distance.isValid()) {
        p->setFont(QFont("Asap", 26, QFont::Normal));
        QFontMetrics fm2(p->font());
        const float val = cursor.distance.to;
        const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
        const QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
        const int w = fm2.horizontalAdvance(rangeText);
        const int x = invert_ ? (textXOffset * 2) : (imageWidth - textXOffset / 2 - w);
        p->drawText(x, imageHeight - 10, rangeText);
    }

    return true;
}

void Plot2DGrid::setAngleVisibility(bool state)
{
    angleVisibility_ = state;
}

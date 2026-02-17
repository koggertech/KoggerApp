#include "plot2D_grid.h"
#include "plot2D.h"
#include "math_defs.h"


Plot2DGrid::Plot2DGrid() : angleVisibility_(false)
{}

bool Plot2DGrid::draw(Plot2D* parent, Dataset* dataset)
{
    Q_UNUSED(dataset);
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible())
        return false;

    QPen pen(_lineColor);
    pen.setWidth(_lineWidth);

    QPainter* p = canvas.painter();
    p->setPen(pen);
    p->setFont(QFont("Asap", 14, QFont::Normal));
    QFontMetrics fm(p->font());

    const int imageHeight{ canvas.height() }, imageWidth{ canvas.width() },
        linesCount{ _lines }, textXOffset{ 30 }, textYOffset{ 10 };

    // линии
    for (int i = 1; i < linesCount; ++i) {
        const int posY = i * imageHeight / linesCount;

        QString lineText;

        if (_velocityVisible && cursor.velocity.isValid()) { // velocity
            const float velFrom{ cursor.velocity.from }, velTo{ cursor.velocity.to },
                velRange{ velTo - velFrom }, attVal{ velRange * i / linesCount + velFrom };
            lineText.append({ QString::number(attVal , 'f', 2) + QObject::tr(" m/s    ")});
        }
        if (angleVisibility_ && cursor.attitude.isValid()) { // angle
            const float attFrom{ cursor.attitude.from }, attTo{ cursor.attitude.to },
                attRange{ attTo - attFrom }, attVal{ attRange * i / linesCount + attFrom };
            QString text{ QString::number(attVal, 'f', 0) + QStringLiteral("°    ") };
            lineText.append(text);
        }
        if (cursor.distance.isValid()) { // depth
            const float distFrom{ cursor.distance.from }, distTo{ cursor.distance.to },
                distRange{ distTo - distFrom }, rangeVal{ distRange * i / linesCount + distFrom };
            lineText.append( { QString::number(rangeVal, 'f', 2) + QObject::tr(" m") } );
        }

        const int textW = fm.horizontalAdvance(lineText);

        if (isFillWidth()) {
            p->drawLine(0, posY, imageWidth, posY);
        }
        else {
            if (invert_) {
                p->drawLine(0, posY, textW + textXOffset, posY);
            }
            else {
                p->drawLine(imageWidth - textW - textXOffset, posY, imageWidth, posY);
            }
        }

        if (!lineText.isEmpty()) {
            const int textX = invert_ ? textXOffset : (imageWidth - textW - textXOffset);
            p->drawText(textX, posY - textYOffset, lineText);
        }
    }

    // глубина графика
    if (cursor.distance.isValid()) {
        p->setFont(QFont("Asap", 26, QFont::Normal));
        QFontMetrics fm2(p->font());
        float val{ cursor.distance.to };
        bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
        QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
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

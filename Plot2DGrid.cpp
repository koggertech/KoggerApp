#include "Plot2D.h"


Plot2DGrid::Plot2DGrid() : angleVisibility_(false)
{}


bool Plot2DGrid::draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor)
{
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

    for (int i = 1; i < linesCount; ++i) {
        const int posY = i * imageHeight / linesCount;
        p->drawLine(0, posY, imageWidth, posY); // line

        QString lineText;

        if (_velocityVisible && cursor.velocity.isValid()) { // velocity
            const float velFrom{ cursor.velocity.from }, velTo{ cursor.velocity.to },
                velRange{ velTo - velFrom }, attVal{ velRange * i / linesCount + velFrom };
            lineText.append({ QString::number(attVal , 'f', 2) + QStringLiteral(" m/s\t") });
        }
        if (angleVisibility_ && cursor.attitude.isValid()) { // angle
            const float attFrom{ cursor.attitude.from }, attTo{ cursor.attitude.to },
                attRange{ attTo - attFrom }, attVal{ attRange * i / linesCount + attFrom };
            QString text{ QString::number(attVal, 'f', 0) + QStringLiteral("°\t") };
            lineText.append(text);
        }
        if (cursor.distance.isValid()) { // depth
            const float distFrom{ cursor.distance.from }, distTo{ cursor.distance.to },
                distRange{ distTo - distFrom }, rangeVal{ distRange * i / linesCount + distFrom };
            lineText.append( { QString::number(rangeVal, 'f', 2) + QStringLiteral(" m") } );
        }

        if (!lineText.isEmpty())
            p->drawText(imageWidth - fm.horizontalAdvance(lineText) - textXOffset, posY - textYOffset, lineText);
    }

    if (cursor.distance.isValid()) {
        p->setFont(QFont("Asap", 26, QFont::Normal));
        float val{ cursor.distance.to };
        QString range_text = QString::number(val, 'f', (val == static_cast<int>(val)) ? 0 : 2) + QStringLiteral(" m");
        p->drawText(imageWidth - textXOffset / 2 - range_text.count() * 25, imageHeight - 10, range_text);
    }

    if (_rangeFinderLastVisible && cursor.distance.isValid()) {
        Epoch* lastEpoch = dataset->last();
        Epoch* preLastEpoch = dataset->lastlast();
        float distance = NAN;

        if (lastEpoch != NULL && isfinite(lastEpoch->rangeFinder()))
            distance = lastEpoch->rangeFinder();
        else if (preLastEpoch != NULL && isfinite(preLastEpoch->rangeFinder()))
            distance = preLastEpoch->rangeFinder();

        if (isfinite(distance)) {
            pen.setColor(QColor(250, 100, 0));
            p->setPen(pen);
            p->setFont(QFont("Asap", 40, QFont::Normal));
            float val{ round(distance * 100.f) / 100.f };
            QString rangeText = QString::number(val, 'f', (val == static_cast<int>(val)) ? 0 : 2) + QStringLiteral(" m");
            p->drawText(imageWidth / 2 - rangeText.count() * 32, imageHeight - 15, rangeText);
        }
    }

    return true;
}

void Plot2DGrid::setAngleVisibility(bool state)
{
    angleVisibility_ = state;
}

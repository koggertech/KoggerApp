#include "plot2D_grid.h"
#include "plot2D.h"

constexpr float epsilon = 0.001f;

Plot2DGrid::Plot2DGrid() : angleVisibility_(false)
{}

bool Plot2DGrid::draw(Plot2D* parent, Dataset* dataset)
{
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

        if (isFillWidth())
            p->drawLine(0, posY, imageWidth, posY);
        else
            p->drawLine(imageWidth - fm.horizontalAdvance(lineText) - textXOffset, posY, imageWidth, posY); // line

        if (!lineText.isEmpty())
            p->drawText(imageWidth - fm.horizontalAdvance(lineText) - textXOffset, posY - textYOffset, lineText);
    }

    if (cursor.distance.isValid()) {
        p->setFont(QFont("Asap", 26, QFont::Normal));
        float val{ cursor.distance.to };
        bool isInteger = std::abs(val - std::round(val)) < epsilon;
        QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
        p->drawText(imageWidth - textXOffset / 2 - rangeText.count() * 25, imageHeight - 10, rangeText);
    }

    if (_rangeFinderLastVisible && cursor.distance.isValid()) {
        Epoch* lastEpoch = dataset->last();
        Epoch* preLastEpoch = dataset->lastlast();
        float distance = NAN;

        if (lastEpoch != NULL && isfinite(lastEpoch->rangeFinder())) {
            distance = lastEpoch->rangeFinder();
        }
        else if (preLastEpoch != NULL && isfinite(preLastEpoch->rangeFinder())) {
            distance = preLastEpoch->rangeFinder();
        }

        if (isfinite(distance)) {
            pen.setColor(QColor(250, 100, 0));
            p->setPen(pen);
            p->setFont(QFont("Asap", 40, QFont::Normal));
            float val{ round(distance * 100.f) / 100.f };
            bool isInteger = std::abs(val - std::round(val)) < epsilon;
            QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
            p->drawText(imageWidth / 2 - rangeText.count() * 32, imageHeight - 15, rangeText);
        }
    }

    if(true) {
        Epoch* lastEpoch = dataset->last();
        Epoch* preLastEpoch = dataset->lastlast();
        float temp = NAN;
        temp = dataset->getLastTemp();

        // qDebug() << "Plot temp def: " << temp;

        // if (lastEpoch != NULL && isfinite(lastEpoch->temperatureAvail())) {
        //     temp = lastEpoch->temperature();
        //     qDebug() << "Plot temp one: " << temp;
        // }
        // else if (preLastEpoch != NULL && isfinite(preLastEpoch->temperatureAvail())) {
        //     temp = preLastEpoch->temperature();
        //     qDebug() << "Plot temp sec: " << temp;
        // } else if() {

        // if (lastEpoch != NULL && isfinite(lastEpoch->temperatureAvail())) {
        //     temp = preLastEpoch->temperature();
        //     qDebug() << "Plot temp sec: " << temp;
        // }

        // }
        // qDebug() << "Plot temp end: " << temp;

        if (isfinite(temp)) {
            pen.setColor(QColor(80, 200, 0));
            p->setPen(pen);
            p->setFont(QFont("Asap", 40, QFont::Normal));
            float val{ round(temp * 100.f) / 100.f };
            bool isInteger = std::abs(val - std::round(val)) < epsilon;
            QString rangeText = QString::number(val, 'f', isInteger ? 0 : 1) + QObject::tr("°");
            p->drawText(imageWidth / 2 - 300, imageHeight - 15, rangeText);
        }
    }

    return true;
}

void Plot2DGrid::setAngleVisibility(bool state)
{
    angleVisibility_ = state;
}

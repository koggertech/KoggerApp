#include "plot2D_rangefinder.h"
#include "plot2D.h"
#include "math_defs.h"
#include <cmath>


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
            if (data != NULL) {
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

    float lastDistance = NAN;

    Epoch* lastEpoch = dataset->last();
    Epoch* preLastEpoch = dataset->lastlast();

    if (lastEpoch != nullptr && std::isfinite(lastEpoch->rangeFinder())) {
        lastDistance = lastEpoch->rangeFinder();
    }
    else if (preLastEpoch != nullptr && std::isfinite(preLastEpoch->rangeFinder())) {
        lastDistance = preLastEpoch->rangeFinder();
    }

    if (std::isfinite(lastDistance)) {
        QPainter* p = canvas.painter();
        if (p != nullptr) {
            QPen pen(QColor(250, 100, 0));
            p->setPen(pen);
            p->setFont(QFont("Asap", 40, QFont::Normal));

            const float val = std::round(lastDistance * 100.f) / 100.f;
            const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
            const QString rangeText = QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
            p->drawText(canvas.width() / 2 - rangeText.size() * 32, canvas.height() - 15, rangeText);
        }
    }

    return true;
}

void Plot2DRangefinder::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

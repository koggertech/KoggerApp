#include "plot2D_rangefinder.h"
#include "plot2D.h"
#include "math_defs.h"
#include <cmath>

namespace {
QString formatDepthText(float distance)
{
    const float val = std::round(distance * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
}
}


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

    const float rangefinderDepth = dataset->getLastRangefinderDepth();
    if (std::isfinite(rangefinderDepth)) {
        QPainter* p = canvas.painter();
        if (p != nullptr) {
            p->setFont(QFont("Asap", 30, QFont::Normal));
            const QString rangefinderText = formatDepthText(rangefinderDepth);
            const int y = canvas.height() - 15;
            const int x = 220;
            p->setPen(QPen(QColor(250, 100, 0)));
            p->drawText(x, y, rangefinderText);
        }
    }

    return true;
}

void Plot2DRangefinder::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

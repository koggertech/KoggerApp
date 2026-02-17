#include "plot2D_bottom_processing.h"
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

    QVector<float> distance1(canvas.width());
    QVector<float> distance2(canvas.width());

    distance1.fill(NAN);
    distance2.fill(NAN);

    if (!cursor.channel2.isValid()) {
        for (int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch *data = dataset->fromIndex(pool_index);
            if (data != NULL) {
                distance1[i] = data->distProccesing(cursor.channel1);
            }
        }
    } else {
        for (int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch *data = dataset->fromIndex(pool_index);
            if (data != NULL) {
                distance1[i] = -data->distProccesing(cursor.channel1);
                distance2[i] = data->distProccesing(cursor.channel2);
            }
        }
    }

    drawY(canvas, distance1, cursor.distance.from, cursor.distance.to, _penLine);
    if (cursor.channel2.isValid()) {
        drawY(canvas, distance2, cursor.distance.from, cursor.distance.to,
              _penLine2);
    }

    const float bottomTrackDepth = dataset->getLastBottomTrackDepth();
    if (drawDepthText_ && std::isfinite(bottomTrackDepth)) {
        QPainter* p = canvas.painter();
        if (p != nullptr) {
            p->setFont(QFont("Asap", 30, QFont::Normal));
            const QString depthText = formatDepthText(bottomTrackDepth);
            const int x = 370;
            const int y = canvas.height() - 15;
            p->setPen(QPen(QColor(50, 255, 0)));
            p->drawText(x, y, depthText);
        }
    }

    return true;
}

#include "plot2D_bottom_processing.h"
#include "plot2D.h"
#include "math_defs.h"
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
            const QString depthText = formatDepthText(bottomTrackDepth);
            int x = 370;
            if (!parent->hasTemperatureValue()) {
                x -= 150;
            }
            if (!parent->hasRangefinderDepthTextValue()) {
                x -= 150;
            }
            const int y = canvas.height() - 15;
            drawValueWithBackdrop(p, x, y, depthText, QColor(50, 255, 0));
        }
    }

    return true;
}

QString Plot2DBottomProcessing::formatDepthText(float distance) const
{
    const float val = std::round(distance * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
}

void Plot2DBottomProcessing::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor) const
{
    if (!painter) {
        return;
    }

    const QFont font("Asap", 30, QFont::Normal);
    const QFontMetrics fm(font);
    const int padX = 8;
    const int padY = 4;
    const int radius = 5;
    const QRect bgRect(x - padX,
                       baselineY - fm.ascent() - padY,
                       fm.horizontalAdvance(text) + padX * 2,
                       fm.height() + padY * 2);

    const auto prevMode = painter->compositionMode();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 115));
    painter->drawRoundedRect(bgRect, radius, radius);

    painter->setFont(font);
    painter->setPen(QPen(textColor));
    painter->drawText(x, baselineY, text);
    painter->setCompositionMode(prevMode);
}

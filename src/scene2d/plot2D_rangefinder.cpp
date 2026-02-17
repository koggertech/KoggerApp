#include "plot2D_rangefinder.h"
#include "plot2D.h"
#include "math_defs.h"
#include <cmath>
#include <QFontMetrics>


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
    if (drawDepthText_ && std::isfinite(rangefinderDepth)) {
        QPainter* p = canvas.painter();
        if (p != nullptr) {
            const QString rangefinderText = formatDepthText(rangefinderDepth);
            const int y = canvas.height() - 15;
            int x = 220;
            if (!parent->hasTemperatureValue()) {
                x -= 150;
            }
            drawValueWithBackdrop(p, x, y, rangefinderText, QColor(250, 100, 0));
        }
    }

    return true;
}

void Plot2DRangefinder::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

QString Plot2DRangefinder::formatDepthText(float distance) const
{
    const float val = std::round(distance * 100.f) / 100.f;
    const bool isInteger = std::abs(val - std::round(val)) < kmath::fltEps;
    return QString::number(val, 'f', isInteger ? 0 : 2) + QObject::tr(" m");
}

void Plot2DRangefinder::drawValueWithBackdrop(QPainter* painter, int x, int baselineY, const QString& text, const QColor& textColor) const
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

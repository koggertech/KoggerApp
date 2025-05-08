#include "plot2D_rangefinder.h"
#include "plot2D.h"


Plot2DRangefinder::Plot2DRangefinder()
{}

bool Plot2DRangefinder::draw(Plot2D* parent, Dataset* dataset)
{
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible() || !cursor.distance.isValid()) {
        return false;
    }

    if (themeId_ < 1) {
        return true;
    }

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

    if (themeId_ == 2) {
        drawY(canvas, distance, cursor.distance.from, cursor.distance.to, penPoint_);
    }

    return true;
}

void Plot2DRangefinder::setTheme(int theme_id)
{
    themeId_ = theme_id;
}

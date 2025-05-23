#include "plot2D_gnss.h"
#include "plot2D.h"


Plot2DGNSS::Plot2DGNSS()
{}

bool Plot2DGNSS::draw(Plot2D* parent, Dataset *dataset)
{
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible() || !cursor.velocity.isValid()) {
        return false;
    }

    QVector<float> h_speed(canvas.width());
    h_speed.fill(NAN);

    for (int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch *data = dataset->fromIndex(pool_index);

        if (data != NULL) {
            h_speed[i] = data->gnssHSpeed();
        }
    }

    drawY(canvas, h_speed, cursor.velocity.from, cursor.velocity.to, penHSpeed_);

    return true;
}

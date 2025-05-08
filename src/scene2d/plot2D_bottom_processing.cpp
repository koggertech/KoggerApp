#include "plot2D_bottom_processing.h"
#include "plot2D.h"


Plot2DBottomProcessing::Plot2DBottomProcessing()
{}

bool Plot2DBottomProcessing::draw(Plot2D* parent, Dataset* dataset)
{
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

    return true;
}

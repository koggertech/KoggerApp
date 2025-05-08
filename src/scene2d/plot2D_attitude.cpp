#include "plot2D_attitude.h"
#include "plot2D.h"


Plot2DAttitude::Plot2DAttitude()
{}

bool Plot2DAttitude::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if(!isVisible() || !cursor.attitude.isValid()) { return false; }

    QVector<float> yaw(canvas.width());
    QVector<float> pitch(canvas.width());
    QVector<float> roll(canvas.width());

    for(int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch* data = dataset->fromIndex(pool_index);

        if(data != NULL && data->isAttAvail()) {
            yaw[i] = data->yaw();
            pitch[i] = data->pitch();
            roll[i] = data->roll();
        } else {
            yaw[i] = NAN;
            pitch[i] = NAN;
            roll[i] = NAN;
        }
    }

    drawY(canvas, yaw, cursor.attitude.from, cursor.attitude.to, _penYaw);
    drawY(canvas, pitch, cursor.attitude.from, cursor.attitude.to, _penPitch);
    drawY(canvas, roll, cursor.attitude.from, cursor.attitude.to, _penRoll);

    return true;
}

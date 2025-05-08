#include "plot2D_encoder.h"
#include "plot2D.h"


Plot2DEncoder::Plot2DEncoder()
{}

bool Plot2DEncoder::draw(Plot2D *parent, Dataset *dataset) {
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();
    
    if (!isVisible() || !cursor.attitude.isValid()) {
        return false;
    }
    
    QVector<float> yaw(canvas.width());
    QVector<float> pitch(canvas.width());
    QVector<float> roll(canvas.width());
    
    for (int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch *data = dataset->fromIndex(pool_index);
        
        if (data != NULL && data->isEncodersSeted()) {
            yaw[i] = data->encoder1();
            pitch[i] = data->encoder2();
            roll[i] = data->encoder3();
        } else {
            yaw[i] = NAN;
            pitch[i] = NAN;
            roll[i] = NAN;
        }
    }
    
    drawY(canvas, yaw, cursor.attitude.from, cursor.attitude.to, penYaw_);
    drawY(canvas, pitch, cursor.attitude.from, cursor.attitude.to, penPitch_);
    
    return true;
}

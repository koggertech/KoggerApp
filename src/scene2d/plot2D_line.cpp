#include "plot2D_line.h"

Plot2DLine::Plot2DLine()
{
}

bool Plot2DLine::drawY(Canvas &canvas, QVector<float> data, float value_from, float value_to, const PlotPen &pen) {
    if(canvas.width() != data.size()) { return false; }

    QVector<float> data_maped(data.size());

    const float canvas_height = canvas.height();
    float value_range = value_to - value_from;
    float value_scale = canvas_height/value_range;

    for(int i = 0; i < data_maped.size(); i++) {
        float y = (data[i] - value_from)*value_scale;
        if(y < 0) { y = 0; }
        else if(y > canvas_height) { y = canvas_height; }
        data_maped[i] = y;
    }

    canvas.drawY(data_maped, pen);

    return true;
}

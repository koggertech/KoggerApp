#include "plot2D_depth.h"
#include "plot2D.h"


Plot2DDepth::Plot2DDepth()
{}

bool Plot2DDepth::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if(!isVisible() || !cursor.distance.isValid()) { return false; }

    QVector<float> depth(canvas.width());

    for(int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch* data = dataset->fromIndex(pool_index);

        if(data != NULL && data->isAttAvail()) {
            depth[i] = data->getDepth();
        } else {
            depth[i] = NAN;
        }
    }

    drawY(canvas, depth, cursor.distance.from, cursor.distance.to, _penDepth);

    return true;
}

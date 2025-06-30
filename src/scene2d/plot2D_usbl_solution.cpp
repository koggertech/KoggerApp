#include "plot2D_usbl_solution.h"
#include "plot2D.h"


Plot2DUSBLSolution::Plot2DUSBLSolution()
{}

bool Plot2DUSBLSolution::draw(Plot2D *parent, Dataset *dataset)
{
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    if (!isVisible() || !cursor.distance.isValid()) {
        return false;
    }

    QVector<float> azimuth(canvas.width());
    azimuth.fill(NAN);

    QVector<float> elevation(canvas.width());
    elevation.fill(NAN);

    QVector<float> distance(canvas.width());
    distance.fill(NAN);

    for (int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch *data = dataset->fromIndex(pool_index);

        if (data != NULL && data->isUsblSolutionAvailable()) {
            IDBinUsblSolution::UsblSolution sol = data->usblSolution();

            azimuth[i] = sol.azimuth_deg;
            elevation[i] = sol.elevation_deg;
            distance[i] = sol.distance_m;
        }
    }

    drawY(canvas, azimuth, cursor.attitude.from, cursor.attitude.to, penAngle_[0]);
    drawY(canvas, elevation, cursor.attitude.from, cursor.attitude.to, penAngle_[1]);
    drawY(canvas, distance, cursor.distance.from, cursor.distance.to, penDist_);

    return true;
}

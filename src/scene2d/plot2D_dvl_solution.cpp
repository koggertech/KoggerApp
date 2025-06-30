#include "plot2D_dvl_solution.h"
#include "plot2D.h"


Plot2DDVLSolution::Plot2DDVLSolution()
{}

bool Plot2DDVLSolution::draw(Plot2D *parent, Dataset *dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (!isVisible() || !cursor.velocity.isValid()) {
        return false;
    }

    QVector<float> velocityX(canvas.width());
    velocityX.fill(NAN);

    QVector<float> velocityY(canvas.width());
    velocityY.fill(NAN);

    QVector<float> velocityZ(canvas.width());
    velocityZ.fill(NAN);

    QVector<float> velocityA(canvas.width());
    velocityA.fill(NAN);

    QVector<float> distance(canvas.width());
    distance.fill(NAN);

    for (int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch *data = dataset->fromIndex(pool_index);

        if (data != NULL && data->isDVLSolutionAvail()) {
            IDBinDVL::DVLSolution sol = data->dvlSolution();
            velocityX[i] = sol.velocity.x;
            velocityY[i] = sol.velocity.y;
            velocityZ[i] = sol.velocity.z;
            velocityA[i] = sqrt(sol.velocity.x * sol.velocity.x +
                                sol.velocity.y * sol.velocity.y +
                                sol.velocity.z * sol.velocity.z);
            distance[i] = sol.distance.z;
        }
    }

    drawY(canvas, velocityX, cursor.velocity.from, cursor.velocity.to,
          _penVelocity[0]);
    drawY(canvas, velocityY, cursor.velocity.from, cursor.velocity.to,
          _penVelocity[1]);
    drawY(canvas, velocityZ, cursor.velocity.from, cursor.velocity.to,
          _penVelocity[2]);
    drawY(canvas, velocityA, cursor.velocity.from, cursor.velocity.to,
          _penVelocity[3]);
    drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penDist);

    return true;
}

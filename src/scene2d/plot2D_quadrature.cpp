#include "plot2D_quadrature.h"
#include "plot2D.h"


Plot2DQuadrature::Plot2DQuadrature()
{}

bool Plot2DQuadrature::draw(Plot2D* parent, Dataset* dataset)
{
    auto &canvas = parent->canvas();
    auto &cursor = parent->cursor();

    Q_UNUSED(dataset);

    if (!isVisible() || !cursor.distance.isValid()) {
        return false;
    }

    QVector<float> real1(canvas.width());
    QVector<float> imag1(canvas.width());
    real1.fill(NAN);
    imag1.fill(NAN);

    QVector<float> real2(canvas.width());
    QVector<float> imag2(canvas.width());
    real2.fill(NAN);
    imag2.fill(NAN);

    QVector<float> real3(canvas.width());
    QVector<float> imag3(canvas.width());
    real3.fill(NAN);
    imag3.fill(NAN);

    QVector<float> real4(canvas.width());
    QVector<float> imag4(canvas.width());
    real4.fill(NAN);
    imag4.fill(NAN);

    // for(int i = 0; i < canvas.width(); i++) {
    //     int pool_index = cursor.getIndex(i);
    //     Epoch* data = dataset->fromIndex(pool_index);
    //     if(data != NULL) {
    //         QByteArray raw = data->complexSignalData();
    //         if(raw.size() > 0) {
    //             const int16_t* data = (int16_t*)raw.data();
    //             real1[i] = data[0+50];
    //             imag1[i] = data[1+50];

    //             real2[i] = data[2+50];
    //             imag2[i] = data[3+50];

    //             real3[i] = data[4+50];
    //             imag3[i] = data[5+50];

    //             real4[i] = data[6+50];
    //             imag4[i] = data[7+50];
    //         }
    //     }
    // }

    // drawY(canvas, real1, -3200+1500, 3200+1500, _penReal);
    // drawY(canvas, imag1, -3200+1500, 3200+1500, _penImag);

    // drawY(canvas, real2, -3200+768, 3200+768, _penReal);
    // drawY(canvas, imag2, -3200+768, 3200+768, _penImag);

    // drawY(canvas, real3, -3200-768, 3200-768, _penReal);
    // drawY(canvas, imag3, -3200-768, 3200-768, _penImag);

    // drawY(canvas, real4, -3200-1500, 3200-1500, _penReal);
    // drawY(canvas, imag4, -3200-1500, 3200-1500, _penImag);

    return true;
}

#include "plot2D_dvl_beam_velocity.h"
#include "plot2D.h"


Plot2DDVLBeamVelocity::Plot2DDVLBeamVelocity()
{}

bool Plot2DDVLBeamVelocity::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (!isVisible() || !cursor.velocity.isValid()) {
        return false;
    }

    QVector<float> beam_velocity(canvas.width());
    beam_velocity.fill(NAN);
    QVector<float> beam_amp(canvas.width());
    beam_amp.fill(NAN);
    QVector<float> beam_mode(canvas.width());
    beam_mode.fill(NAN);
    QVector<float> beam_coh(canvas.width());
    beam_coh.fill(NAN);
    QVector<float> beam_dist(canvas.width());
    beam_dist.fill(NAN);

    for (int ibeam = 0; ibeam < 4; ibeam++) {
        if (((_beamFilter >> ibeam) & 1) == 0) {
            continue;
        }

        for (int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch *data = dataset->fromIndex(pool_index);

            if (data != NULL && data->isDopplerBeamAvail(ibeam)) {
                IDBinDVL::BeamSolution beam = data->dopplerBeam(ibeam);
                beam_velocity[i] = beam.velocity;
                beam_amp[i] = beam.amplitude * 0.1f;
                beam_mode[i] = (beam.mode * 6 + ibeam) * 2;
                beam_coh[i] = beam.coherence[0];
                beam_dist[i] = beam.distance;
            }
        }

        drawY(canvas, beam_velocity, cursor.velocity.from, cursor.velocity.to,
              _penBeam[ibeam]);
        //            drawY(canvas, beam_amp, 0, 100, _penAmp[ibeam]);
        //            drawY(canvas, beam_coh, 0, 1000, _penAmp[ibeam]);
        drawY(canvas, beam_mode, canvas.height(), 0, _penMode[ibeam]);
        drawY(canvas, beam_dist, cursor.distance.from, cursor.distance.to,
              _penAmp[ibeam]);
    }

    return true;
}

void Plot2DDVLBeamVelocity::setBeamFilter(int filter)
{
    _beamFilter = filter;
}

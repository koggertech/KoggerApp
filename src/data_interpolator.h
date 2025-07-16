#pragma once

#include <math.h>
#include <time.h>

#include "dataset_defs.h"


class Dataset;
class DataInterpolator {
public:
    explicit DataInterpolator(Dataset* datasetPtr);

    void interpolatePos(bool fromStart);
    void interpolateYaw(bool fromStart);
    void clear();

private:
    float interpYaw(float start, float end, float progress) const;
    NED interpNED(const NED& start, const NED& end, float progress) const;
    LLA interpLLA(const LLA& start, const LLA& end, float progress) const;
    float interpDist(float start, float end, float progress) const;
    qint64 calcTimeDiffInNanoSecs(time_t startSecs, int startNanoSecs, time_t endSecs, int endNanoSecs) const;
    qint64 convertToNanosecs(time_t secs, int nanoSecs) const;
    std::pair<time_t, int> convertFromNanosecs(qint64 totalNanoSecs) const; // first - secs, second - nanosecs

private:
    Dataset* datasetPtr_;
    int lastYawInterpIndx_;
    int lastPosInterpIndx_;
};

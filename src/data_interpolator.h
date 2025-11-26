#pragma once

#include <math.h>
#include <time.h>

#include "dataset_defs.h"


class Dataset;
class DataInterpolator {
public:
    explicit DataInterpolator(Dataset* datasetPtr);

    void interpolatePos(bool fromStart);
    void interpolateAtt(bool fromStart);
    void clear();

private:
    float interpAttParam(float start, float end, float progress) const;
    NED interpNED(const NED& start, const NED& end, float progress) const;
    LLA interpLLA(const LLA& start, const LLA& end, float progress) const;
    float interpDist(float start, float end, float progress) const;
    qint64 calcTimeDiffInNanoSecs(time_t startSecs, int startNanoSecs, time_t endSecs, int endNanoSecs) const;
    qint64 convertToNanosecs(time_t secs, int nanoSecs) const;
    std::pair<time_t, int> convertFromNanosecs(qint64 totalNanoSecs) const; // first - secs, second - nanosecs

private:
    const quint64 oneSecInNsecs = 1000000000ULL;
    const quint64 oneHundredNsecs = 100000000ULL;
    const quint64 zeroNsecs = 0ULL;

    Dataset* datasetPtr_;
    int lastAttInterpIndx_;
    int lastPosInterpIndx_;
};

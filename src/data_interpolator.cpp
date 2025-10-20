#include "data_interpolator.h"

#include "dataset.h"


DataInterpolator::DataInterpolator(Dataset *datasetPtr) :
    datasetPtr_(datasetPtr),
    lastAttInterpIndx_(0),
    lastPosInterpIndx_(0)
{}

void DataInterpolator::interpolatePos(bool fromStart)
{
    if (!datasetPtr_) {
        return;
    }

    int startEpochIndx = fromStart ? 0 : lastPosInterpIndx_;
    int endEpochIndx = datasetPtr_->size();

    if ((endEpochIndx - startEpochIndx) < 1) {
        return;
    }

    int firstValidIndex = startEpochIndx;
    //qDebug() << "firstValidIndex" << firstValidIndex;
    bool beenInterp = false;

    while (firstValidIndex < endEpochIndx) {
        while (firstValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(firstValidIndex); ep) {
                if (ep->getPositionDataType() == DataType::kRaw) {
                    break;
                }
                ++firstValidIndex;
            }
        }

        int secondValidIndex = firstValidIndex + 1;
        while (secondValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(secondValidIndex); ep) {
                if (ep->getPositionDataType() == DataType::kRaw) {
                    break;
                }
                ++secondValidIndex;
            }
        }

        if (secondValidIndex > endEpochIndx) {
            break;
        }

        int numInterpIndx = secondValidIndex - firstValidIndex;
        if (numInterpIndx < 1) {
            firstValidIndex = secondValidIndex;
            continue;
        }

        auto* startEpoch = datasetPtr_->fromIndex(firstValidIndex);
        auto* endEpoch   = datasetPtr_->fromIndex(secondValidIndex);
        if (!startEpoch || !endEpoch) {
            continue;
        }

        auto startPos = startEpoch->getPositionGNSS();
        auto endPos = endEpoch->getPositionGNSS();

        const int numSteps = secondValidIndex - firstValidIndex;

        const quint64 startTimeNs = convertToNanosecs(startPos.time.sec, startPos.time.nanoSec);
        const quint64 endTimeNs   = convertToNanosecs(endPos.time.sec,   endPos.time.nanoSec);
        const bool haveTimes = (startTimeNs != 0 || endTimeNs != 0) && (endTimeNs > startTimeNs);
        const quint64 timeDiffNs = haveTimes ? (endTimeNs - startTimeNs) : zeroNsecs;

        const quint64 stepNs = haveTimes ? (timeDiffNs / numSteps) : oneHundredNsecs;

        int cnt = 1;
        for (int j = firstValidIndex + 1; j < secondValidIndex; ++j, ++cnt) {
            if (auto* ep = datasetPtr_->fromIndex(j)) {
                // quint64 currentNs = haveTimes ? (convertToNanosecs(ep->getPositionGNSS().time.sec, ep->getPositionGNSS().time.nanoSec)) : (startTimeNs + stepNs * cnt);
                // if (!haveTimes) {
                //     const auto t = convertFromNanosecs(currentNs);
                //     ep->setGNSSSec(t.first);
                //     ep->setGNSSNanoSec(t.second);
                // }
                quint64 currentNs = (startTimeNs + stepNs * cnt);
                const auto t = convertFromNanosecs(currentNs);
                ep->setGNSSSec(t.first);
                ep->setGNSSNanoSec(t.second);

                double progress = haveTimes ? double(currentNs - startTimeNs) / double(timeDiffNs) : double(cnt) / double(numSteps);
                if (!haveTimes || currentNs <= startTimeNs || currentNs >= endTimeNs) {
                    progress = double(cnt) / double(numSteps);
                }
                progress = qBound(0.0, progress, 1.0);

                const auto resNed = interpNED(startPos.ned, endPos.ned, float(progress));
                const auto resLla = interpLLA(startPos.lla, endPos.lla, float(progress));

                ep->setPositionNED(resNed);
                ep->setPositionLLA(resLla);
                ep->setPositionDataType(DataType::kInterpolated);
                emit datasetPtr_->positionAdded(j);
                lastPosInterpIndx_ = j;
                beenInterp = true;
            }
        }

        firstValidIndex = secondValidIndex;
    }

    if (beenInterp) {
        //emit datasetPtr_->interpPos(endEpochIndx);
    }
    else {
        lastPosInterpIndx_ = firstValidIndex - 1;
    }
}

void DataInterpolator::interpolateAtt(bool fromStart)
{
    if (!datasetPtr_) {
        return;
    }

    int startEpochIndx = fromStart ? 0 : lastAttInterpIndx_;
    int endEpochIndx = datasetPtr_->size();

    if ((endEpochIndx - startEpochIndx) < 1) {
        return;
    }

    int firstValidIndex = startEpochIndx;
    bool beenInterp = false;

    while (firstValidIndex < endEpochIndx) {
        while (firstValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(firstValidIndex); ep) {
                if (ep->getAttDataType() == DataType::kRaw) {
                    break;
                }
                ++firstValidIndex;
            }
        }

        int secondValidIndex = firstValidIndex + 1;
        while (secondValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(secondValidIndex); ep) {
                if (ep->getAttDataType() == DataType::kRaw) {
                    break;
                }
                ++secondValidIndex;
            }
        }

        if (secondValidIndex > endEpochIndx) {
            break;
        }

        int numInterpIndx = secondValidIndex - firstValidIndex;
        if (numInterpIndx < 1) {
            firstValidIndex = secondValidIndex;
            continue;
        }

        int fromIndx = firstValidIndex + 1;
        int toIndx = secondValidIndex;

        // boundaries epochs
        auto* startEpoch = datasetPtr_->fromIndex(firstValidIndex);
        auto* endEpoch = datasetPtr_->fromIndex(secondValidIndex);
        if (!startEpoch || !endEpoch) {
            continue;
        }

        auto startYaw = startEpoch->yaw();
        auto endYaw = endEpoch->yaw();
        auto startPitch = startEpoch->pitch();
        auto endPitch = endEpoch->pitch();
        auto startRoll = startEpoch->roll();
        auto endRoll = endEpoch->roll();


        const int numSteps = secondValidIndex - firstValidIndex;

        const quint64 startTimeNs = convertToNanosecs(startEpoch->getPositionGNSS().time.sec,
                                                      startEpoch->getPositionGNSS().time.nanoSec);
        const quint64 endTimeNs   = convertToNanosecs(endEpoch->getPositionGNSS().time.sec,
                                                    endEpoch->getPositionGNSS().time.nanoSec);
        const bool haveTimes = (startTimeNs != 0 || endTimeNs != 0) && (endTimeNs > startTimeNs);
        const quint64 timeDiffNs = haveTimes ? (endTimeNs - startTimeNs) : zeroNsecs;

        const quint64 stepNs = haveTimes ? (timeDiffNs / numSteps) : oneHundredNsecs;

        int cnt = 1;
        for (int j = fromIndx; j < toIndx; ++j, ++cnt) {
            if (auto* ep = datasetPtr_->fromIndex(j); ep) {

                quint64 currentNs = haveTimes ? convertToNanosecs(ep->getPositionGNSS().time.sec, ep->getPositionGNSS().time.nanoSec) : (startTimeNs + stepNs * cnt);

                if (!haveTimes) {
                    const auto t = convertFromNanosecs(currentNs);
                    ep->setGNSSSec(t.first);
                    ep->setGNSSNanoSec(t.second);
                }

                double progress = haveTimes
                                      ? double(currentNs - startTimeNs) / double(timeDiffNs)
                                      : double(cnt) / double(numSteps);
                if (!haveTimes || currentNs <= startTimeNs || currentNs >= endTimeNs) {
                    progress = double(cnt) / double(numSteps);
                }
                progress = qBound(0.0, progress, 1.0);

                float resYaw   = interpAttParam(startYaw,   endYaw,   float(progress));
                float resPitch = interpAttParam(startPitch, endPitch, float(progress));
                float resRoll  = interpAttParam(startRoll,  endRoll,  float(progress));

                ep->setAtt(resYaw, resPitch, resRoll, DataType::kInterpolated);
                beenInterp = true;
                lastAttInterpIndx_ = j;
            }
        }

        firstValidIndex = secondValidIndex;
    }

    if (beenInterp) {
    //    emit datasetPtr_->interpYaw(endEpochIndx);
    }
    else {
        lastAttInterpIndx_ = firstValidIndex - 1;
    }
}

void DataInterpolator::clear()
{
    lastAttInterpIndx_= 0;
    lastPosInterpIndx_ = 0;
}

float DataInterpolator::interpAttParam(float start, float end, float progress) const
{
    float delta = end - start;
    if (delta > 180.0f) {
        delta -= 360.0f;
    }
    else if (delta < -180.0f) {
        delta += 360.0f;
    }

    float interpolated = start + progress * delta;
    if (interpolated < 0.0f) {
        interpolated += 360.0f;
    }
    else if (interpolated >= 360.0f) {
        interpolated -= 360.0f;
    }

    return interpolated;
}

NED DataInterpolator::interpNED(const NED &start, const NED &end, float progress) const
{
    NED result;
    result.n = (1.0 - progress) * start.n + progress * end.n;
    result.e = (1.0 - progress) * start.e + progress * end.e;
    result.d = (1.0 - progress) * start.d + progress * end.d;
    return result;
}

LLA DataInterpolator::interpLLA(const LLA &start, const LLA &end, float progress) const
{
    LLA result;
    result.latitude  = (1.0 - progress) * start.latitude  + progress * end.latitude;
    result.longitude = (1.0 - progress) * start.longitude + progress * end.longitude;
    result.altitude  = (1.0 - progress) * start.altitude  + progress * end.altitude;
    return result;
}

float DataInterpolator::interpDist(float start, float end, float progress) const
{
    return (1.0 - progress) * start + progress * end;
}

qint64 DataInterpolator::calcTimeDiffInNanoSecs(time_t startSecs, int startNanoSecs, time_t endSecs, int endNanoSecs) const
{
    qint64 stTime = static_cast<qint64>(startSecs) * oneSecInNsecs + startNanoSecs;
    qint64 enTime = static_cast<qint64>(endSecs)   * oneSecInNsecs + endNanoSecs;

    return enTime - stTime;
}

qint64 DataInterpolator::convertToNanosecs(time_t secs, int nanoSecs) const
{
    return static_cast<qint64>(secs) * oneSecInNsecs + nanoSecs;
}

std::pair<time_t, int> DataInterpolator::convertFromNanosecs(qint64 totalNanoSecs) const
{
    time_t seconds  = static_cast<time_t>(totalNanoSecs / oneSecInNsecs);
    int nanoseconds = static_cast<int>(totalNanoSecs % oneSecInNsecs);

    return { seconds, nanoseconds };
}

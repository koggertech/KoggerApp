#include "data_interpolator.h"

#include "dataset.h"


DataInterpolator::DataInterpolator(Dataset *datasetPtr) :
    datasetPtr_(datasetPtr),
    lastYawInterpIndx_(0),
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
    //bool beenInterp = false;

    while (firstValidIndex < endEpochIndx) {
        while (firstValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(firstValidIndex); ep) {
                if (ep->getPositionGNSS().ned.isCoordinatesValid() &&
                    ep->getPositionGNSS().lla.isCoordinatesValid() ) {
                    break;
                }
                ++firstValidIndex;
            }
        }

        int secondValidIndex = firstValidIndex + 1;
        while (secondValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(secondValidIndex); ep) {
                if (ep->getPositionGNSS().ned.isCoordinatesValid() &&
                    ep->getPositionGNSS().lla.isCoordinatesValid()) {
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

        auto* startEpoch = datasetPtr_->fromIndex(firstValidIndex);
        auto* endEpoch = datasetPtr_->fromIndex(secondValidIndex);
        if (!startEpoch || !endEpoch) {
            continue;
        }

        auto startPos = startEpoch->getPositionGNSS();
        auto endPos = endEpoch->getPositionGNSS();

        auto timeDiffNano = calcTimeDiffInNanoSecs(startEpoch->getPositionGNSS().time.sec, startEpoch->getPositionGNSS().time.nanoSec, endEpoch->getPositionGNSS().time.sec,   endEpoch->getPositionGNSS().time.nanoSec);
        auto timeOnStep = static_cast<quint64>(timeDiffNano * 1.0f / static_cast<float>(numInterpIndx));

        int cnt = 1;
        auto startTime = convertToNanosecs(startEpoch->getPositionGNSS().time.sec, startEpoch->getPositionGNSS().time.nanoSec);

        for (int j = fromIndx; j < toIndx; ++j, ++cnt) {
            auto pTime = convertFromNanosecs(startTime + cnt * timeOnStep);
            if (auto* ep = datasetPtr_->fromIndex(j); ep) {
                ep->setGNSSSec(pTime.first);
                ep->setGNSSNanoSec(pTime.second);

                auto currentTime = convertToNanosecs(ep->getPositionGNSS().time.sec, ep->getPositionGNSS().time.nanoSec);
                float progress = (currentTime - startTime) * 1.0f / static_cast<float>(timeDiffNano);

                if (!ep->getInterpNED().isCoordinatesValid()) {
                    lastPosInterpIndx_ = j;
                    auto resNed = interpNED(startPos.ned, endPos.ned, progress);
                    auto resLla = interpLLA(startPos.lla, endPos.lla, progress);
                    //qDebug() << "      interp ned" << j << resNed.n << resNed.e << "lla" << resLla.latitude << resLla.longitude;
                    ep->setInterpNED(resNed);
                    ep->setInterpLLA(resLla);

                    //beenInterp = true;
                }
            }
        }

        firstValidIndex = secondValidIndex;
    }

    //if (beenInterp) {
    //    emit datasetPtr_->interpPos(endEpochIndx);
    //}
}

void DataInterpolator::interpolateYaw(bool fromStart)
{
    if (!datasetPtr_) {
        return;
    }

    int startEpochIndx = fromStart ? 0 : lastYawInterpIndx_;
    int endEpochIndx = datasetPtr_->size();

    if ((endEpochIndx - startEpochIndx) < 1) {
        return;
    }

    int firstValidIndex = startEpochIndx;
    //bool beenInterp = false;

    while (firstValidIndex < endEpochIndx) {
        while (firstValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(firstValidIndex); ep) {
                if (isfinite(ep->yaw())) {
                    break;
                }
                ++firstValidIndex;
            }
        }

        int secondValidIndex = firstValidIndex + 1;
        while (secondValidIndex < endEpochIndx) {
            if (auto* ep = datasetPtr_->fromIndex(secondValidIndex); ep) {
                if (isfinite(ep->yaw())) {
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
        auto timeDiffNano = calcTimeDiffInNanoSecs(startEpoch->getPositionGNSS().time.sec, startEpoch->getPositionGNSS().time.nanoSec, endEpoch->getPositionGNSS().time.sec,   endEpoch->getPositionGNSS().time.nanoSec);
        auto timeOnStep = static_cast<quint64>(timeDiffNano * 1.0f / static_cast<float>(numInterpIndx));

        // time
        int cnt = 1;
        auto startTime = convertToNanosecs(startEpoch->getPositionGNSS().time.sec, startEpoch->getPositionGNSS().time.nanoSec);

        for (int j = fromIndx; j < toIndx; ++j, ++cnt) {
            auto pTime = convertFromNanosecs(startTime + cnt * timeOnStep);
            if (auto* ep = datasetPtr_->fromIndex(j); ep) {
                ep->setGNSSSec(pTime.first);
                ep->setGNSSNanoSec(pTime.second);

                auto currentTime = convertToNanosecs(ep->getPositionGNSS().time.sec, ep->getPositionGNSS().time.nanoSec);
                float progress = (currentTime - startTime) * 1.0f / static_cast<float>(timeDiffNano);

                if (!std::isfinite(ep->getInterpYaw())) {
                    lastYawInterpIndx_ = j;
                    float resYaw = interpYaw(startYaw, endYaw, progress);
                    //qDebug() << "      interp yaw" << j << resYaw ;
                    ep->setInterpYaw(resYaw);
                    //beenInterp = true;
                }
            }
        }

        firstValidIndex = secondValidIndex;
    }

    //if (beenInterp) {
    //    emit datasetPtr_->interpYaw(endEpochIndx);
    //}
}

void DataInterpolator::clear()
{
    lastYawInterpIndx_= 0;
    lastPosInterpIndx_ = 0;

}

float DataInterpolator::interpYaw(float start, float end, float progress) const
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
    qint64 stTime = static_cast<qint64>(startSecs) * 1'000'000'000 + startNanoSecs;
    qint64 enTime = static_cast<qint64>(endSecs)   * 1'000'000'000 + endNanoSecs;

    return enTime - stTime;
}

qint64 DataInterpolator::convertToNanosecs(time_t secs, int nanoSecs) const
{
    return static_cast<qint64>(secs) * 1'000'000'000 + nanoSecs;
}

std::pair<time_t, int> DataInterpolator::convertFromNanosecs(qint64 totalNanoSecs) const
{
    time_t seconds  = static_cast<time_t>(totalNanoSecs / 1'000'000'000);
    int nanoseconds = static_cast<int>(totalNanoSecs % 1'000'000'000);

    return { seconds, nanoseconds };
}

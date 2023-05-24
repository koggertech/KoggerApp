#ifndef MAXPOINTSFILTER_H
#define MAXPOINTSFILTER_H

#include <cmath>

#include "abstractbottomtrackfilter.h"

class MaxPointsFilter : public AbstractBottomTrackFilter
{
public:
    MaxPointsFilter();

    void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;
    void setMaxPointsCount(int count);

private:

    int mMaxPointsCount = 1.0f;
};

#endif // MAXPOINTSFILTER_H

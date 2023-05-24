#ifndef NEARESTPOINTFILTER_H
#define NEARESTPOINTFILTER_H

#include <QPair>

#include <cmath>

#include <abstractbottomtrackfilter.h>
#include <Edge.h>



class NearestPointFilter : public AbstractBottomTrackFilter
{
public:
    NearestPointFilter();

    void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;
    void setRange(float range);

private:

    float mRange = 10.0f;
};

#endif // NEARESTPOINTFILTER_H

#ifndef NEARESTPOINTFILTER_H
#define NEARESTPOINTFILTER_H

#include <QPair>
#include <QVariantMap>

#include <cmath>

#include <abstractbottomtrackfilter.h>
#include <Edge.h>

class NearestPointFilter : public AbstractBottomTrackFilter
{
    Q_OBJECT

public:
    NearestPointFilter();

    Q_INVOKABLE AbstractBottomTrackFilter::FilterType type() const override;
    Q_INVOKABLE float distance() const;
public:

    void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;
    void setDistance(float distance);
signals:

    void distanceChanged(int dist);

private:
    float mDistance = 1.0f;
};

#endif // NEARESTPOINTFILTER_H

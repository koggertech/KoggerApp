#ifndef NEARESTPOINTFILTER_H
#define NEARESTPOINTFILTER_H

#include <QPair>
#include <QVariantMap>

#include <cmath>

#include <abstractentitydatafilter.h>
#include <Edge.h>

class NearestPointFilter : public AbstractEntityDataFilter
{
    Q_OBJECT
    Q_PROPERTY(float distance READ distance WRITE setDistance)

public:
    NearestPointFilter(QObject* parent = nullptr);
    NearestPointFilter(float distance, QObject* parent = nullptr);
    virtual ~NearestPointFilter();

    AbstractEntityDataFilter::FilterType type() const override;
    float distance() const;

public Q_SLOTS:
    void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;
    void setDistance(float distance);

private:
    float m_distance = 1.0f;
};

#endif // NEARESTPOINTFILTER_H

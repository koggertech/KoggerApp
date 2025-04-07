#ifndef MAXPOINTSFILTER_H
#define MAXPOINTSFILTER_H

#include <QVariantMap>

#include <cmath>

#include <abstract_entity_data_filter.h>

class MaxPointsFilter : public AbstractEntityDataFilter
{
    Q_OBJECT
    Q_PROPERTY(int maxPointsCount READ maxPointsCount WRITE setMaxPointsCount)

public:
    explicit MaxPointsFilter(QObject* parent = nullptr);
    explicit MaxPointsFilter(int maxPointsCount, QObject* parent = nullptr);
    virtual ~MaxPointsFilter();

    virtual AbstractEntityDataFilter::FilterType type() const override;
    virtual void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;
    int maxPointsCount() const;
    void setMaxPointsCount(int count);

private:
    int m_maxPointsCount = 10000.0f;
};

#endif // MAXPOINTSFILTER_H

#ifndef MAXPOINTSFILTER_H
#define MAXPOINTSFILTER_H

#include <QVariantMap>

#include <cmath>

#include <constants.h>
#include <abstractbottomtrackfilter.h>

class MaxPointsFilter : public AbstractBottomTrackFilter
{
    Q_OBJECT

public:

    explicit MaxPointsFilter(QObject* parent = nullptr);

    Q_INVOKABLE virtual AbstractBottomTrackFilter::FilterType type() const override;
    Q_INVOKABLE int maxPointsCount() const;

public:

    virtual void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) override;

public:
    void setMaxPointsCount(int count);

signals:

    void maxPointsCountChanged(int count);

private:

    int mMaxPointsCount = 10000.0f;
};

#endif // MAXPOINTSFILTER_H

#ifndef ABSTRACTBOTTOMTRACKFILTER_H
#define ABSTRACTBOTTOMTRACKFILTER_H

#include "qqml.h"
#include <QObject>
#include <QVector>
#include <QVector3D>

class AbstractBottomTrackFilter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:

    explicit AbstractBottomTrackFilter(QObject* parent = nullptr) : QObject(parent) {};

    enum FilterType {
        MaxPointsCount = 1,
        NearestPointDistance = 2
    };

    Q_ENUM(FilterType)

    Q_INVOKABLE virtual AbstractBottomTrackFilter::FilterType type() const = 0;

public:

    virtual void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) = 0;
};

#endif // ABSTRACTBOTTOMTRACKFILTER_H

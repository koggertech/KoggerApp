#ifndef ABSTRACTBOTTOMTRACKFILTER_H
#define ABSTRACTBOTTOMTRACKFILTER_H

#include <QVector>
#include <QVector3D>

class AbstractBottomTrackFilter
{

public:

    virtual void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) = 0;
};

#endif // ABSTRACTBOTTOMTRACKFILTER_H

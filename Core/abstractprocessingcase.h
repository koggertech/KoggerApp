#ifndef ABSTRACTPROCESSINGCASE_H
#define ABSTRACTPROCESSINGCASE_H

#include <QVector>
#include <QVector3D>

#include "vertexobject.h"
#include "Point3D.h"
#include "sceneparams.h"

class AbstractProcessingCase
{
public:
    virtual VertexObject process(const QVector <QVector3D>& bottomTrack, SceneParams params) = 0;
};

//! Хешер для типа данных точки (необходим для устранения повторяющихся
//! точек с помощью QSet при триангуляции)
template <typename T>
inline uint qHash(const Point3D <T> &p, uint seed = 0)
{
    Q_UNUSED(seed)

    uint h1 = qHash(p.x());
    uint h2 = qHash(p.y());
    uint h3 = qHash(p.z());
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

#endif // ABSTRACTPROCESSINGCASE_H

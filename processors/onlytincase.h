#ifndef ONLYTINCASE_H
#define ONLYTINCASE_H

#include <QDebug>

#include <set>

#include "abstractprocessingcase.h"
#include <point2d.h>
#include <bowyerwatsondelaunay.h>
#include <DelaunayTriangulation.h>

class OnlyTinCase : public AbstractProcessingCase
{
public:
    OnlyTinCase();

    VertexObject process(const QVector <QVector3D>& bottomTrack, SceneParams params) override;
};

#endif // ONLYTINCASE_H

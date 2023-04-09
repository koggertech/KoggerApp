#ifndef TINWITHBARYCENTRICCASE_H
#define TINWITHBARYCENTRICCASE_H

#include <QSet>

#include "abstractprocessingcase.h"
#include "DelaunayTriangulation.h"
#include "barycentricinterpolator.h"
#include "gridgenerator.h"

#include <QDebug>

class TinWithBarycentricCase : public AbstractProcessingCase
{
public:
    TinWithBarycentricCase();

    ~TinWithBarycentricCase();

    VertexObject process(const QVector <QVector3D>& bottomTrack, SceneParams params) override;
};

#endif // TINWITHBARYCENTRICCASE_H

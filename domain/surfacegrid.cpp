#include "surfacegrid.h"
#include <drawutils.h>

SurfaceGrid::SurfaceGrid(QObject *parent)
: SceneObject(parent)
{}

SurfaceGrid::~SurfaceGrid()
{}

bool SurfaceGrid::isTriangle() const
{
    return RENDER_IMPL()->primitiveType() == GL_TRIANGLES;
}

bool SurfaceGrid::isQuad() const
{
    bool retVal{ false };

#ifndef Q_OS_ANDROID
    retVal = RENDER_IMPL()->primitiveType() == GL_QUADS;
#endif

    return retVal;
}

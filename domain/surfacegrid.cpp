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
    return RENDER_IMPL()->primitiveType() == GL_QUADS;
}

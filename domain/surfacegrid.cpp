#include "surfacegrid.h"
#include <draw_utils.h>

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

#if !defined(Q_OS_ANDROID) && !defined(LINUX_ES)
    retVal = RENDER_IMPL()->primitiveType() == GL_QUADS;
#endif

    return retVal;
}

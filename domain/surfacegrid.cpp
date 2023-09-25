#include "surfacegrid.h"

SurfaceGrid::SurfaceGrid(QObject *parent)
: SceneGraphicsObject(parent)
{
    setPrimitiveType(GL_LINES);
}

bool SurfaceGrid::isTriangle() const
{
    return m_primitiveType == GL_TRIANGLES;
}

bool SurfaceGrid::isQuad() const
{
    return m_primitiveType == GL_QUADS;
}

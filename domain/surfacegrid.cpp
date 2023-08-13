#include "surfacegrid.h"

SurfaceGrid::SurfaceGrid(QObject *parent)
: DisplayedObject(parent)
{
    mPrimitiveType = GL_LINES;
}

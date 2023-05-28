#include "sceneboundary.h"

SceneBoundary::SceneBoundary()
{
    setPrimitiveType(GL_LINES);
}

void SceneBoundary::setBoundary(const Cube &boundary)
{
    mBoundary = boundary;
}


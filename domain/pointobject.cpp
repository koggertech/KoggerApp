#include "pointobject.h"

PointObject::PointObject(QObject *parent)
    : DisplayedObject(GL_POINTS, parent)
{}

float PointObject::x() const
{
    return mX;
}

float PointObject::y() const
{
    return mY;
}

float PointObject::z() const
{
    return mZ;
}

void PointObject::setX(float x)
{
    if(mX == x)
        return;

    mX = x;

    Q_EMIT coordChanged(mX, mY, mZ);
}

void PointObject::setY(float y)
{
    if(mY == y)
        return;

    mY = y;

    Q_EMIT coordChanged(mX, mY, mZ);
}

void PointObject::setZ(float z)
{
    if(mZ == z)
        return;

    mZ = z;

    Q_EMIT coordChanged(mX, mY, mZ);
}

void PointObject::setCoord(float x, float y, float z)
{
    if(mX == x && mY == y && mZ == z)
        return;

    mX = x;
    mY = y;
    mZ = z;

    Q_EMIT coordChanged(mX, mY, mZ);
}

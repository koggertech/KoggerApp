#ifndef ABSTRACTPICKER_H
#define ABSTRACTPICKER_H

#include <QVector>

#include "vertexobject.h"

class AbstractPicker
{
public:

    virtual VertexObject pick(const VertexObject& object) = 0;
    virtual QString pickingMethod() = 0;

protected:

    virtual VertexObject pickAsTriangle(const VertexObject& object) = 0;
    virtual VertexObject pickAsQuad(const VertexObject& object) = 0;

protected:

    QVector3D mLastIntersectionPoint;
};

#endif // ABSTRACTPICKER_H

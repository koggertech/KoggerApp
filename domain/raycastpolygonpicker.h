#ifndef RAYCASTPOLYGONPICKER_H
#define RAYCASTPOLYGONPICKER_H

#include <abstractpicker.h>
#include <constants.h>
#include "Triangle.h"
#include "Quad.h"

#include <QDebug>

class RayCastPolygonPicker : public AbstractPicker
{
public:
    RayCastPolygonPicker(const QVector3D& origin, const QVector3D& dir);

    virtual VertexObject pick(const VertexObject& object) override;
    QString pickingMethod() override;

protected:

    virtual VertexObject pickAsTriangle(const VertexObject& object) override;
    virtual VertexObject pickAsQuad(const VertexObject& object) override;

private:

    QVector3D mOrigin;
    QVector3D mDir;
};

#endif // RAYCASTPOLYGONPICKER_H

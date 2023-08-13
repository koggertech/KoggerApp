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

    virtual std::shared_ptr <VertexObject> pick(std::shared_ptr <VertexObject> object) override;
    QString pickingMethod() override;

protected:

    virtual std::shared_ptr <VertexObject> pickAsTriangle(std::shared_ptr <VertexObject> object);
    virtual std::shared_ptr <VertexObject> pickAsQuad(std::shared_ptr <VertexObject> object);

private:

    QVector3D mOrigin;
    QVector3D mDir;
};

#endif // RAYCASTPOLYGONPICKER_H

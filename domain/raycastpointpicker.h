#ifndef RAYCASTPOINTPICKER_H
#define RAYCASTPOINTPICKER_H

#include "raycastpolygonpicker.h"

class RayCastPointPicker : public RayCastPolygonPicker
{
public:
    RayCastPointPicker(const QVector3D& origin, const QVector3D& dir);

    VertexObject pick(const VertexObject& object) override;

    QString pickingMethod() override;
};

#endif // RAYCASTPOINTPICKER_H

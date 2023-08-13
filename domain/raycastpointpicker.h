#ifndef RAYCASTPOINTPICKER_H
#define RAYCASTPOINTPICKER_H

#include "raycastpolygonpicker.h"
#include "raycastlinepicker.h"

class RayCastPointPicker : public AbstractPicker
{
public:
    RayCastPointPicker(const QVector3D& origin, const QVector3D& dir);

    std::shared_ptr <VertexObject> pick(std::shared_ptr <VertexObject> sourceObject) override;

    QString pickingMethod() override;

private:

    std::shared_ptr <RayCastPolygonPicker> mpPolygonPicker;
    std::shared_ptr <RayCastLinePicker> mpLinePicker;

};

#endif // RAYCASTPOINTPICKER_H

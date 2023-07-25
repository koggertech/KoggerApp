#include "raycastpointpicker.h"

RayCastPointPicker::RayCastPointPicker(const QVector3D& origin, const QVector3D& dir)
{
    mpPolygonPicker = std::make_shared <RayCastPolygonPicker>(origin,dir);
    mpLinePicker = std::make_shared <RayCastLinePicker>(origin,dir);
}

VertexObject RayCastPointPicker::pick(const VertexObject& object)
{
    VertexObject pickedObject(GL_POINTS, {});
    QVector3D lastIntersectionPoint;

    if (object.primitiveType() == GL_LINES ||
        object.primitiveType() == GL_LINE_STRIP){
        pickedObject = mpLinePicker->pick(object);
        lastIntersectionPoint = mpLinePicker->lastIntersectionPoint();


        return {GL_POINTS, {lastIntersectionPoint}};
    }

    if (object.primitiveType() == GL_TRIANGLES ||
        object.primitiveType() == GL_QUADS)
    {
        pickedObject = mpPolygonPicker->pick(object);
        lastIntersectionPoint = mpPolygonPicker->lastIntersectionPoint();
    }

    if (pickedObject.cdata().isEmpty()){
        return {GL_POINTS, {}};
    }

    auto pickedPoint = pickedObject.cdata().first();
    auto distToLastPoint = pickedPoint.distanceToPoint(lastIntersectionPoint);

    for (const auto& point : pickedObject.cdata()){
        auto distToCurrentPoint = point.distanceToPoint(lastIntersectionPoint);

        if (distToCurrentPoint <= distToLastPoint){
            pickedPoint = point;
            distToLastPoint = distToCurrentPoint;
        }
    }

    return {GL_POINTS, {pickedPoint}};
}

QString RayCastPointPicker::pickingMethod()
{
    return PICKING_METHOD_POINT;
}

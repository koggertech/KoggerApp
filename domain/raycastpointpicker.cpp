#include "raycastpointpicker.h"

RayCastPointPicker::RayCastPointPicker(const QVector3D& origin, const QVector3D& dir)
: RayCastPolygonPicker(origin, dir)
{

}

VertexObject RayCastPointPicker::pick(const VertexObject& object)
{
    auto pickedObject = RayCastPolygonPicker::pick(object);

    if (pickedObject.cdata().isEmpty()){
        return {GL_POINTS, {}};
    }

    auto pickedPoint = pickedObject.cdata().first();
    auto distToLastPoint = pickedPoint.distanceToPoint(mLastIntersectionPoint);

    for (const auto& point : pickedObject.cdata()){
        auto distToCurrentPoint = point.distanceToPoint(mLastIntersectionPoint);

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

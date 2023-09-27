#include "raycastpointpicker.h"

RayCastPointPicker::RayCastPointPicker(const QVector3D& origin, const QVector3D& dir)
{
    mpPolygonPicker = std::make_shared <RayCastPolygonPicker>(origin,dir);
    mpLinePicker = std::make_shared <RayCastLinePicker>(origin,dir);
}

//std::shared_ptr <VertexObject> RayCastPointPicker::pick(std::shared_ptr <VertexObject> sourceObject)
//{
//    std::shared_ptr <VertexObject> pickedObject;
//    QVector3D lastIntersectionPoint;
//    QVector <QVector3D> result;
//
//    if (sourceObject->primitiveType() == GL_LINES ||
//        sourceObject->primitiveType() == GL_LINE_STRIP){
//        pickedObject = mpLinePicker->pick(sourceObject);
//        lastIntersectionPoint = mpLinePicker->lastIntersectionPoint();
//
//        result.append(lastIntersectionPoint);
//
//        return std::make_shared <VertexObject>(GL_POINTS, result);
//    }
//
//    if (sourceObject->primitiveType() == GL_TRIANGLES ||
//        sourceObject->primitiveType() == GL_QUADS)
//    {
//        pickedObject = mpPolygonPicker->pick(sourceObject);
//        lastIntersectionPoint = mpPolygonPicker->lastIntersectionPoint();
//    }
//
//    if (pickedObject->cdata().isEmpty()){
//        return std::make_shared <VertexObject>(GL_POINTS, QVector <QVector3D>());
//    }
//
//    auto pickedPoint = pickedObject->cdata().first();
//    auto distToLastPoint = pickedPoint.distanceToPoint(lastIntersectionPoint);
//
//    for (const auto& point : pickedObject->cdata()){
//        auto distToCurrentPoint = point.distanceToPoint(lastIntersectionPoint);
//
//        if (distToCurrentPoint <= distToLastPoint){
//            pickedPoint = point;
//            distToLastPoint = distToCurrentPoint;
//        }
//    }
//
//    result.append(pickedPoint);
//
//    return std::make_shared <VertexObject>(GL_POINTS, result);
//}

QString RayCastPointPicker::pickingMethod()
{
    return PICKING_METHOD_POINT;
}

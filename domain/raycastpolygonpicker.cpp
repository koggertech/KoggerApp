#include "raycastpolygonpicker.h"

RayCastPolygonPicker::RayCastPolygonPicker(const QVector3D& origin, const QVector3D& dir)
: mOrigin(origin)
, mDir(dir)
{}

std::shared_ptr <VertexObject> RayCastPolygonPicker::pick(std::shared_ptr <VertexObject> sourceObject)
{
    std::shared_ptr <VertexObject> result;

    if (sourceObject->primitiveType() == GL_TRIANGLES){
        result = pickAsTriangle(sourceObject);
    }else if (sourceObject->primitiveType() == GL_QUADS){
        result = pickAsQuad(sourceObject);
    }

    return result;
}

QString RayCastPolygonPicker::pickingMethod()
{
    return PICKING_METHOD_POLYGON;
}

std::shared_ptr <VertexObject> RayCastPolygonPicker::pickAsTriangle(std::shared_ptr <VertexObject> sourceObject)
{
    using CandidateTriangle = Triangle <float>;
    using IntersectionPoint = QVector3D;

    QVector <QPair <CandidateTriangle, IntersectionPoint>> candidates;
    QVector <QVector3D> result;

    auto size = sourceObject->cdata().size();

    for (int i = 0; i < size; i+=3){

        Triangle <float> triangle(
                                  sourceObject->cdata().at(i),
                                  sourceObject->cdata().at(i+1),
                                  sourceObject->cdata().at(i+2)
                                );

        QVector3D intersectionPoint;

        bool intersects = triangle.intersectsWithLine(mOrigin, mDir, intersectionPoint, true);

        if (intersects){
            candidates.append(
                              {triangle, intersectionPoint}
                             );
        }
    }

    if (candidates.isEmpty())
        return std::make_shared <VertexObject>(GL_TRIANGLES, result);

    // Choosing nearest triangle to origin point

    auto distToLastCandidate = candidates.first().first.distanceToPoint(mOrigin);

    for (auto& candidate : candidates){

        auto candidateTriangle = candidate.first;

        auto distToCurrentCandidate = candidateTriangle.distanceToPoint(mOrigin);

        if (distToCurrentCandidate <= distToLastCandidate){

            result = {
                                        candidateTriangle.A().toQVector3D(),
                                        candidateTriangle.B().toQVector3D(),
                                        candidateTriangle.C().toQVector3D()
                                     };

            mLastIntersectionPoint = candidate.second;

            distToLastCandidate = distToCurrentCandidate;
        }
    }

    return std::make_shared <VertexObject>(GL_TRIANGLES, result);
}

std::shared_ptr <VertexObject> RayCastPolygonPicker::pickAsQuad(std::shared_ptr <VertexObject> sourceObject)
{
    QVector <QPair <Quad <float>, QVector3D>> candidates;
    QVector <QVector3D> result;

    auto size = sourceObject->cdata().size();

    for (int i = 0; i < size; i+=4){

        Quad <float> quad(
                        sourceObject->cdata().at(i),
                        sourceObject->cdata().at(i+1),
                        sourceObject->cdata().at(i+2),
                        sourceObject->cdata().at(i+3)
                    );

        QVector3D intersectionPoint;

        bool intersects = quad.intersectsWithLine(mOrigin, mDir, intersectionPoint, true);

        if (intersects){
            candidates.append(
                             {quad, intersectionPoint}
                            );
        }
    }

    if (candidates.isEmpty())
        return std::make_shared <VertexObject>(GL_QUADS, result);

    // Choosing nearest to origin point triangle

    auto distToLastCandidate = candidates.first().first.distanceToPoint(mOrigin);

    for (const auto& candidate : candidates){

        auto candidateQuad = candidate.first;

        auto distToCurrentCandidate = candidateQuad.distanceToPoint(mOrigin);

        if (distToCurrentCandidate <= distToLastCandidate){

            result = {
                        candidateQuad.A().toQVector3D(),
                        candidateQuad.B().toQVector3D(),
                        candidateQuad.C().toQVector3D(),
                        candidateQuad.D().toQVector3D()
                    };

            mLastIntersectionPoint = candidate.second;

            distToLastCandidate = distToCurrentCandidate;
        }
    }

    return std::make_shared <VertexObject>(GL_QUADS, result);
}

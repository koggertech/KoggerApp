#include "raycastpolygonpicker.h"

RayCastPolygonPicker::RayCastPolygonPicker(const QVector3D& origin, const QVector3D& dir)
: mOrigin(origin)
, mDir(dir)
{}

VertexObject RayCastPolygonPicker::pick(const VertexObject& object)
{
    VertexObject result;

    if (object.primitiveType() == GL_TRIANGLES){
        result = pickAsTriangle(object);
    }else if (object.primitiveType() == GL_QUADS){
        result = pickAsQuad(object);
    }

    return result;
}

QString RayCastPolygonPicker::pickingMethod()
{
    return PICKING_METHOD_POLYGON;
}

VertexObject RayCastPolygonPicker::pickAsTriangle(const VertexObject& object)
{
    using CandidateTriangle = Triangle <float>;
    using IntersectionPoint = QVector3D;

    QVector <QPair <CandidateTriangle, IntersectionPoint>> candidates;

    auto size = object.cdata().size();

    for (int i = 0; i < size; i+=3){

        Triangle <float> triangle(
                                  object.cdata().at(i),
                                  object.cdata().at(i+1),
                                  object.cdata().at(i+2)
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
        return {GL_TRIANGLES, {}};

    // Choosing nearest triangle to origin point

    QVector <QVector3D> resultTriangleVertices;

    auto distToLastCandidate = candidates.first().first.distanceToPoint(mOrigin);

    for (auto& candidate : candidates){

        auto candidateTriangle = candidate.first;

        auto distToCurrentCandidate = candidateTriangle.distanceToPoint(mOrigin);

        if (distToCurrentCandidate <= distToLastCandidate){

            resultTriangleVertices = {
                                        candidateTriangle.A().toQVector3D(),
                                        candidateTriangle.B().toQVector3D(),
                                        candidateTriangle.C().toQVector3D()
                                     };

            mLastIntersectionPoint = candidate.second;

            distToLastCandidate = distToCurrentCandidate;
        }
    }

    return {GL_TRIANGLES,resultTriangleVertices};
}

VertexObject RayCastPolygonPicker::pickAsQuad(const VertexObject& object)
{
    QVector <QPair <Quad <float>, QVector3D>> candidates;

    auto size = object.cdata().size();

    for (int i = 0; i < size; i+=4){

        Quad <float> quad(
                        object.cdata().at(i),
                        object.cdata().at(i+1),
                        object.cdata().at(i+2),
                        object.cdata().at(i+3)
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
        return {GL_QUADS, {}};

    // Choosing nearest to origin point triangle

    QVector <QVector3D> resultQuadVertices;

    auto distToLastCandidate = candidates.first().first.distanceToPoint(mOrigin);

    for (const auto& candidate : candidates){

        auto candidateQuad = candidate.first;

        auto distToCurrentCandidate = candidateQuad.distanceToPoint(mOrigin);

        if (distToCurrentCandidate <= distToLastCandidate){

            resultQuadVertices = {
                                    candidateQuad.A().toQVector3D(),
                                    candidateQuad.B().toQVector3D(),
                                    candidateQuad.C().toQVector3D(),
                                    candidateQuad.D().toQVector3D()
                                };

            mLastIntersectionPoint = candidate.second;

            distToLastCandidate = distToCurrentCandidate;
        }
    }

    return {GL_QUADS, resultQuadVertices};
}

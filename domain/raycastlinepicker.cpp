#include "raycastlinepicker.h"

#include <Edge.h>

RayCastLinePicker::RayCastLinePicker(const QVector3D& origin, const QVector3D& dir)
: mOrigin(origin)
, mDir(dir)
{
}

std::shared_ptr <VertexObject> RayCastLinePicker::pick(std::shared_ptr <VertexObject> sourceObject)
{
    using CandidateLine = Edge <float>;
    using IntersectionPoint = QVector3D;

    QVector <QPair <CandidateLine, IntersectionPoint>> candidates;
    QVector <QVector3D> result;

    for (int i = 0; i < sourceObject->cdata().size()-1; i+=2){

        auto line = CandidateLine(sourceObject->cdata().at(i),
                                  sourceObject->cdata().at(i+1));

        IntersectionPoint intersectionPoint;

        if (line.intersectsWithLine(mOrigin,
                                    mDir,
                                    intersectionPoint,
                                    false))
        {
            candidates.append(
                            {line,intersectionPoint}
                        );
        }

       // qDebug() << "Inters: " << intersectionPoint;
    }

    if (candidates.isEmpty())
        return std::make_shared <VertexObject>(GL_LINES, result);

    auto distToLastCandidate = mOrigin.distanceToPoint(candidates.first().second);

    for (const auto& candidate : candidates){
        auto distToCurrentCandidate = mOrigin.distanceToPoint(candidate.second);

        if (distToCurrentCandidate <= distToLastCandidate){
            result = {
                        candidate.first.p1().toQVector3D(),
                        candidate.first.p2().toQVector3D()
                    };

            mLastIntersectionPoint = candidate.second;
        }
    }

    return std::make_shared <VertexObject>(GL_LINES, result);
}

QString RayCastLinePicker::pickingMethod()
{
    return "LINE";
}

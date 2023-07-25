#include "raycastlinepicker.h"

#include <Edge.h>

RayCastLinePicker::RayCastLinePicker(const QVector3D& origin, const QVector3D& dir)
: mOrigin(origin)
, mDir(dir)
{
}

VertexObject RayCastLinePicker::pick(const VertexObject &object)
{
    using CandidateLine = Edge <float>;
    using IntersectionPoint = QVector3D;

    QVector <QPair <CandidateLine, IntersectionPoint>> candidates;

    for (int i = 0; i < object.cdata().size()-1; i+=2){

        auto line = CandidateLine(object.cdata().at(i),
                                  object.cdata().at(i+1));

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

        qDebug() << "Inters: " << intersectionPoint;
    }

    if (candidates.isEmpty())
        return {GL_LINES, {}};

    qDebug() << "Candidates count: " << candidates.size();

    QVector <QVector3D> result;

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

    return {GL_LINES, result};
    //return {GL_LINES, {}};
}

QString RayCastLinePicker::pickingMethod()
{
    return "LINE";
}

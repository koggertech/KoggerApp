#include "nearestpointfilter.h"

#include <constants.h>

NearestPointFilter::NearestPointFilter()
{

}

AbstractBottomTrackFilter::FilterType NearestPointFilter::type() const
{
    return FilterType::NearestPointDistance;
}

float NearestPointFilter::distance() const
{
    return mDistance;
}

void NearestPointFilter::setDistance(float distance)
{
    if (mDistance == distance)
        return;

    mDistance = distance;

    Q_EMIT distanceChanged(mDistance);
}

void NearestPointFilter::apply(const QVector<QVector3D> &origin, QVector<QVector3D> &filtered)
{
    if (mDistance <= 0)
        return;

    float trackLength = 0.0f;

    using GlobalLength = float;
    using Section = QPair <QPair<QVector3D, GlobalLength>, QPair<QVector3D, GlobalLength>>;

    QVector <Section> sections;

    for (int i = 0; i < origin.size()-1; i++){
        float segmentLength = sqrt(pow(origin[i+1].x() - origin[i].x(), 2) +
                              pow(origin[i+1].y() - origin[i].y(), 2) * 1.0);

        Section section{
            {origin[i], trackLength},
            {origin[i+1], trackLength + segmentLength},
        };

        sections.append(section);

        trackLength += segmentLength;
    }

    float currentLength = 0.0f;

    while(currentLength <= trackLength){
        for (const auto& section : sections){
            if (currentLength >= section.first.second &&
                currentLength <= section.second.second){
                float distToFirstPoint = abs(currentLength - section.first.second);
                float distToSecondPoint = abs(currentLength - section.second.second);

                QVector3D point;

                if (distToFirstPoint <= distToSecondPoint){
                    point = section.first.first;
                }else{
                    point = section.second.first;
                }

                if (!filtered.empty()){
                    if (filtered.last() != point){
                        filtered.append(point);
                    }
                }else{
                    filtered.append(point);
                }
            }
        }

        currentLength += mDistance;
    }
}

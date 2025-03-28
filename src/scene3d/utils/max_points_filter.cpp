#include "max_points_filter.h"
#include <QDebug>

MaxPointsFilter::MaxPointsFilter(QObject *parent)
    : AbstractEntityDataFilter(parent)
{}

MaxPointsFilter::MaxPointsFilter(int maxPointsCount, QObject *parent)
: AbstractEntityDataFilter(parent)
, m_maxPointsCount(maxPointsCount)
{}

MaxPointsFilter::~MaxPointsFilter()
{}

AbstractEntityDataFilter::FilterType MaxPointsFilter::type() const
{
    return FilterType::MaxPointsCount;
}

int MaxPointsFilter::maxPointsCount() const
{
    return m_maxPointsCount;
}

void MaxPointsFilter::setMaxPointsCount(int count)
{
    if (m_maxPointsCount == count)
        return;

    m_maxPointsCount = count;
}

void MaxPointsFilter::apply(const QVector<QVector3D> &origin, QVector<QVector3D> &filtered)
{
    if (m_maxPointsCount <= 1)
        return;

    if (m_maxPointsCount > origin.size())
        m_maxPointsCount = origin.size();

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
    float range = trackLength / static_cast <float>(m_maxPointsCount);

    while(currentLength < trackLength){
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

        currentLength += range;
    }
}

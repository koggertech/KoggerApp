#include "bottomtrackprovider.h"

#include <QMatrix4x4>

void BottomTrackProvider::setBottomTrack(QVector<QVector3D> &bottomTrack)
{
    //m_bottomTrack = bottomTrack;
    m_bottomTrack.clear();

    for(auto& v : bottomTrack)
        m_bottomTrack.append(QVector3D(v.x(), v.z(), -v.y()));

    Q_EMIT bottomTrackChanged(m_bottomTrack);
}

QVector<QVector3D> BottomTrackProvider::getBottomTrack() const
{
    return m_bottomTrack;
}

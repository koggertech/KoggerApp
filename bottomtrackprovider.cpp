#include "bottomtrackprovider.h"

void BottomTrackProvider::setBottomTrack(QVector<QVector3D> &bottomTrack)
{
    m_bottomTrack = bottomTrack;

    Q_EMIT bottomTrackChanged(m_bottomTrack);
}

QVector<QVector3D> BottomTrackProvider::getBottomTrack() const
{
    return m_bottomTrack;
}

#include "bottomtrackprovider.h"

void BottomTrackProvider::setBottomTrack(QVector<QVector3D> &bottomTrack)
{
    mBottomTrack = bottomTrack;

    Q_EMIT bottomTrackChanged();
}

QVector<QVector3D> BottomTrackProvider::getBottomTrack() const
{
    return mBottomTrack;
}

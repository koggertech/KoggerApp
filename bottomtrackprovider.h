#ifndef BOTTOMTRACKPROVIDER_H
#define BOTTOMTRACKPROVIDER_H

#include <QObject>
#include <QVector>
#include <QVector3D>

class BottomTrackProvider : public QObject
{
    Q_OBJECT
public:

    void setBottomTrack(QVector <QVector3D>& bottomTrack);
    QVector <QVector3D> getBottomTrack() const;

private:

    QVector <QVector3D> mBottomTrack;

signals:

    void bottomTrackChanged();

};

#endif // BOTTOMTRACKPROVIDER_H

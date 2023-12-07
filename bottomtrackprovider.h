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

signals:
    void bottomTrackChanged(QVector <QVector3D>& bottomTrack);

private:
    QVector <QVector3D> m_bottomTrack;

};

#endif // BOTTOMTRACKPROVIDER_H

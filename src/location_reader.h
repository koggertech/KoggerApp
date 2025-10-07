// location_reader.h
#pragma once
#include <QObject>
#include <QGeoPositionInfo>
#include <QGeoCoordinate>
#include <QTimer>

#include <QVector>


class LocationReader : public QObject
{
    Q_OBJECT
public:
    explicit LocationReader(QObject* parent = nullptr);
    ~LocationReader();

signals:
    void positionUpdated(const QGeoPositionInfo& info);
    void gpsAlive(bool ok);


private slots:
    void onPositionUpdated(const QGeoPositionInfo& info);
    void onGpsTimeout();

private slots:
    void onPositionUpdatedSec();

private:
    QVector<QGeoPositionInfo> buildYerevanLakeDemoTrack() const;

private:
    QTimer                     timer_;
    QVector<QGeoPositionInfo>  demoTrack_;
    int                        demoIndex_ = 0;

    QTimer gpsWatchdog_;
    bool gpsAlive_ = false;
};

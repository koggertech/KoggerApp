#include "location_reader.h"

#include <QGeoCoordinate>
#include <QDateTime>
#include <QtMath>
#include <QDebug>
#include "core.h"

extern Core core;


LocationReader::LocationReader(QObject *parent)
    : QObject(parent)
{
    return; //

    //qDebug() << "LocationReader ctr";

#ifdef Q_OS_WINDOWS
    //TEST TRACK
    demoTrack_ = buildYerevanLakeDemoTrack();
    demoIndex_ = 0;

    timer_.setParent(this);
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &LocationReader::onPositionUpdatedSec);
    timer_.start();
#else
    QGeoPositionInfoSource* src = QGeoPositionInfoSource::createDefaultSource(this);
    if (src) {
        connect(src, &QGeoPositionInfoSource::positionUpdated,
                this, &LocationReader::onPositionUpdated);
        src->setUpdateInterval(1000);
        src->startUpdates();
    } else {
        qWarning() << "No position source available!";
    }
#endif


    gpsWatchdog_.setParent(this);
    gpsWatchdog_.setSingleShot(true);
    gpsWatchdog_.setTimerType(Qt::PreciseTimer);
    gpsWatchdog_.setInterval(3000);
    connect(&gpsWatchdog_, &QTimer::timeout, this, &LocationReader::onGpsTimeout);
    gpsWatchdog_.start();
}

LocationReader::~LocationReader()
{
    //qDebug() << "LocationReader dtr";

#ifdef Q_OS_WINDOWS
    if (timer_.isActive()) {
        timer_.stop();
    }
#endif

    if (gpsWatchdog_.isActive()) {
        gpsWatchdog_.stop();
    }
}

void LocationReader::onPositionUpdated(const QGeoPositionInfo &info)
{
    if (!gpsAlive_) {
        gpsAlive_ = true;
        emit gpsAlive(true);
    }
    gpsWatchdog_.start();

    QGeoCoordinate c = info.coordinate();
    //QString str =   "Lat: " + QString::number(c.latitude(), 'f', 4)  + " "
    //              + "Lon: " + QString::number(c.longitude(), 'f', 4) + " "
    //              + "Alt: " + QString::number(c.altitude(), 'f', 4)  + " "
    //              + "Yaw: " + QString::number(info.attribute(QGeoPositionInfo::Attribute::Direction), 'f', 4);

    //core.consoleInfo(str);

    emit positionUpdated(info);
}

void LocationReader::onGpsTimeout()
{
    if (gpsAlive_) {
        gpsAlive_ = false;
        emit gpsAlive(false);
    }
}

void LocationReader::onPositionUpdatedSec()
{
    if (!gpsAlive_) {
        gpsAlive_ = true;
        emit gpsAlive(true);
    }
    gpsWatchdog_.start();

    if (demoTrack_.isEmpty()) {
        return;
    }

    QGeoPositionInfo info = demoTrack_.at(demoIndex_); // get next point
    info.setTimestamp(QDateTime::currentDateTime());

    const QGeoCoordinate& c = info.coordinate();
    //QString str = QStringLiteral("Lat: %1 Lon: %2 Alt: %3  Head: %4  Time: %5")
    //                 .arg(c.latitude(),  0, 'f', 6)
    //                 .arg(c.longitude(), 0, 'f', 6)
    //                 .arg(c.altitude(),  0, 'f', 1)
    //                 .arg(info.attribute(QGeoPositionInfo::Direction), 0, 'f', 1)
    //                 .arg(info.timestamp().toString(Qt::ISODate));
    //core.consoleInfo(str);

    emit positionUpdated(info);

    demoIndex_ = (demoIndex_ + 1) % demoTrack_.size(); // by circle
}

QVector<QGeoPositionInfo> LocationReader::buildYerevanLakeDemoTrack() const
{
    QVector<QGeoPositionInfo> track;
    track.reserve(50);

    const double centerLat = 40.15982;
    const double centerLon = 44.47790;
    const double radiusM   = 300.0;

    const double metersPerDegLat = 111111.0;
    const double metersPerDegLon = metersPerDegLat * qCos(qDegreesToRadians(centerLat));
    const double dLat = radiusM / metersPerDegLat;
    const double dLon = radiusM / metersPerDegLon;

    QVector<QGeoCoordinate> coords;
    coords.reserve(50);
    for (int i = 0; i < 50; ++i) {
        const double deg = (360.0 * i) / 50.0;
        const double theta = qDegreesToRadians(deg);
        const double lat = centerLat + dLat * qSin(theta);
        const double lon = centerLon + dLon * qCos(theta);
        coords.push_back(QGeoCoordinate(lat, lon, 909.0));
    }

    for (int i = 0; i < 50; ++i) {
        const QGeoCoordinate& cur = coords[i];
        const QGeoCoordinate& nxt = coords[(i + 1) % 50];
        const double az = cur.azimuthTo(nxt);
        QGeoPositionInfo pi;
        pi.setCoordinate(cur);
        pi.setAttribute(QGeoPositionInfo::Direction, az);
        track.push_back(pi);
    }

    return track;
}


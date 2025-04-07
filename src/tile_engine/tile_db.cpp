#include "tile_db.h"

#include <QStandardPaths>
#include <QDir>
#include <QVariant>
#include <QBuffer>
#include <QDebug>
#include <QThread>


namespace map {

TileDB::TileDB(std::weak_ptr<TileProvider> tileProvider) :
    QObject(nullptr),
    tileProvider_(tileProvider),
    stopRequested_(false)
{
    qRegisterMetaType<TileIndex>("TileIndex");
    qRegisterMetaType<TileInfo>("TileInfo");
}

TileDB::~TileDB()
{
    db_.close();
}

void TileDB::loadTiles(const QSet<TileIndex> &tileIndices)
{
    stopRequested_ = false;

    for (const auto& itm : tileIndices) {
        if (!pendingLoadRequests_.contains(itm)) {
            pendingLoadRequests_.insert(itm);
        }
    }

    processNextTile();
}

void TileDB::saveTile(const TileIndex &tileIndx, const QImage &image)
{
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG");

    QSqlQuery query(db_);
    query.prepare("REPLACE INTO tiles (x, y, z, image) VALUES (:x, :y, :z, :image)");
    query.bindValue(":x", tileIndx.x_);
    query.bindValue(":y", tileIndx.y_);
    query.bindValue(":z", tileIndx.z_);
    query.bindValue(":image", imageData);

    if (!query.exec()) {
        qWarning() << "Failed to save the tile to the database:" << query.lastError().text();
    }

    emit tileSaved(tileIndx);
}

void TileDB::stopLoading(const TileIndex& tileIndex)
{
    pendingLoadRequests_.remove(tileIndex);

    emit tileLoadStopped(tileIndex);
}

void TileDB::stopAndClearRequests()
{
    stopRequested_ = true;

    for (auto it = pendingLoadRequests_.begin(); it != pendingLoadRequests_.end(); ++it) {
        emit tileLoadStopped(*it);
    }

    pendingLoadRequests_.clear();
}

void TileDB::init()
{
    QString dbName;

    if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
        switch (sharedProvider->getProviderId()) {
        case 1: {
            dbName = "tiles_google";
            break;
        }
        default: {
            dbName = "tiles_undefined";
            break;
        }
        }
    }

    if (dbName.isEmpty()) {
        qWarning() << "TileDB::init: Failed to init, tileProvider is 'nullptr'";
        return;
    }

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + dbName + ".db";
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    db_ = QSqlDatabase::addDatabase("QSQLITE", "TileDBConnection");
    db_.setDatabaseName(dbPath);

    if (!db_.open()) {
        qWarning() << "Failed to open the database:" << db_.lastError().text();
    }
    else {
        QSqlQuery query(db_);
        if (!query.exec("CREATE TABLE IF NOT EXISTS tiles ("
                        "x INTEGER, "
                        "y INTEGER, "
                        "z INTEGER, "
                        "image BLOB, "
                        "PRIMARY KEY (x, y, z))")) {
            qWarning() << "Failed to create table tiles:" << query.lastError().text();
        }
    }
}

void TileDB::processNextTile()
{
    if (stopRequested_) {
        return;
    }

    if (pendingLoadRequests_.isEmpty()) {
        return;
    }

    auto it = pendingLoadRequests_.begin();
    TileIndex index = *it;
    pendingLoadRequests_.erase(it);

    QSqlQuery query(db_);
    query.prepare("SELECT image FROM tiles WHERE x = :x AND y = :y AND z = :z");
    query.bindValue(":x", index.x_);
    query.bindValue(":y", index.y_);
    query.bindValue(":z", index.z_);

    if (!query.exec()) {
        emit tileLoadFailed(index, query.lastError().text());
    }
    else if (query.next()) {
        QByteArray imageData = query.value(0).toByteArray();
        QImage image;
        if (image.loadFromData(imageData)) {
            if (!stopRequested_) {
                emit tileLoaded(index, image);
            }
        }
        else {
            emit tileLoadFailed(index, "Failed to load image from database data");
        }
    }
    else {
        emit tileLoadFailed(index, "Tile not found in the database");
    }

    if (!stopRequested_) {
        QMetaObject::invokeMethod(this, "processNextTile", Qt::QueuedConnection);
    }
}


} // namespace map

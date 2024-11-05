#include "tile_manager.h"

#include <QDebug>
#include <QUrl>

#include "map_defs.h"
#include "tile_google_provider.h"


namespace map {


TileManager::TileManager(QObject *parent) :
    QObject(parent),
    tileSet_(std::make_unique<TileSet>()),
    tileProvider_(std::make_shared<TileGoogleProvider>()),
    tileDownloader_(std::make_unique<TileDownloader>(tileProvider_,  10, this)),
    tileDB_(std::make_unique<TileDB>())
{
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded, this,
                     [&](const TileIndex& tileIndx, const QImage& image, const TileInfo& info){
                        Q_UNUSED(image);
                        Q_UNUSED(info);

                        qDebug() << "Tile downloaded from:" << tileProvider_->createURL(tileIndx) << "bytes" <<image.sizeInBytes();

                        // save image
                     }, Qt::DirectConnection);

    // process error
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed, this,
                     [&](const TileIndex& tileIndx, const QString& errorString){
                         qWarning() << "Failed to download tile from:" << tileProvider_->createURL(tileIndx)
                         << "Error:" << errorString;
                     }, Qt::DirectConnection);
}

TileManager::~TileManager()
{

}

void TileManager::getRectRequest(QVector<QVector3D> rect)
{
    QList<TileIndex> resps;

    for (auto& itm : rect) {
        // NEDRect -> LLARect
        LLA lla(itm.x(), itm.y(), 0.0f);

        // LLARect -> tileIndx
        auto tileIndx = tileProvider_.get()->llaToTileIndex(lla, tileProvider_.get()->heightToTileZ(itm.z()));

        resps.append(tileIndx);

        //
        tileSet_->addTile(tileIndx);

        //auto tlla = tileProvider_.get()->indexToTileInfo(tileIndx);
        //qDebug() << "north:" << tlla.bounds.north << "south:" << tlla.bounds.south << "west:"<<tlla.bounds.west <<"east:" << tlla.bounds.east << "tileSizeMeters:" <<tlla.tileSizeMeters;
    }

    tileDownloader_.get()->downloadTiles(resps);
}


} // namespace map

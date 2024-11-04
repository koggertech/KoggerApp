#include "tile_manager.h"

#include <QDebug>


namespace map {


TileManager::TileManager(QObject *parent) :
    QObject(parent),
    tileGoogleProvider_(std::make_unique<TileGoogleProvider>())
{

}

TileManager::~TileManager()
{

}

void TileManager::getRectRequest(QVector<QVector3D> rect)
{
    //qDebug() << "TileManager::getRectRequest:" << rect;

    for (auto& itm : rect) {
        LLA lla(itm.x(), itm.y(), 0.0f);


        auto zoomLevel = tileGoogleProvider_.get()->heightToTileZ(itm.z());

        auto tileIndx = tileGoogleProvider_.get()->llaToTileIndex(lla, zoomLevel);

        //qDebug().noquote() << tileGoogleProvider_->createURL(tileIndx.x_, tileIndx.y_, tileIndx.z_);

        auto it = tiles_.find(tileIndx);

        if (it == tiles_.end()) {
            Tile newTile;
            //newTile.setIndex(tileIndx);

            auto emplaceResult = tiles_.emplace(tileIndx, std::move(newTile));
            if (!emplaceResult.second) {
                continue;
            }

            //qDebug() << "Added new tile:" << tileIndx;

            // download

        } else {
            //qDebug() << "Tile already exists:" << tileIndx;
        }
    }

    qDebug() << tiles_.size();
}


} // namespace map

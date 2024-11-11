#pragma once

#include <QObject>
#include <QVector>

#include "map_defs.h"
#include "plotcash.h"

#include "tile_provider.h"


namespace map {

class TileDB : public QObject
{
    Q_OBJECT

public:
    TileDB(std::weak_ptr<TileProvider> tileProvider);

    void stopAndClearRequests();

public slots:


signals:
    void tileLoaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);

protected:

private:
    std::weak_ptr<TileProvider> tileProvider_;

};


} // namespace map


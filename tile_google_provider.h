#pragma once

#include "tile_provider.h"
#include "map_defs.h"


namespace map {


constexpr double GOOGLE_TILE_CONSTANT = 156543000.03392;
const int GOOGLE_PROVIDER_ID = 1;
const int googleSat = 988;
const QString secGoogleWord = QStringLiteral("Galileo");
const QString language = QStringLiteral("en-US");
const QString server = QStringLiteral("khm");
const QString request = QStringLiteral("kh");


class TileGoogleProvider : public TileProvider
{
public:
    TileGoogleProvider();

    int32_t heightToTileZ(float height) const override final;
    int32_t lonToTileX(double lon, int z) const override final;
    int32_t latToTileY(double lat, int z) const override final;
    TileInfo indexToTileInfo(TileIndex tileIndx) const override final;
    QString createURL(const TileIndex& tileIndx) const override final;

private:
    int generateNum(int x, int y) const;
    void generateWords(const int x, const int y, QString& sec1, QString& sec2) const;
};


} // namespace map


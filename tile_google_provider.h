#pragma once

#include "tile_provider.h"


namespace map {


constexpr double GOOGLE_TILE_CONSTANT = 156543000.03392;
const int GOOGLE_PROVIDER_ID = 1;
const int googleSat = 989;
const QString secGoogleWord = QStringLiteral("Galileo");
const QString language = QStringLiteral("en-US");
const QString server = QStringLiteral("khm");
const QString request = QStringLiteral("kh");


class TileGoogleProvider : public TileProvider
{
public:
    TileGoogleProvider();

    int32_t heightToTileZ(const float height) override final;
    int32_t lonToTileX(const double lon, const int z) override final;
    int32_t latToTileY(const double lat, const int z) override final;

    int generateNum(int x, int y) const;
    void generateWords(const int x, const int y, QString& sec1, QString& sec2) const;
    QString createURL(const int x, const int y, const int zoom) const;

private:

};


} // namespace map


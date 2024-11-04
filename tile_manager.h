#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>


#include <memory>


#include "map_defs.h"
#include "tile_google_provider.h"



namespace map {


class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject *parent = nullptr);
    virtual ~TileManager();

public slots:
    void getRectRequest(QVector<QVector3D> rect);

private:
    std::unique_ptr<TileGoogleProvider> tileGoogleProvider_;
    std::unordered_map<TileIndex, Tile> tiles_; // Использование std::unordered_map с TileIndex как ключ

};



} // namespace map


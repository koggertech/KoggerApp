#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include <memory>

#include "tile_set.h"
#include "tile_provider.h"
#include "tile_downloader.h"
#include "tile_db.h"
#include "dataset.h"


namespace map {


class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject* parent = nullptr);
    ~TileManager();

    std::shared_ptr<TileSet> getTileSetPtr() const;

public slots:
    void getRectRequest(QVector<LLA> request, bool isPerspective, LLARef viewLlaRef, bool moveUp, CameraTilt tiltCam);
    void getLlaRef(LLARef viewLlaRef);

private:
    std::shared_ptr<TileProvider> tileProvider_;
    std::shared_ptr<TileDownloader> tileDownloader_;
    std::shared_ptr<TileDB> tileDB_;
    std::shared_ptr<TileSet> tileSet_;
    int lastZoomLevel_;

    static constexpr int maxTilesCapacity_{ 800 };
    static constexpr int minTilesCapacity_{ 400 };
    static constexpr int maxConcurrentDownloads_{ 10 };
};


} // namespace map


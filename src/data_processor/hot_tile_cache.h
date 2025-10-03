#pragma once

#include <list>
#include <QHash>
#include <QSet>
#include "data_processor_defs.h"
#include "surface_tile.h"


class DataProcessor;
class HotTileCache {
public:
    HotTileCache(size_t maxCapacity, size_t minCapacity);

    void clear();

    void setDataProcessorPtr(DataProcessor* ptr);;
    void setCapacity(size_t maxCap, size_t minCap = 0);
    void putBatch(TileMap&& tiles, DataSource source, bool useTextures);
    TileMap getForKeys(const QSet<TileKey>& keys, QSet<TileKey>* missing);
    bool contains(const TileKey& k) const;
    bool checkAnyTileForZoom(int targetZoom) const;
    size_t size() const;

    void onSendSavedTiles(const QVector<TileKey>& savedKeys);

private:
    struct Node {
        TileKey     key;
        SurfaceTile tile;
        DataSource  source      = DataSource::kUndefined;
        bool        hasTextures = false;
        bool        blocked     = false;
    };
    using ListIt = std::list<Node>::iterator;

    void touch(ListIt it);
    void upsertMove(TileKey key, SurfaceTile&& val, DataSource source, bool useTextures);
    void evictIfNeeded();

private:
    DataProcessor* dataProcPtr_{ nullptr };
    size_t maxCapacity_;
    size_t minCapacity_;
    std::list<Node> nodes_;
    QHash<TileKey, ListIt> index_; // fast access
};

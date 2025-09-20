#pragma once

#include <list>
#include <QHash>
#include <QSet>
#include "surface_tile.h"


class HotTileCache {
public:
    explicit HotTileCache(size_t maxCapacity = 2048, size_t minCapacity = 1536);

    void clear();
    void setCapacity(size_t maxCap, size_t minCap = 0);
    void putBatch(const TileMap& tiles, bool useTextures);
    void putBatch(TileMap&& tiles, bool useTextures);
    TileMap getForKeys(const QSet<TileKey>& keys, QSet<TileKey>* missing);
    bool contains(const TileKey& k) const;
    size_t size() const;

private:
    struct Node {
        TileKey     key;
        SurfaceTile tile;
        bool        hasTextures = false;
    };
    using ListIt = std::list<Node>::iterator;

    void touch(ListIt it);
    void upsertCopy(const TileKey& key, const SurfaceTile& val, bool useTextures);
    void upsertMove(TileKey key, SurfaceTile&& val, bool useTextures);
    void evictIfNeeded();

private:
    size_t maxCapacity_;
    size_t minCapacity_;
    std::list<Node> nodes_;
    QHash<TileKey, ListIt> index_; // fast access
};

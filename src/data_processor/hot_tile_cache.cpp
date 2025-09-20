#include "hot_tile_cache.h"


HotTileCache::HotTileCache(size_t maxCapacity, size_t minCapacity)
    : maxCapacity_(maxCapacity),
    minCapacity_(minCapacity ? minCapacity : (maxCapacity ? maxCapacity / 2 : 0))
{
    index_.reserve(int(maxCapacity_ * 2));
}

void HotTileCache::clear()
{
    nodes_.clear();
    index_.clear();
}

void HotTileCache::setCapacity(size_t maxCap, size_t minCap)
{
    maxCapacity_ = maxCap;
    minCapacity_ = (minCap ? minCap : (maxCap ? maxCap / 2 : 0));
    evictIfNeeded();
}

void HotTileCache::putBatch(const TileMap& tiles, bool useTextures)
{
    for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
        upsertCopy(it.key(), it.value(), useTextures);
    }
}

void HotTileCache::putBatch(TileMap&& tiles, bool useTextures)
{
    for (auto it = tiles.begin(); it != tiles.end(); ++it) {
        TileKey     key = it.key();
        SurfaceTile val = std::move(it.value());
        upsertMove(std::move(key), std::move(val), useTextures);
    }

    tiles.clear();
}

TileMap HotTileCache::getForKeys(const QSet<TileKey>& keys, QSet<TileKey>* missing)
{
    TileMap out;
    out.reserve(keys.size());

    for (const auto& k : keys) {
        auto it = index_.find(k);
        if (it == index_.end()) {
            if (missing) missing->insert(k);
            continue;
        }

        ListIt nodeIt = it.value();

        if (!nodeIt->hasTextures) {
            if (missing) missing->insert(k);
            continue;
        }

        touch(nodeIt);
        out.insert(k, nodeIt->tile);
    }

    return out;
}

bool HotTileCache::contains(const TileKey &k) const
{
    return index_.contains(k);
}

size_t HotTileCache::size() const
{
    return size_t(index_.size());
}

void HotTileCache::touch(ListIt it)
{
    nodes_.splice(nodes_.begin(), nodes_, it);
}

void HotTileCache::upsertCopy(const TileKey &key, const SurfaceTile &val, bool useTextures)
{
    auto it = index_.find(key);

    if (it != index_.end()) {
        ListIt nodeIt = it.value();
        nodeIt->tile        = val;
        nodeIt->hasTextures = useTextures;
        touch(nodeIt);
    }
    else {
        nodes_.push_front(Node{ key, val, useTextures });
        index_.insert(nodes_.front().key, nodes_.begin());
        evictIfNeeded();
    }
}

void HotTileCache::upsertMove(TileKey key, SurfaceTile &&val, bool useTextures)
{
    auto it = index_.find(key);

    if (it != index_.end()) {
        ListIt nodeIt = it.value();
        nodeIt->tile        = std::move(val);
        nodeIt->hasTextures = useTextures;
        touch(nodeIt);
    }
    else {
        nodes_.push_front(Node{ std::move(key), std::move(val), useTextures });
        index_.insert(nodes_.front().key, nodes_.begin());
        evictIfNeeded();
    }
}

void HotTileCache::evictIfNeeded()
{
    if (!maxCapacity_) {
        return;
    }

    while (size_t(index_.size()) > maxCapacity_) {
        while (size_t(index_.size()) > minCapacity_) {
            auto last = std::prev(nodes_.end());
            index_.remove(last->key);
            nodes_.erase(last);

            if (size_t(index_.size()) <= minCapacity_) {
                break;
            }
        }

        if (size_t(index_.size()) <= maxCapacity_) {
            break;
        }
    }
}

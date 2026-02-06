#include "hot_tile_cache.h"
#include "data_processor.h"


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

void HotTileCache::setDataProcessorPtr(DataProcessor *ptr)
{
    dataProcPtr_ = ptr;
}

void HotTileCache::setCapacity(size_t maxCap, size_t minCap)
{
    maxCapacity_ = maxCap;
    minCapacity_ = (minCap ? minCap : (maxCap ? maxCap / 2 : 0));
    evictIfNeeded();
}

void HotTileCache::putBatch(TileMap&& tiles, DataSource source)
{
    for (auto it = tiles.begin(); it != tiles.end(); ++it) {
        TileKey     key = it.key();
        SurfaceTile val = std::move(it.value());

        upsertMove(std::move(key), std::move(val), source);
    }

    tiles.clear();

    evictIfNeeded();
}

TileMap HotTileCache::getForKeys(const QSet<TileKey>& keys, QSet<TileKey>* missing)
{
    TileMap out;
    out.reserve(keys.size());

    for (const auto& k : keys) {
        auto it = index_.find(k);
        if (it == index_.end()) {
            if (missing) {
                missing->insert(k);
            }
            continue;
        }

        ListIt nodeIt = it.value();

        touch(nodeIt);
        out.insert(k, nodeIt->tile);
    }

    return out;
}

bool HotTileCache::contains(const TileKey &k) const
{
    return index_.contains(k);
}

bool HotTileCache::checkAnyTileForZoom(int targetZoom) const
{
    for (auto it = index_.cbegin(); it != index_.cend(); ++it) {
        if (it.key().zoom == targetZoom) {
            return true;
        }
    }

    return false;
}

size_t HotTileCache::size() const
{
    return size_t(index_.size());
}

void HotTileCache::onSendSavedTiles(const QVector<TileKey> &savedKeys)
{
    for (const TileKey& k : savedKeys) {
        auto it = index_.find(k);
        if (it == index_.end()) {
            continue;
        }

        ListIt nodeIt = it.value();
        if (!nodeIt->blocked) { // узел уже обновили более свежими данными — игнорируем старый ACK
            continue;
        }

        index_.remove(k);
        nodes_.erase(nodeIt);
    }
}

void HotTileCache::touch(ListIt it)
{
    if (it->blocked) { // если заблокирован и ждёт ACK то не двигаем с хвостп
        return;
    }

    nodes_.splice(nodes_.begin(), nodes_, it);
}

void HotTileCache::upsertMove(TileKey key, SurfaceTile &&val, DataSource source)
{
    auto it = index_.find(key);
    const bool incomingIsCalc = (source == DataSource::kCalculation);

    if (it != index_.end()) {
        ListIt nodeIt = it.value();

        const bool curBlocked   = nodeIt->blocked;
        const bool curIsCalc    = (nodeIt->source == DataSource::kCalculation);

        if (curBlocked) {
            if (!incomingIsCalc) { // узел заблокирован и пришла db-версия - игнорируем
                return;
            }
            else {
                nodeIt->tile        = std::move(val);
                nodeIt->source      = source;
                nodeIt->blocked     = false; // снимаем блок
                touch(nodeIt);
            }
        }
        else {
            if (curIsCalc && !incomingIsCalc) { // в кеше расчёт (актуальнее), а пришло из БД - игнорируем
                return;
            }
            nodeIt->tile        = std::move(val); // иначе принимаем обновление
            nodeIt->source      = source;
            touch(nodeIt);
        }
    }
    else {
        nodes_.push_front(Node{ std::move(key), std::move(val), source, /*blocked*/false }); // новый узел
        index_.insert(nodes_.front().key, nodes_.begin());
    }
}

void HotTileCache::evictIfNeeded()
{
    if (!maxCapacity_) {
        return;
    }

    const size_t total = size();
    if (total <= maxCapacity_) {
        return;
    }

    // Считаем текущее число НЕзаблокированных (горячих)
    size_t unblocked = 0, blocked = 0;
    for (const auto& n : nodes_) {
        n.blocked ? ++blocked : ++unblocked;
    }

    //qDebug() << " --- evict start total:" << total << "unblocked:" << unblocked << "blocked:" << blocked << "max:" << maxCapacity_ << "min:" << minCapacity_;

    QHash<TileKey, SurfaceTile> toSave;

    // Ужимаем только незаблокированную часть до minCapacity_
    // total может стать > maxCapacity_, если почти все стали blocked — это ок. удатся после onSendSavedTiles().
    while (unblocked > minCapacity_ && !nodes_.empty()) {
        auto it = nodes_.end();
        bool found = false;

        while (it != nodes_.begin()) { // ищем с хвоста первый незаблокированный
            --it;

            if (!it->blocked) {
                found = true;
                break;
            }
        }

        if (!found) {
            break;
        }

        if (it->source == DataSource::kCalculation) { // после процессинга блокируем в кеше и отправим в БД
            it->blocked = true;
            toSave.insert(it->key, it->tile);

            --unblocked;
            ++blocked;
        }
        else { // узел из БД/кэша удаляем сразу
            const TileKey key = it->key;
            index_.remove(key);
            it = nodes_.erase(it);

            --unblocked;
        }
    }

    if (!toSave.isEmpty() && dataProcPtr_) {
        dataProcPtr_->onDbSaveTiles(toSave);
    //    qDebug() << " --- evict save batch:" << toSave.size() << "unblocked(now):" << unblocked;
    }
}

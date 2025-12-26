#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include <QString>
#include <memory>

#include "tile_set.h"
#include "tile_provider.h"
#include "tile_downloader.h"
#include "tile_db.h"


namespace map {


class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject* parent = nullptr);
    ~TileManager();

    std::shared_ptr<TileSet> getTileSetPtr() const;
    int32_t currentProviderId() const;
    QString currentProviderName() const;
    void setProvider(int32_t providerId);
    void toggleProvider();

public slots:
    void getRectRequest(QVector<LLA> request, bool isPerspective, LLARef viewLlaRef, bool moveUp, map::CameraTilt tiltCam);
    void getLlaRef(LLARef viewLlaRef);

signals:
    void providerChanged(int32_t providerId);

private:
    static QString providerNameForId(int32_t providerId);

    std::shared_ptr<TileProvider> tileProvider_;
    std::shared_ptr<TileDownloader> tileDownloader_;
    std::shared_ptr<TileDB> tileDB_;
    std::shared_ptr<TileSet> tileSet_;
    int lastZoomLevel_;
    int32_t providerId_;

    static constexpr int maxTilesCapacity_{ 800 };
    static constexpr int minTilesCapacity_{ 400 };
    static constexpr int maxConcurrentDownloads_{ 10 };
};


} // namespace map


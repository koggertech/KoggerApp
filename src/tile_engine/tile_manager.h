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
    void setInternetAvailable(bool available);
    bool isInternetAvailable() const;
    void setMapEnabled(bool enabled);
    bool isMapEnabled() const;

public slots:
    void getRectRequest(QVector<LLA> request, bool isPerspective, LLARef viewLlaRef, bool moveUp, map::CameraTilt tiltCam);
    void getLlaRef(LLARef viewLlaRef);

signals:
    void providerChanged(int32_t providerId);
    void internetAvailabilityChanged(bool available);
    void mapEnabledChanged(bool enabled);

private:
    static QString providerNameForId(int32_t providerId);

    int32_t providerId_;
    std::shared_ptr<TileProvider> tileProvider_;
    std::shared_ptr<TileDownloader> tileDownloader_;
    std::shared_ptr<TileDB> tileDB_;
    std::shared_ptr<TileSet> tileSet_;
    int lastZoomLevel_;
    bool internetAvailable_;
    bool mapEnabled_;

    static constexpr int maxTilesCapacity_{ 800 };
    static constexpr int minTilesCapacity_{ 400 };
    static constexpr int maxConcurrentDownloads_{ 10 };
};

} // namespace map


#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include <QString>
#include <QImage>
#include <memory>

#include "tile_set.h"
#include "tile_provider.h"
#include "tile_downloader.h"
#include "tile_db.h"

class QNetworkAccessManager;


namespace map {

class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject* parent = nullptr);
    ~TileManager() override;

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
    void imageryVersionChanged();

private slots:
    void onDownloadFailedForVersion(const map::TileIndex& tileIndx, const QString& errorString, int httpStatus);
    void onDownloadedForVersion(const map::TileIndex& tileIndx, const QImage& image);

private:
    static QString providerNameForId(int32_t providerId);
    static QString versionSettingsKey(const QString& manifestKey);

    void seedProviderVersion();
    void maybeTriggerVersionResolve();
    void startVersionResolve(quint64 generation);
    bool probeVersion(int candidate, quint64 generation, bool inWindow);
    void onWindowProbeFinished(int candidate, bool alive, quint64 generation);
    void fetchManifest(quint64 generation);
    void applyResolvedVersion(int version, bool baseAlive);

    int32_t providerId_;
    std::shared_ptr<TileProvider> tileProvider_;
    std::shared_ptr<TileDownloader> tileDownloader_;
    std::shared_ptr<TileDB> tileDB_;
    std::shared_ptr<TileSet> tileSet_;
    int lastZoomLevel_;
    bool internetAvailable_;
    bool mapEnabled_;

    QNetworkAccessManager* versionNam_;
    int versionFailureStreak_;
    bool versionResolveInFlight_;
    bool versionConfirmed_;
    quint64 versionResolveGeneration_;
    int versionResolveBase_;
    int versionResolveBest_;
    int versionWindowPending_;
    bool versionBaseAlive_;
    bool versionAnyAnswer_;
    qint64 versionNextResolveMs_;

    static constexpr int maxTilesCapacity_{ 800 };
    static constexpr int minTilesCapacity_{ 400 };
    static constexpr int maxConcurrentDownloads_{ 10 };
    static constexpr int versionFailureThreshold_{ 6 };
    static constexpr qint64 versionResolveCooldownMs_{ 60LL * 60 * 1000 };
    static constexpr qint64 versionNoAnswerRetryMs_{ 60LL * 1000 };
    static constexpr int versionProbeSpan_{ 32 };
    static constexpr int versionJitterMaxMs_{ 30 * 1000 };
    static constexpr int versionRequestTimeoutMs_{ 15 * 1000 };
    static constexpr int versionProbeTimeoutMs_{ 5 * 1000 };
};

} // namespace map

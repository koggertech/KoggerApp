#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QByteArray>
#include <QHash>
#include "data_processor_defs.h"
#include "surface_tile.h"


enum class DbRole : quint8 { Reader, Writer };

struct DbTile {
    TileKey     key;
    int         tilePx      = defaultTileSidePixelSize;
    int         hmRatio     = defaultTileHeightMatrixRatio;
    double      originX     = 0.0;   // origin при расчёте, возможно ломается при переоткрытии
    double      originY     = 0.0;
    QByteArray  mosaicBlob;   // qCompress(256*256 RAW8)
    bool        hasHeight   = false;
    int         heightFmt   = 0; // 0=float32, 1=int16+scale
    QByteArray  heightBlob;
    float       heightScale = 0.f;
    float       heightZMin  = 0.f;
    bool        hasMarks    = false;
    int         marksFmt    = 0;  // 0=u8
    QByteArray  marksBlob;
    int         headIndx    = -1;
};

class MosaicDB : public QObject
{
    Q_OBJECT
public:
    explicit MosaicDB(DbRole role, bool deleteOnClose = false, QObject* parent = nullptr);
    ~MosaicDB() override;

    // Call once from main() before the DataProcessor exists — scopes the cache file to
    // this process so two instances cannot delete each other's DB. Pass 0 when the slot
    // is unknown: the path then falls back to the PID.
    static void setInstanceIndex(int index);
    static QString surfaceDbPath();
    static bool removeDbFiles(const QString& dbPath);

public slots:
    bool open();
    void close();
    void finalizeAndClose();

    void checkAnyTileForZoom(int zoom);
    void saveTiles(int engineVer, const QHash<TileKey, SurfaceTile>& tiles, bool useTextures, int tilePx, int hmRatio);
    void loadTilesForKeys(const QSet<TileKey>& keys);

    void walCheckpointPassive();
    void walCheckpointRestart();
    void walCheckpointTruncate();
    void checkpointIfWalTooBig(uint64_t limitBytes = 128ll * 1024 * 1024);

signals:
    void anyTileForZoom(int zoom, bool exists);
    void tilesLoadedForZoom(int zoom, const QList<DbTile>& tiles);
    void tilesLoadedForKeys(const QList<DbTile>& tiles);
    void sendSavedKeys(QVector<TileKey> savedKeys);
    void schemaReady();

private:
    friend class DataProcessor;

    bool ensureSchema();
    static QByteArray packRaw8(const std::vector<uint8_t>& v);
    static std::vector<uint8_t> unpackRaw8(const QByteArray& blob);

    static QByteArray packFloat32(const QVector<QVector3D>& verts, int hmRatio);
    static void       unpackFloat32(const QByteArray& blob, QVector<QVector3D>& verts, int hmRatio);

    static QByteArray packMarksU8(const QVector<HeightType>& marks);
    static void       unpackMarksU8(const QByteArray& blob, QVector<HeightType>& marks);

private:
    const int    kMaxPairsPerBatch_ = 128; // запись пачками
    const int    busyTimeoutMs_ = 2000;
    QSqlDatabase db_;
    QString      dbPath_;
    QString      connName_;
    DbRole       role_;
    bool         deleteOnClose_;
    bool         filesDeleted_;
};

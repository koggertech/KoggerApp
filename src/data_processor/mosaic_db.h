#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QByteArray>
#include <QHash>
#include "data_processor_defs.h"
#include "surface_tile.h"


struct DbTile {
    TileKey     key;
    int         tilePx      = defaultTileSidePixelSize;
    int         hmRatio     = defaultTileHeightMatrixRatio;
    double      originX     = 0.0;   // origin при расчёте, возможно ломается при переоткрытии
    double      originY     = 0.0;
    bool        hasMosaic   = false;
    QByteArray  mosaicBlob;   // qCompress(256*256 RAW8)
    bool        hasHeight   = false;
    int         heightFmt   = 0; // 0=float32, 1=int16+scale
    QByteArray  heightBlob;
    float       heightScale = 0.f;
    float       heightZMin  = 0.f;
    bool        hasMarks    = false;
    int         marksFmt    = 0;  // 0=u8
    QByteArray  marksBlob;
};

class MosaicDB : public QObject
{
    Q_OBJECT
public:
    explicit MosaicDB(const QString& klfPath, QObject* parent = nullptr);
    ~MosaicDB();

public slots:
    bool open();
    void close();

    void checkAnyTileForZoom(int zoom);
    void loadTilesForZoom(int zoom); //
    void saveTiles(int engineVer, const QHash<TileKey, SurfaceTile>& tiles, bool useTextures, int tilePx, int hmRatio);
    void loadTilesForKeys(const QSet<TileKey>& keys);

signals:
    void anyTileForZoom(int zoom, bool exists);
    void tilesLoadedForZoom(int zoom, const QList<DbTile>& tiles);
    void tilesLoadedForKeys(const QList<DbTile>& tiles);

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
    const int    kMaxPairsPerBatch_ = 128;
    QSqlDatabase db_;
    QString      klfPath_;
    QString      dbPath_;
    QString      connName_;
};

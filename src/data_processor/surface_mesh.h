#pragma once

#include <vector>
#include <QHash>
#include <QVector3D>
#include "surface_tile.h"
#include "math_defs.h"
#include "data_processor_defs.h"


class SurfaceMesh {
public:
    /*methods*/
    SurfaceMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    ~SurfaceMesh();

    QSet<SurfaceTile*> getUpdatedTiles() const;
    void reinit(int tileSidePixelSize, int heightMatrixRatio, float tileResolution);
    bool concatenate(kmath::MatrixParams& actualMatParams);
    void printMatrix() const;
    void clear();

    void clearHeightData(HeightType heightType = HeightType::kUndefined);
    bool hasData() const;

    const std::vector<SurfaceTile*>&        getTilesCRef() const;
    std::vector<std::vector<SurfaceTile*>>& getTileMatrixRef();

    inline SurfaceTile* getTileByXYIndxs(int ix, int iy) noexcept { return tileMatrix_[ix][iy]; }

    SurfaceTile*                            getTilePtrByKey(const TileKey& key);
    const SurfaceTile*                      getTileCPtrByKey(const TileKey& key) const;
    std::pair<bool, SurfaceTile>            getTileCopyByKey(const TileKey& key) const;

    int                                     getCurrentZoom() const;
    inline int getPixelWidth() const noexcept {
        return numWidthTiles_ * tileSidePixelSize_;
    };
    inline int getPixelHeight() const noexcept {
        return numHeightTiles_ * tileSidePixelSize_;
    };
    inline QVector3D convertPhToPixCoords(const QVector3D& phCoords) const noexcept {
        return QVector3D((phCoords.x() - origin_.x()) * invRes_, (phCoords.y() - origin_.y()) * invRes_, 0.0f);
    }

    int                                     getTileSidePixelSize() const;
    int                                     getNumWidthTiles() const;
    int                                     getNumHeightTiles() const;
    int                                     getStepSizeHeightMatrix() const;
    bool                                    getIsInited() const;

    // LRU
    void setLRUWatermarks(int highA, int lowB);
    int  currentInitedTiles() const;
    int  scanInitedTiles(); // debug
    void setTileUsed(const QSet<SurfaceTile*>& written, bool evict);
    void touch(const TileKey& key);
    void resetTileByKey(const TileKey& key);

private:
    /*methods*/
    void initializeMatrix(int numWidthTiles, int numHeightTiles);
    void resizeColumnsLeft(int columnsToAdd);
    void resizeRowsBottom(int rowsToAdd);
    void resizeColumnsRight(int columnsToAdd);
    void resizeRowsTop(int rowsToAdd);
    float getWidthMeters() const;
    float getHeightMeters() const;

    // LRU
    void evictIfNeeded();

    /*data*/
    std::vector<SurfaceTile*> tiles_;
    std::vector<std::vector<SurfaceTile*>> tileMatrix_;
    QHash<TileKey, SurfaceTile*> tileByKey_; // new map
    int   zoomIndex_{1};                     // 0..6
    QVector3D origin_;
    float tileResolution_;
    float invRes_;
    float tileSideMeterSize_;
    int numWidthTiles_;
    int numHeightTiles_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;

    // LRU
    QHash<TileKey, qint64> lru_;
    int  highWM_;
    int  lowWM_;

    // индекс тайла x,y у origin_
    qint64 baseTx_ = 0;
    qint64 baseTy_ = 0;
};

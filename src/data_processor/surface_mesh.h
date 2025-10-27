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

    void reinit(int tileSidePixelSize, int heightMatrixRatio, float tileResolution);
    bool concatenate(kmath::MatrixParams& actualMatParams);
    QVector3D convertPhToPixCoords(QVector3D phCoords) const;
    void printMatrix() const;
    void clear();

    void clearHeightData(HeightType heightType = HeightType::kUndefined);
    bool hasData() const;

    const std::vector<SurfaceTile*>&        getTilesCRef() const;
    std::vector<std::vector<SurfaceTile*>>& getTileMatrixRef();

    inline SurfaceTile* getTileByXYIndxs(int ix, int iy) { return tileMatrix_[ix][iy]; }

    SurfaceTile*                            getTilePtrByKey(const TileKey& key);
    int                                     getCurrentZoom() const;
    int                                     getPixelWidth() const;
    int                                     getPixelHeight() const;
    int                                     getTileSidePixelSize() const;
    int                                     getNumWidthTiles() const;
    int                                     getNumHeightTiles() const;
    int                                     getStepSizeHeightMatrix() const;
    bool                                    getIsInited() const;

    // LRU
    void setMaxInitedTiles(int maxTiles); // 0 - no limit
    int  getMaxInitedTiles() const;
    int  currentInitedTiles() const;
    int  scanInitedTiles(); // debug
    void onTilesWritten(const QSet<SurfaceTile*>& written);
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
    void registerNewTile(SurfaceTile* t);

    /*data*/
    std::vector<SurfaceTile*> tiles_;
    std::vector<std::vector<SurfaceTile*>> tileMatrix_;
    QHash<TileKey, SurfaceTile*> tileByKey_; // new map
    int   zoomIndex_{1};                     // 0..6
    QVector3D origin_;
    float tileResolution_;
    float tileSideMeterSize_;
    int numWidthTiles_;
    int numHeightTiles_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;

    // LRU
    QHash<TileKey, uint64_t> lru_;
    uint64_t                 tick_;
    int                      maxTrackedTiles_;
    QSet<TileKey>            initedKeys_;
    int                      initedCount_;

};

#pragma once

#include <stdint.h>
#include <vector>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLFunctions>
#include "data_processor_defs.h"

enum class UpdateHint : uint8_t
{
    kUndefined = 0,
    kUpdateTexture,
    kAddOrUpdateTexture
};

class SurfaceTile;
using TileMap = QHash<TileKey, SurfaceTile>;

static constexpr int   defaultTileSidePixelSize     = 256;
static constexpr int   defaultTileHeightMatrixRatio = 16;
static constexpr float defaultTileResolution        = 1.0f / 10.f;

enum class HeightType {
    kUndefined = 0,
    kExrtapolation,
    kMosaic,
    kTriangulation
};

class SurfaceTile {
public:
    /*methods*/
    SurfaceTile();
    SurfaceTile(QVector3D origin);
    SurfaceTile(const TileKey& key, QVector3D origin);
    void init(int sidePixelSize, int heightMatrixRatio, float resolution);
    void updateHeightIndices(); // обновляет индексы для отрисовки

    void       resetInitData();

    void       setUpdateHint(UpdateHint h);
    UpdateHint updateHint() const;

    void                        setMosaicTextureId(GLuint val);
    void                        setIsUpdated(bool state);
    const TileKey&              getKey() const;
    QVector3D                   getOrigin() const;
    bool                        getIsInited() const;
    GLuint                      getMosaicTextureId() const;
    int                         getIsUpdated() const;
    std::vector<uint8_t>&       getMosaicImageDataRef();
    const std::vector<uint8_t>& getMosaicImageDataCRef() const;
    QVector<QVector3D>&         getHeightVerticesRef();
    QVector<HeightType>&        getHeightMarkVerticesRef();
    const QVector<QVector2D>&   getMosaicTextureVerticesCRef() const;
    const QVector<QVector3D>&   getHeightVerticesCRef() const;
    const QVector<HeightType>&  getHeightMarkVerticesCRef() const;
    const QVector<int>&         getHeightIndicesCRef() const;
    // tile size
    int sidePixelSize() const;
    int heightMatrixRatio() const;
    float resolution() const;
    float sideWorld() const;
    void footprint(float& x0, float& x1, float& y0, float& y1) const;

private:
    friend class SurfaceView;

    /*methods*/
    inline bool checkVerticesDepth(int topLeft, int topRight, int bottomLeft, int bottomRight) const;

    /*data*/
    TileKey   key_;
    QVector3D origin_;
    std::vector<uint8_t> imageData_;        // текстура
    QVector<QVector3D> heightVertices_;     // матрица высот //
    QVector<HeightType> heightMarkVertices_;// что будем процессить в матрице высот //
    QVector<int> heightIndices_;            // что будем рисовать в матрице высот ////
    QVector<QVector2D> textureVertices_;    // текстурные координаты //
    GLuint textureId_;
    bool isUpdated_;
    bool isInited_; //
    // tile size
    int   sidePixelSize_;
    int   heightMatrixRatio_;
    float resolution_;
    UpdateHint updateHint_;
};

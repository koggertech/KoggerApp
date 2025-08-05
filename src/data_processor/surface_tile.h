#pragma once

#include <stdint.h>
#include <vector>
#include <QUuid>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLFunctions>
#include "scene_object.h"


static constexpr int   defaultTileSidePixelSize     = 256;
static constexpr int   defaultTileHeightMatrixRatio = 16;
static constexpr float defaultTileResolution        = 1.0f / 10.f;

enum class HeightType {
    kUndefined = 0,
    kMosaic,
    kIsobaths
};

class SurfaceTile {
public:
    /*methods*/
    SurfaceTile(QVector3D origin, bool generateGridContour);
    void init(int sidePixelSize, int heightMatrixRatio, float resolution);
    void updateHeightIndices();

    void setMosaicTextureId(GLuint val);
    void setIsPostUpdate(bool state);
    QUuid                                    getUuid() const;
    QVector3D                                getOrigin() const;
    bool                                     getIsInited() const;
    GLuint                                   getMosaicTextureId() const;
    int                                      getIsPostUpdate() const;
    std::vector<uint8_t>&                    getMosaicImageDataRef();
    const std::vector<uint8_t>&              getMosaicImageDataCRef() const;
    QVector<QVector3D>&                      getHeightVerticesRef();
    QVector<HeightType>&                     getHeightMarkVerticesRef();
    const QVector<QVector2D>&                getMosaicTextureVerticesRef() const;
    const QVector<QVector3D>&                getHeightVerticesConstRef() const;
    const QVector<int>&                      getHeightIndicesRef() const;
    const SceneObject::RenderImplementation& getGridRenderImplRef() const;
    const SceneObject::RenderImplementation& getContourRenderImplRef() const;

private:
    /*methods*/
    bool checkVerticesDepth(int topLeft, int topRight, int bottomLeft, int bottomRight) const;

    /*data*/
    QUuid id_;
    QVector3D origin_;
    std::vector<uint8_t> imageData_;
    QVector<QVector3D> heightVertices_;
    QVector<HeightType> heightMarkVertices_; // mosaic trace or not
    QVector<int> heightIndices_;
    QVector<QVector2D> textureVertices_;
    SceneObject::RenderImplementation gridRenderImpl_;
    SceneObject::RenderImplementation contourRenderImpl_;
    GLuint textureId_;
    bool isPostUpdate_;
    bool isInited_;
    bool generateGridContour_;
};

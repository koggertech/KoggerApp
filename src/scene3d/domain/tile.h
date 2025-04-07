#pragma once

#include <vector>
#include <QUuid>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLFunctions>
#include "scene_object.h"


class GlobalMesh;
class Tile {
public:
    /*methods*/
    Tile(QVector3D origin, bool generateGridContour);
    void init(int sidePixelSize, int heightMatrixRatio, float resolution);
    void updateHeightIndices();

    void setTextureId(GLuint val);
    void setIsUpdate(bool state);
    QUuid                                    getUuid() const;
    QVector3D                                getOrigin() const;
    bool                                     getIsInited() const;
    GLuint                                   getTextureId() const;
    int                                      getIsUpdate() const;
    std::vector<uint8_t>&                    getImageDataRef();
    QVector<QVector3D>&                      getHeightVerticesRef();
    QVector<char>&                           getHeightMarkVerticesRef();
    const QVector<QVector2D>&                getTextureVerticesRef() const;
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
    QVector<char> heightMarkVertices_;
    QVector<int> heightIndices_;
    QVector<QVector2D> textureVertices_;
    SceneObject::RenderImplementation gridRenderImpl_;
    SceneObject::RenderImplementation contourRenderImpl_;
    GLuint textureId_;
    bool isUpdate_;
    bool isInited_;
    bool generateGridContour_;
};

#pragma once

#include <QUuid>
#include <QImage>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLFunctions>

#include "sceneobject.h"

class GlobalMesh;
class Tile {
public:
    Tile();

    void initTile(QVector3D origin, int heightRatio, int tileSize, QImage::Format imageFormat = QImage::Format_Indexed8);

    void setSomeInt(int val);
    void setTextureId(GLuint val);
    void setIsUpdate(bool val);

    QUuid       getUuid() const;
    QVector3D   getTileOrigin() const;
    bool        getIsInited() const;
    int         getSomeInt() const;
    GLuint      getTextureId() const;
    int         getIsUpdate() const;

    QImage&                                     getImageRef();
    QImage                                      getImage();
    const QVector<QVector2D>&                   getTextureVerticesRef() const;
    const QVector<QVector3D>&                   getHeightVerticesRef() const;
    QVector<QVector3D>&                   		getHeightVerticesRef();
    const QVector<int>&                         getHeightIndicesRef() const;
    const SceneObject::RenderImplementation&    getGridRenderImplRef() const;


private:
    QUuid id_;
    QVector3D tileOrigin_;
    QImage image_;

    QVector<QVector3D> heightVertices_;
    QVector<int> heightIndices_;
    GLuint textureId_;
    QVector<QVector2D> textureVertices_;

    SceneObject::RenderImplementation gridRenderImpl_;

    int someInt_; // test
    bool isUpdate_;
    bool isInited_;
};

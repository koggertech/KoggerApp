#pragma once

#include <QUuid>
#include <QImage>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QOpenGLFunctions>


class Tile {
public:
    Tile();

    Tile(const Tile& other)
        : image_(other.image_),
        textureVertices_(other.textureVertices_),
        heightVertices_(other.heightVertices_),
        heightIndices_(other.heightIndices_),
        id_(other.id_),
        someInt_(other.someInt_),
        textureId_(other.textureId_),
        isUpdate_(other.isUpdate_) {}

    Tile& operator=(const Tile& other) {
        if (this != &other) {
            image_ = other.image_;
            textureVertices_ = other.textureVertices_;
            heightVertices_ = other.heightVertices_;
            heightIndices_ = other.heightIndices_;
            id_ = other.id_;
            someInt_ = other.someInt_;
            textureId_ = other.textureId_;
            isUpdate_ = other.isUpdate_;
        }
        return *this;
    }

    Tile(Tile&& other) noexcept
        : image_(std::move(other.image_)),
        textureVertices_(std::move(other.textureVertices_)),
        heightVertices_(std::move(other.heightVertices_)),
        heightIndices_(std::move(other.heightIndices_)),
        id_(std::move(other.id_)),
        someInt_(other.someInt_),
        textureId_(other.textureId_),
        isUpdate_(other.isUpdate_) {}

    Tile& operator=(Tile&& other) noexcept {
        if (this != &other) {
            image_ = std::move(other.image_);
            textureVertices_ = std::move(other.textureVertices_);
            heightVertices_ = std::move(other.heightVertices_);
            heightIndices_ = std::move(other.heightIndices_);
            id_ = std::move(other.id_);
            someInt_ = other.someInt_;
            textureId_ = other.textureId_;
            isUpdate_ = other.isUpdate_;
        }
        return *this;
    }




    void initTile(QVector3D origin, int heightRatio, int tileSize, QImage::Format imageFormat = QImage::Format_Indexed8);

    void setSomeInt(int val);
    void setTextureId(GLuint val);
    void setIsUpdate(bool val);

    QUuid getUuid() const;
    int getSomeInt() const;
    GLuint getTextureId() const;
    int getIsUpdate() const;


    QImage& getImageRef();
    QImage getImage();

    const QVector<QVector2D>& getTextureVerticesRef() const;
    const QVector<QVector3D>& getHeightVerticesRef() const;
    const QVector<int>& getHeightIndicesRef() const;


    QVector3D tileOrigin_;

private:
    QImage image_;
    QVector<QVector2D> textureVertices_;

    QVector<QVector3D> heightVertices_;
    QVector<int> heightIndices_;

    QUuid id_;
    int someInt_;
    GLuint textureId_;
    bool isUpdate_;



    //SceneObject::RenderImplementation gridRenderImpl_;
    //bool gridVisible_;
};


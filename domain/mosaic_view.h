#pragma once

#include "sceneobject.h"
#include <memory>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <random>
#include <surfacegrid.h>


class MosaicView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MosaicView)

public:
    class MosaicViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        MosaicViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        void setTexture(QOpenGLTexture* texturePtr);
        void setTextureImage(QImage texture);
        void setIndices(QVector<int>& indices);
        void setNeedsTextureInit(bool state);
        void setTexCoords(const QVector<QVector2D>& texCoords);
        QOpenGLTexture* getTexturePtr();
        QImage getTextureImagePtr();
        QVector<int>& getIndicesPtr();
        bool getNeedsTextureInit();

    private:
        friend class MosaicView;

        void initializeTexture();

        SceneObject::RenderImplementation gridRenderImpl_;
        QVector<int> indices_;
        QVector<QVector2D> texCoords_;
        QOpenGLTexture* texture_ = nullptr;
        QImage textureImage_;
        bool needsTextureInit_ = false;
    };


    explicit MosaicView(QObject* parent = nullptr);
    virtual ~MosaicView();

    /*SceneObject*/
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

    void updateData();

private:
    QImage generateImage(int width, int height);
    void generateRandomVertices(int width, int height, float cellSize);
    void updateGrid();
    void makeQuadGrid();

    /*data*/
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> dis_;
    std::shared_ptr <SurfaceGrid> grid_;
    const int width_ = 100;
    const int height_ = 100;
    const float cellSize_ = 1.0f;
};

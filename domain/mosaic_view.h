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
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        void setIndices(QVector<int>& indices);
        QVector<int>& getIndicesPtr();
        void setTexture(const QImage& texture);

    private:
        friend class MosaicView;

        void initializeTexture();

        QOpenGLTexture* texture_;
        QImage textureImage_;
        bool textureInitialized_ = false;
        QVector<int> indices_;
        //QVector<QVector2D> texCoords_;
        SceneObject::RenderImplementation m_gridRenderImpl;
    };

    explicit MosaicView(QObject* parent = nullptr);
    virtual ~MosaicView();

    /*SceneObject*/
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

    void updateData();

private:
    void setTexture(const QImage& texture);
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

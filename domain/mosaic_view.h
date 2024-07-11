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
        //MosaicViewRenderImplementation()
        //{ }; // non copy

        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;


        void setTexture(const QImage& texture);
        void generateRandomVertices(int width, int height, float gridSize);

        void setGens(std::mt19937* gena, std::uniform_real_distribution<>* disa)
        {
            gen = gena;
            dis = disa;
        };

    private:
        friend class MosaicView;

        void initializeTexture();

        QOpenGLTexture* texture_;
        QImage textureImage_;
        bool textureInitialized_ = false;

        QVector<QVector3D> vertices_;
        QVector<int> indices_;
        QVector<QColor> colors_;

        QVector<QVector2D> texCoords_;


        std::mt19937* gen;//(rd());
        std::uniform_real_distribution<>* dis;//(0.0f, 1.0f);

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
    void generateRandomVertices(int width, int height, float gridSize);


    std::random_device rd;
    std::mt19937 gen;//(rd());
    std::uniform_real_distribution<> dis;//(0.0f, 1.0f);

    std::shared_ptr <SurfaceGrid> m_grid;

};

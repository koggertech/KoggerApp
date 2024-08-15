#pragma once

#include "sceneobject.h"
#include <QVector>
#include <QVector2D>
#include <random>


class MosaicView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MosaicView)

public:
    class MosaicViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        MosaicViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        friend class MosaicView;
        SceneObject::RenderImplementation gridRenderImpl_;
        QVector<int> indices_;
        QVector<QVector2D> texCoords_;
        GLuint textureId_;
    };

    explicit MosaicView(QObject* parent = nullptr);
    virtual ~MosaicView();
    void setTextureId(GLuint textureId);
    void updateData();
    void clear();

private:
    /*data*/
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> dis_;
    const int width_ = 25;
    const int height_ = 25;
    const float cellSize_ = 7.0f;
};

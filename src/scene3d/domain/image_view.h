#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>

#include "scene_object.h"


class GraphicsScene3dView;
class ImageView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ImageView)

public:

    /*structures*/
    class ImageViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        ImageViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        friend class ImageView;

        /*data*/
        QVector<int> indices_;
        QVector<QVector2D> texCoords_;
        GLuint textureId_;
    };

    /*methods*/
    explicit ImageView(QObject* parent = nullptr);
    virtual ~ImageView();

    void updateTexture(const QString& imagePath, QVector3D lt, QVector3D rb);

    void clear();

    void setView(GraphicsScene3dView* viewPtr);
    void setTextureId(GLuint textureId);
    void setUseLinearFilter(bool state);
    GLuint  getTextureId() const;
    bool    getUseLinearFilter() const;
    QImage& getTextureTasksRef();

private:
    /*methods*/

    /*data*/
    GLuint textureId_;
    QImage textureTask_;
    QVector3D lt_;
    QVector3D rb_;
    bool useLinearFilter_;
};

#pragma once

#include <memory>
#include <sceneobject.h>


class MosaicView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MosaicView)

public:
    class MosaicViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class MosaicView;
    };

    explicit MosaicView(QObject* parent = nullptr);
    virtual ~MosaicView();

    /*SceneObject*/
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

private:

};

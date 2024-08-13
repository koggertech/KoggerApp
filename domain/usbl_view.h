#pragma once

#include "sceneobject.h"
#include <memory>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QVector3D>
#include <QColor>


class UsblView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(UsblView)

public:
    class UsblViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class UsblView;

        // data

    };

    explicit UsblView(QObject* parent = nullptr);
    virtual ~UsblView();

    /*SceneObject*/
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

    void updateData();

private:
    // data

};

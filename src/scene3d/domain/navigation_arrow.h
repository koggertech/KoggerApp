#pragma once

#include "scene_object.h"


class NavigationArrow : public SceneObject
{
    Q_OBJECT
public:
    class NavigationArrowRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        NavigationArrowRenderImplementation();
        virtual ~NavigationArrowRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap<QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
        QVector3D getPosition() const;
        float getAngle() const;

    private:
        friend class NavigationArrow;

        QVector3D position_;
        float angle_ = 0.0f;
        QVector<QVector3D> arrowVertices_;
        QVector<QVector3D> arrowRibs_;
    };

    explicit NavigationArrow(QObject *parent = nullptr);
    void setPositionAndAngle(const QVector3D& position, float degAngle);
    void resetPositionAndAngle();

private:
    /*methods*/
    QVector<QVector3D> makeArrowVertices() const;
    QVector<QVector3D> makeArrowRibs() const;
};

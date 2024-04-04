#pragma once

#include <sceneobject.h>


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
    private:
        friend class NavigationArrow;
        QVector<QVector3D> cubeVertices_;
        bool isEnabled_;
    };

    explicit NavigationArrow(QObject *parent = nullptr);
    void setPositionAndAngle(const QVector3D& position, float degAngle);
    void setEnabled(bool state);
private:
    /*methods*/
    void moveToPosition(QVector<QVector3D>& cubeVertices, const QVector3D& position) const;
    void rotateByDegrees(QVector<QVector3D>& cubeVertices, float degAngle) const;
    /*data*/
    QVector<QVector3D> cubeVertices_ = {{ -2.f, -2.f,  0.f },
                                        {  0.f,  0.f,  0.f },
                                        {  2.f, -2.f,  0.f },
                                        {  0.f,  5.f,  0.f }};
    bool isEnabled_;
};

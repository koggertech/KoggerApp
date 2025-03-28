#ifndef COORDINATEAXES_H
#define COORDINATEAXES_H

#include "scene_object.h"

class CoordinateAxes : public SceneObject
{
    Q_OBJECT
public:
    class CoordinateAxesRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        CoordinateAxesRenderImplementation();
        virtual ~CoordinateAxesRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                          const QMatrix4x4& mvp,
                          const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class CoordinateAxes;
        QVector3D m_position = {0.0f, 0.0f, 0.0f};
    };

    explicit CoordinateAxes(QObject *parent = nullptr);
    void setPosition(const QVector3D& pos);

private:
    QVector3D m_position = {0.0f, 0.0f, 0.0f};
};

#endif // COORDINATEAXES_H

#ifndef BOTTOMTRACK_H
#define BOTTOMTRACK_H

#include <memory>
#include <sceneobject.h>

class BottomTrack : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BottomTrack)

public:
    class BottomTrackRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        BottomTrackRenderImplementation();
        virtual ~BottomTrackRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    };

    explicit BottomTrack(QObject* parent = nullptr);
    virtual ~BottomTrack();
    virtual SceneObjectType type() const override;
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
};

#endif // BOTTOMTRACK_H

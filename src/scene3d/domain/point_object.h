#ifndef POINTOBJECT_H
#define POINTOBJECT_H

#include "scene_object.h"

class PointObject : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PointObject)
    Q_PROPERTY(QVector3D position READ position CONSTANT)

public:
    class PointObjectRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        PointObjectRenderImplementation();
        virtual ~PointObjectRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                          const QMatrix4x4& mvp,
                          const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        void setPosition(float x, float y, float z);
        void setPosition(const QVector3D& pos);
        QVector3D position() const;
        float x() const;
        float y() const;
        float z() const;
    };

    explicit PointObject(QObject *parent = nullptr);
    explicit PointObject(float x, float y, float z) { setPosition(x, y, z); }

    SceneObjectType type() const override;
    float x() const;
    float y() const;
    float z() const;
    QVector3D position() const;

public Q_SLOTS:
    virtual void setData(const QVector <QVector3D>& data, int primitiveType = GL_POINTS) override;
    void setPosition(float x, float y, float z);
    void setPosition(const QVector3D& pos);

private:
    friend class PointGroup;
    int m_indexInGroup = 0;
};

#endif // POINTOBJECT_H

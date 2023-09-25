#ifndef POINTOBJECT_H
#define POINTOBJECT_H

#include <scenegraphicsobject.h>

class PointObject : public SceneGraphicsObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT("PointObject")
    Q_PROPERTY(QVector3D position READ position CONSTANT)

public:
    explicit PointObject(QObject *parent = nullptr);

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    virtual SceneObjectType type() const override;
    float x() const;
    float y() const;
    float z() const;
    QVector3D position() const;

public Q_SLOTS:
    void setPosition(float x, float y, float z);
    void setPosition(QVector3D pos);
    virtual void append(const QVector3D& vertex) override;
    virtual void append(const QVector<QVector3D>& other) override;
    virtual void setData(const QVector <QVector3D>& data) override;
};

#endif // POINTOBJECT_H

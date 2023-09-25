#ifndef POLYGONOBJECT_H
#define POLYGONOBJECT_H

#include <QStandardItemModel>

#include <memory>

#include <pointgroup.h>

class PolygonObject : public PointGroup
{
    Q_OBJECT
    QML_NAMED_ELEMENT("PolygonObject")

public:
    explicit PolygonObject(QObject *parent = nullptr);
    virtual ~PolygonObject();

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    virtual SceneObject::SceneObjectType type() const override;
};

#endif // POLYGONOBJECT_H

#ifndef POLYGONOBJECT_H
#define POLYGONOBJECT_H

#include <QStandardItemModel>

#include <memory>

#include "point_group.h"

class PolygonGroup;
class PolygonObject : public PointGroup
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PolygonObject)

public:
    class PolygonObjectRenderImplementation : public PointGroup::PointGroupRenderImplementation
    {
    public:
        PolygonObjectRenderImplementation();
        ~PolygonObjectRenderImplementation() override;
        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& mvp,
                    const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    };

    explicit PolygonObject(QObject *parent = nullptr);
    ~PolygonObject() override;
    SceneObject::SceneObjectType type() const override;

private:
    friend class PolygonGroup;
    int m_indexInGroup = 0;
};

#endif // POLYGONOBJECT_H

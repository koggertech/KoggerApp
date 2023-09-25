#ifndef POINTGROUP_H
#define POINTGROUP_H

#include <QStandardItemModel>

#include <scenegraphicsobject.h>

class PointObject;
class PointGroup : public SceneGraphicsObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT("PointGroup")

public:
    explicit PointGroup(QObject *parent = nullptr);

    virtual ~PointGroup();

    virtual SceneObject::SceneObjectType type() const override;

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    std::shared_ptr <PointObject> at(int index) const;

public Q_SLOTS:
    virtual void append(std::shared_ptr <PointObject> point);
    virtual void setData(const QVector <QVector3D>& data) override;
    virtual void clearData() override;
    virtual void append(const QVector3D& vertex) override;
    virtual void append(const QVector<QVector3D>& other) override;

    void removeAt(int index);

protected:
    QList <std::shared_ptr <PointObject>> m_pointList;
};

#endif // POINTGROUP_H

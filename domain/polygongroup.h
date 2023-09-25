#ifndef POLYGONGROUP_H
#define POLYGONGROUP_H

#include <QStandardItemModel>

#include <scenegraphicsobject.h>

class PolygonObject;
class PolygonGroup : public SceneGraphicsObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT("PolygonGroup")

public:
    explicit PolygonGroup(QObject *parent = nullptr);
    virtual ~PolygonGroup();

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    virtual SceneObject::SceneObjectType type() const override;
    std::shared_ptr <PolygonObject> at(int index) const;
    Q_INVOKABLE PolygonObject* polygonAt(int index);

public Q_SLOTS:
    std::shared_ptr <PolygonObject> addPolygon();
    void addPolygon(std::shared_ptr <PolygonObject> polygon);
    void removePolygon(std::shared_ptr <PolygonObject> polygon);
    void removePolygonAt(int index);
    void setData(const QVector <QVector3D>& data) override;
    void clearData() override;
    void append(const QVector3D& vertex) override;
    void append(const QVector<QVector3D>& other) override;
    void setPrimitiveType(int primitiveType) override;

private:
    QList <std::shared_ptr <PolygonObject>> m_polygonList;
};

#endif // POLYGONGROUP_H

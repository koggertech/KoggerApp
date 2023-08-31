#ifndef POLYGONGROUP_H
#define POLYGONGROUP_H

#include <QStandardItemModel>

#include <sceneobject.h>
#include <polygonlistmodel.h>

using PolygonPtr     = std::shared_ptr <PolygonObject>;

class PolygonGroup : public SceneObject
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ model CONSTANT)

public:
    explicit PolygonGroup(QObject *parent = nullptr);

    Q_INVOKABLE QStringList visualItems() const;
    Q_INVOKABLE PolygonObject *polygonAt(int index) const;

    virtual void draw(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, QMap <QString, QOpenGLShaderProgram*> shaderProgramMap) const override;

    virtual SceneObject::SceneObjectType type() const override;

    void addPolygon(std::shared_ptr <PolygonObject> polygon);
    void removePolygon(int index);
    PolygonPtr at(int index) const;
    int polygonsCount() const;
    int indexOf(std::shared_ptr <PolygonObject> polygon) const;

private:
    PolygonListModel *polygonListModel() const;
    QStandardItemModel* model() const;

signals:
    void countChanged(int count);

private:
    QList <std::shared_ptr <PolygonObject>> mPolygonList;
    std::unique_ptr <QStandardItemModel> mModel;
};

#endif // POLYGONGROUP_H

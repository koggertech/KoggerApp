#ifndef POINTGROUP_H
#define POINTGROUP_H

#include <QStandardItemModel>

#include <sceneobject.h>
#include <pointobject.h>

class PointGroup : public SceneObject
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ model CONSTANT)

public:
    explicit PointGroup(QObject *parent = nullptr);

    virtual ~PointGroup();

    virtual SceneObject::SceneObjectType type() const override;

    void addPoint(std::shared_ptr <PointObject> point);

    void removePoint(int index);

    std::shared_ptr <PointObject> at(int index) const;

    QObject* pointAt(int index) const;

private:
    QStandardItemModel *model() const;

Q_SIGNALS:
    void countChanged(int count);

private:
    std::unique_ptr <QStandardItemModel> mModel;
    QList <std::shared_ptr <PointObject>> mPointList;
};

Q_DECLARE_METATYPE(PointGroup*)

#endif // POINTGROUP_H

#ifndef POLYGONOBJECT_H
#define POLYGONOBJECT_H

#include <QStandardItemModel>

#include <memory>

#include <displayedobject.h>
#include <pointobject.h>

class PolygonGroup;

class PolygonObject : public DisplayedObject
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ model CONSTANT)

public:
    explicit PolygonObject(QObject *parent = nullptr);

    virtual ~PolygonObject();

    virtual SceneObject::SceneObjectType type() const override;

    virtual void setData(const QVector <QVector3D>& data) override;

    virtual void append(const QVector <QVector3D>& data) override;

    virtual void append(const QVector3D& point) override;

    virtual void remove(int index) override;

    int pointCount() const;

    const QVector3D& pointAt(int index) const;

    void replacePoint(int index, const QVector3D& point);

private:
    QStandardItemModel* model() const;

private:
    QList <std::shared_ptr <PointObject>> mPointList;

    std::unique_ptr <QStandardItemModel> mModel;
};

#endif // POLYGONOBJECT_H

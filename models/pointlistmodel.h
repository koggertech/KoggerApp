#ifndef POINTLISTMODEL_H
#define POINTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector3D>

#include <memory>

#include <pointobject.h>

using PointList    = QList <std::shared_ptr <PointObject>>;
using PointListPtr = std::shared_ptr <PointList>;

class PointListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum RoleNames{
        NameRole   = Qt::UserRole,
        XValueRole = Qt::UserRole + 2,
        YValueRole = Qt::UserRole + 3,
        ZValueRole = Qt::UserRole + 4
    };

    explicit PointListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QVariant data(const QModelIndex& index, int role) const override;

    void insert(int index, std::shared_ptr <PointObject> point);

    void append(std::shared_ptr <PointObject> point);

    void clear();

protected:

    virtual QHash <int, QByteArray> roleNames() const override;

private:

    PointListPtr mPointList;
    QHash <int, QByteArray> mRoleNames;
};

#endif // POINTLISTMODEL_H

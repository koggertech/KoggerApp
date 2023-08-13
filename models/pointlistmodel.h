#ifndef POINTLISTMODEL_H
#define POINTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector3D>

#include <memory>

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

    void insert(int index, const QVector3D& point);

    void append(const QVector3D& point);

    void changePoint(int index, const QVector3D& point);

    void clear();

protected:

    virtual QHash <int, QByteArray> roleNames() const override;

private:

    QList <QVector3D> mData;
    QHash <int, QByteArray> mRoleNames;
};

#endif // POINTLISTMODEL_H

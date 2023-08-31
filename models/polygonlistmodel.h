#ifndef POLYGONLISTMODEL_H
#define POLYGONLISTMODEL_H

#include <QAbstractListModel>

#include <polygonobject.h>

using PolygonList    = QList <std::shared_ptr <PolygonObject>>;
using PolygonListPtr = std::shared_ptr <PolygonList>;

class PolygonListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum RoleNames{
        NameRole       = Qt::UserRole,
        PointGroupRole = Qt::UserRole + 2
    };

    explicit PolygonListModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex& parent) const override;

    virtual QVariant data(const QModelIndex& index, int role) const override;

    int count() const;

    void insert(int index, std::shared_ptr <PolygonObject> polygon);

    void append(std::shared_ptr <PolygonObject> polygon);

    void removeAt(int index);

    PolygonListPtr polygonList() const;

protected:

    virtual QHash <int, QByteArray> roleNames() const override;

Q_SIGNALS:

    void countChanged(int count);

private:

    PolygonListPtr mPolygonList;
    QHash <int, QByteArray> mRoleNames;
};

#endif // POLYGONLISTMODEL_H

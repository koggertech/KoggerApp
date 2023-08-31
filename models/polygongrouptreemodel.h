#ifndef POLYGONGROUPTREEMODEL_H
#define POLYGONGROUPTREEMODEL_H

#include <QAbstractItemModel>
#include <polygonobject.h>

class PolygonGroupTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PolygonGroupTreeModel(QObject *parent = nullptr);

    virtual ~PolygonGroupTreeModel();

    QVariant data(const QModelIndex& index, int role) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

private:
    std::shared_ptr <PolygonObject> getItem(const QModelIndex &index) const;

private:
    std::shared_ptr <PolygonObject> mRootItem;
    QList <std::shared_ptr <PolygonObject>> mData;
};

#endif // POLYGONGROUPTREEMODEL_H

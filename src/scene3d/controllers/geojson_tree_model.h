#pragma once

#include <QAbstractListModel>
#include <QVector>

struct GeoJsonTreeNode
{
    QString id;
    QString parentId;
    QString name;
    QString geomType;
    int vertexCount{0};
    int depth{0};
    bool isFolder{false};
    bool visible{true};
    bool expanded{true};
};

class GeoJsonTreeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        ParentIdRole,
        NameRole,
        GeomTypeRole,
        VertexCountRole,
        DepthRole,
        IsFolderRole,
        VisibleRole,
        ExpandedRole
    };

    explicit GeoJsonTreeModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<GeoJsonTreeNode>& nodes() const;
    void setNodes(QVector<GeoJsonTreeNode> nodes);

private:
    QVector<GeoJsonTreeNode> nodes_;
};


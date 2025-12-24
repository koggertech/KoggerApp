#include "geojson_tree_model.h"

GeoJsonTreeModel::GeoJsonTreeModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int GeoJsonTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return nodes_.size();
}

QVariant GeoJsonTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= nodes_.size()) {
        return {};
    }
    const auto& n = nodes_.at(index.row());
    switch (role) {
    case IdRole:          return n.id;
    case ParentIdRole:    return n.parentId;
    case NameRole:        return n.name;
    case GeomTypeRole:    return n.geomType;
    case VertexCountRole: return n.vertexCount;
    case DepthRole:       return n.depth;
    case IsFolderRole:    return n.isFolder;
    case VisibleRole:     return n.visible;
    case ExpandedRole:    return n.expanded;
    default:              return {};
    }
}

QHash<int, QByteArray> GeoJsonTreeModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {ParentIdRole, "parentId"},
        {NameRole, "name"},
        {GeomTypeRole, "geomType"},
        {VertexCountRole, "vertexCount"},
        {DepthRole, "depth"},
        {IsFolderRole, "isFolder"},
        {VisibleRole, "visible"},
        {ExpandedRole, "expanded"}
    };
}

const QVector<GeoJsonTreeNode>& GeoJsonTreeModel::nodes() const
{
    return nodes_;
}

void GeoJsonTreeModel::setNodes(QVector<GeoJsonTreeNode> nodes)
{
    beginResetModel();
    nodes_ = std::move(nodes);
    endResetModel();
}


#include "geojson_tree_model.h"

#include <QHash>

GeoJsonTreeModel::GeoJsonTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , root_(std::make_unique<Node>())
{}

QModelIndex GeoJsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0 || row < 0) {
        return {};
    }

    Node* parentNode = nodeFromIndex(parent);
    if (!parentNode) {
        return {};
    }
    if (row >= parentNode->children.size()) {
        return {};
    }
    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex GeoJsonTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto* node = static_cast<Node*>(index.internalPointer());
    if (!node || node->parent == root_.get() || !node->parent) {
        return {};
    }

    const int row = rowOfNode(node->parent);
    if (row < 0) {
        return {};
    }
    return createIndex(row, 0, node->parent);
}

int GeoJsonTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }
    Node* parentNode = nodeFromIndex(parent);
    return parentNode ? parentNode->children.size() : 0;
}

int GeoJsonTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant GeoJsonTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    auto* node = static_cast<Node*>(index.internalPointer());
    if (!node) {
        return {};
    }
    const auto& n = node->data;

    switch (role) {
    case Qt::DisplayRole:
        return n.isFolder ? n.name : n.geomType;
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
        {Qt::DisplayRole, "display"},
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

bool GeoJsonTreeModel::hasChildren(const QModelIndex& parent) const
{
    Node* parentNode = nodeFromIndex(parent);
    return parentNode && !parentNode->children.isEmpty();
}

const QVector<GeoJsonTreeNode>& GeoJsonTreeModel::nodes() const
{
    return nodes_;
}

void GeoJsonTreeModel::setNodes(QVector<GeoJsonTreeNode> nodes)
{
    beginResetModel();

    nodes_ = std::move(nodes);
    storage_.clear();
    root_ = std::make_unique<Node>();

    QHash<QString, Node*> idMap;
    QVector<Node*> order;
    order.reserve(nodes_.size());
    storage_.reserve(static_cast<size_t>(nodes_.size()));

    for (const auto& data : nodes_) {
        auto node = std::make_unique<Node>();
        node->data = data;
        Node* raw = node.get();
        storage_.push_back(std::move(node));
        idMap.insert(data.id, raw);
        order.push_back(raw);
    }

    for (Node* node : order) {
        Node* parent = nullptr;
        if (!node->data.parentId.isEmpty()) {
            parent = idMap.value(node->data.parentId, nullptr);
        }
        if (!parent) {
            parent = root_.get();
        }
        node->parent = parent;
        parent->children.append(node);
    }

    endResetModel();
}

GeoJsonTreeModel::Node* GeoJsonTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return root_.get();
    }
    return static_cast<Node*>(index.internalPointer());
}

int GeoJsonTreeModel::rowOfNode(const Node* node) const
{
    if (!node || !node->parent) {
        return -1;
    }
    const auto& siblings = node->parent->children;
    for (int i = 0; i < siblings.size(); ++i) {
        if (siblings.at(i) == node) {
            return i;
        }
    }
    return -1;
}

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
    idMap_.clear();

    QVector<Node*> order;
    order.reserve(nodes_.size());
    storage_.reserve(static_cast<size_t>(nodes_.size()));

    for (const auto& data : nodes_) {
        auto node = std::make_unique<Node>();
        node->data = data;
        Node* raw = node.get();
        storage_.push_back(std::move(node));
        idMap_.insert(data.id, raw);
        order.push_back(raw);
    }

    for (Node* node : order) {
        Node* parent = nullptr;
        if (!node->data.parentId.isEmpty()) {
            parent = idMap_.value(node->data.parentId, nullptr);
        }
        if (!parent) {
            parent = root_.get();
        }
        node->parent = parent;
        parent->children.append(node);
    }

    endResetModel();
}

QVariant GeoJsonTreeModel::roleData(const QModelIndex& index, int role) const
{
    return data(index, role);
}

bool GeoJsonTreeModel::insertNode(const GeoJsonTreeNode& node)
{
    if (idMap_.contains(node.id)) {
        return false;
    }

    Node* parent = root_.get();
    if (!node.parentId.isEmpty()) {
        parent = idMap_.value(node.parentId, root_.get());
    }

    const int row = parent->children.size();
    beginInsertRows(indexForNode(parent), row, row);

    auto n = std::make_unique<Node>();
    n->data = node;
    n->parent = parent;
    Node* raw = n.get();
    storage_.push_back(std::move(n));
    parent->children.append(raw);
    idMap_.insert(node.id, raw);
    nodes_.append(node);

    endInsertRows();
    return true;
}

bool GeoJsonTreeModel::removeNode(const QString& id)
{
    Node* node = idMap_.value(id, nullptr);
    if (!node || node == root_.get()) {
        return false;
    }

    Node* parent = node->parent ? node->parent : root_.get();
    const int row = rowOfNode(node);
    if (row < 0) {
        return false;
    }

    QVector<QString> ids;
    collectIds(node, ids);

    beginRemoveRows(indexForNode(parent), row, row);
    parent->children.removeAt(row);
    endRemoveRows();

    for (const auto& rid : ids) {
        idMap_.remove(rid);
        for (int i = 0; i < nodes_.size(); ++i) {
            if (nodes_.at(i).id == rid) {
                nodes_.removeAt(i);
                break;
            }
        }
        for (auto it = storage_.begin(); it != storage_.end(); ++it) {
            if ((*it)->data.id == rid) {
                storage_.erase(it);
                break;
            }
        }
    }

    return true;
}

bool GeoJsonTreeModel::updateNodeVisible(const QString& id, bool visible)
{
    Node* node = idMap_.value(id, nullptr);
    if (!node) {
        return false;
    }
    if (node->data.visible == visible) {
        return true;
    }
    node->data.visible = visible;
    const QModelIndex idx = indexForNode(node);
    emit dataChanged(idx, idx, {VisibleRole});
    return true;
}

bool GeoJsonTreeModel::updateNodeName(const QString& id, const QString& name)
{
    Node* node = idMap_.value(id, nullptr);
    if (!node) {
        return false;
    }
    if (node->data.name == name) {
        return true;
    }
    node->data.name = name;
    for (auto& n : nodes_) {
        if (n.id == id) {
            n.name = name;
            break;
        }
    }
    const QModelIndex idx = indexForNode(node);
    emit dataChanged(idx, idx, {NameRole, Qt::DisplayRole});
    return true;
}

bool GeoJsonTreeModel::updateNodeVertexCount(const QString& id, int vertexCount)
{
    Node* node = idMap_.value(id, nullptr);
    if (!node) {
        return false;
    }
    if (node->data.vertexCount == vertexCount) {
        return true;
    }
    node->data.vertexCount = vertexCount;
    const QModelIndex idx = indexForNode(node);
    emit dataChanged(idx, idx, {VertexCountRole});
    return true;
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

QModelIndex GeoJsonTreeModel::indexForNode(const Node* node) const
{
    if (!node || node == root_.get()) {
        return {};
    }
    const int row = rowOfNode(node);
    if (row < 0) {
        return {};
    }
    return createIndex(row, 0, const_cast<Node*>(node));
}

void GeoJsonTreeModel::collectIds(Node* node, QVector<QString>& ids) const
{
    if (!node) {
        return;
    }
    ids.push_back(node->data.id);
    for (auto* child : node->children) {
        collectIds(child, ids);
    }
}

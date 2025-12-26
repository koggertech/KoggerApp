#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <memory>
#include <vector>

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
};

class GeoJsonTreeModel : public QAbstractItemModel
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
    };

    explicit GeoJsonTreeModel(QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    const QVector<GeoJsonTreeNode>& nodes() const;
    void setNodes(QVector<GeoJsonTreeNode> nodes);
    Q_INVOKABLE QVariant roleData(const QModelIndex& index, int role) const;
    bool insertNode(const GeoJsonTreeNode& node);
    bool removeNode(const QString& id);
    bool updateNodeVisible(const QString& id, bool visible);
    bool updateNodeVertexCount(const QString& id, int vertexCount);

private:
    struct Node {
        GeoJsonTreeNode data;
        Node* parent{nullptr};
        QVector<Node*> children;
    };

    Node* nodeFromIndex(const QModelIndex& index) const;
    int rowOfNode(const Node* node) const;
    QModelIndex indexForNode(const Node* node) const;
    void collectIds(Node* node, QVector<QString>& ids) const;

    QVector<GeoJsonTreeNode> nodes_;
    std::unique_ptr<Node> root_;
    std::vector<std::unique_ptr<Node>> storage_;
    QHash<QString, Node*> idMap_;
};

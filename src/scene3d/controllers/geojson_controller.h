#pragma once

#include <QObject>
#include <QModelIndex>
#include <QString>
#include <QVector>
#include <memory>

#include "geojson_defs.h"
#include "geojson_feature_model.h"
#include "geojson_tree_model.h"

class GeoJsonController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* featureModel READ featureModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* treeModel READ treeModel CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(bool drawing READ drawing NOTIFY drawingChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString selectedFeatureId READ selectedFeatureId NOTIFY selectionChanged)
    Q_PROPERTY(int selectedVertexIndex READ selectedVertexIndex NOTIFY selectionChanged)
    Q_PROPERTY(QString currentFolderId READ currentFolderId NOTIFY currentFolderChanged)
    Q_PROPERTY(QString currentFolderName READ currentFolderName NOTIFY currentFolderChanged)
    Q_PROPERTY(QString selectedNodeId READ selectedNodeId NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedNodeIsFolder READ selectedNodeIsFolder NOTIFY selectionChanged)

public:
    enum Tool
    {
        Select = 0,
        DrawPoint = 1,
        DrawLine = 2,
        DrawPolygon = 3
    };
    Q_ENUM(Tool)

    explicit GeoJsonController(QObject* parent = nullptr);
    ~GeoJsonController() override;

    QAbstractItemModel* featureModel();
    QAbstractItemModel* treeModel();

    bool enabled() const;
    void setEnabled(bool enabled);

    int tool() const;
    void setTool(int tool);

    bool drawing() const;

    QString currentFile() const;
    QString lastError() const;

    QString selectedFeatureId() const;
    int selectedVertexIndex() const;
    QString currentFolderId() const;
    QString currentFolderName() const;
    QString selectedNodeId() const;
    bool selectedNodeIsFolder() const;

    const GeoJsonDocument& document() const;
    const QVector<GeoJsonCoord>& draftCoords() const;
    bool previewActive() const;
    GeoJsonCoord previewCoord() const;

    Q_INVOKABLE void newDocument();
    Q_INVOKABLE bool loadFile(const QString& path);
    Q_INVOKABLE bool saveFile(const QString& path);
    Q_INVOKABLE bool loadKgt(const QString& path);
    Q_INVOKABLE bool saveKgt(const QString& path);
    Q_INVOKABLE bool importGeoJsonToFolder(const QString& path, const QString& folderId);
    Q_INVOKABLE bool exportFolder(const QString& path, const QString& folderId);
    Q_INVOKABLE void setCurrentFolder(const QString& folderId);
    Q_INVOKABLE void selectNode(const QString& nodeId, bool isFolder, const QString& parentId);
    Q_INVOKABLE void selectIndex(const QModelIndex& index);
    Q_INVOKABLE void addFolderToRoot();
    Q_INVOKABLE void addFolderToCurrent();
    Q_INVOKABLE void toggleFolderExpanded(const QString& folderId);
    Q_INVOKABLE void setNodeVisible(const QString& nodeId, bool isFolder, bool visible);

    Q_INVOKABLE void finishDrawing();
    Q_INVOKABLE void cancelDrawing();
    Q_INVOKABLE void undoLastVertex();
    Q_INVOKABLE void deleteSelectedFeature();

    Q_INVOKABLE void selectFeature(const QString& id);

    void selectVertex(const QString& featureId, int vertexIndex);

    void setPreview(const GeoJsonCoord& c);
    void clearPreview();

    void addDraftVertex(const GeoJsonCoord& c);
    void setDraft(const QVector<GeoJsonCoord>& coords);

    bool updateVertex(const QString& featureId, int vertexIndex, const GeoJsonCoord& c);

    QVector<const GeoJsonFeature*> visibleFeatures() const;

signals:
    void enabledChanged();
    void toolChanged();
    void drawingChanged();
    void currentFileChanged();
    void lastErrorChanged();
    void documentChanged();
    void selectionChanged();
    void currentFolderChanged();
    void fileLoaded(const QString& path);

private:
    struct Folder;
    static GeoJsonStyle defaultStyleFor(GeoJsonGeometryType t);
    static GeoJsonGeometryType toolToGeomType(Tool t);
    static bool isDrawTool(Tool t);
    static void ensurePolygonClosed(GeoJsonFeature& f);
    void setLastError(QString err);
    void clearLastError();
    void clearSelection();
    void syncModelFromDocument();
    bool removeFeatureById(const QString& id);
    GeoJsonFeature* findFeatureById(const QString& id);
    const GeoJsonFeature* findFeatureById(const QString& id) const;
    GeoJsonFeature* findFeatureByIdGlobal(const QString& id);
    Folder* findFolderById(const QString& id) const;
    void rebuildTreeModel();
    void rebuildTreeNodes(Folder* folder, int depth, QVector<GeoJsonTreeNode>& out) const;
    Folder* addFolderInternal(Folder* parent, const QString& name);
    QString autoFolderName(Folder* parent) const;
    bool folderHasChildren(const Folder* folder) const;

private:
    std::unique_ptr<Folder> root_;
    Folder* currentFolder_{nullptr};
    GeoJsonFeatureModel model_;
    GeoJsonTreeModel treeModel_;
    bool enabled_{false};
    Tool tool_{Select};

    QVector<GeoJsonCoord> draft_;
    bool previewActive_{false};
    GeoJsonCoord preview_{};

    QString currentFile_;
    QString lastError_;

    QString selectedFeatureId_;
    int selectedVertexIndex_{-1};
    QString selectedNodeId_;
    bool selectedNodeIsFolder_{false};
};

#include "geojson_controller.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <vector>

#include "geojson_io.h"

struct GeoJsonController::Folder
{
    QString id;
    QString name;
    bool visible{true};
    GeoJsonDocument doc;
    std::vector<std::unique_ptr<Folder>> children;
    Folder* parent{nullptr};
};

static QString makeFolderId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

GeoJsonController::~GeoJsonController() = default;

GeoJsonController::GeoJsonController(QObject* parent)
    : QObject(parent)
    , model_(this)
    , treeModel_(this)
{
    root_ = std::make_unique<Folder>();
    root_->id = makeFolderId();
    root_->name = QStringLiteral("Root");
    root_->visible = true;

    currentFolder_ = addFolderInternal(root_.get(), QStringLiteral("Folder 1"));
    syncModelFromDocument();
    rebuildTreeModel();
}



QAbstractItemModel* GeoJsonController::featureModel()
{
    return &model_;
}

QAbstractItemModel* GeoJsonController::treeModel()
{
    return &treeModel_;
}

bool GeoJsonController::enabled() const
{
    return enabled_;
}

void GeoJsonController::setEnabled(bool enabled)
{
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    emit enabledChanged();
}

int GeoJsonController::tool() const
{
    return static_cast<int>(tool_);
}

void GeoJsonController::setTool(int tool)
{
    const auto next = static_cast<Tool>(tool);
    if (tool_ == next) {
        return;
    }

    tool_ = next;

    if (!isDrawTool(tool_)) {
        cancelDrawing();
    }

    emit toolChanged();
}

bool GeoJsonController::drawing() const
{
    return isDrawTool(tool_) && !draft_.isEmpty();
}

QString GeoJsonController::currentFile() const
{
    return currentFile_;
}

QString GeoJsonController::lastError() const
{
    return lastError_;
}

QString GeoJsonController::selectedFeatureId() const
{
    return selectedFeatureId_;
}

int GeoJsonController::selectedVertexIndex() const
{
    return selectedVertexIndex_;
}

QString GeoJsonController::currentFolderId() const
{
    return currentFolder_ ? currentFolder_->id : QString();
}

QString GeoJsonController::currentFolderName() const
{
    return currentFolder_ ? currentFolder_->name : QString();
}

QString GeoJsonController::selectedNodeId() const
{
    return selectedNodeId_;
}

bool GeoJsonController::selectedNodeIsFolder() const
{
    return selectedNodeIsFolder_;
}

const GeoJsonDocument& GeoJsonController::document() const
{
    static const GeoJsonDocument empty;
    return currentFolder_ ? currentFolder_->doc : empty;
}

const QVector<GeoJsonCoord>& GeoJsonController::draftCoords() const
{
    return draft_;
}

bool GeoJsonController::previewActive() const
{
    return previewActive_;
}

GeoJsonCoord GeoJsonController::previewCoord() const
{
    return preview_;
}

void GeoJsonController::newDocument()
{
    root_->children.clear();
    currentFolder_ = addFolderInternal(root_.get(), QStringLiteral("Folder 1"));

    model_.setFeatures({});
    currentFile_.clear();
    clearLastError();
    cancelDrawing();
    clearSelection();
    rebuildTreeModel();
    emit currentFileChanged();
    emit documentChanged();
    emit currentFolderChanged();
}

bool GeoJsonController::loadFile(const QString& path)
{
    QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();
    if (ext == QStringLiteral("kgt")) {
        return loadKgt(path);
    }

    return importGeoJsonToFolder(path, currentFolderId());
}

bool GeoJsonController::saveFile(const QString& path)
{
    QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();
    if (ext == QStringLiteral("kgt")) {
        return saveKgt(path);
    }

    return exportFolder(path, currentFolderId());
}

bool GeoJsonController::loadKgt(const QString& path)
{
    clearLastError();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("load: cannot open file"));
        return false;
    }

    const QByteArray bytes = f.readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (doc.isNull() || pe.error != QJsonParseError::NoError) {
        setLastError(QStringLiteral("load: invalid JSON: ") + pe.errorString());
        return false;
    }

    if (!doc.isObject()) {
        setLastError(QStringLiteral("load: root must be object"));
        return false;
    }

    const QJsonObject rootObj = doc.object();
    if (rootObj.value(QStringLiteral("type")).toString() != QStringLiteral("KoggerGeometryTree")) {
        setLastError(QStringLiteral("load: type must be KoggerGeometryTree"));
        return false;
    }

    const QJsonValue foldersVal = rootObj.value(QStringLiteral("folders"));
    if (!foldersVal.isArray()) {
        setLastError(QStringLiteral("load: folders must be array"));
        return false;
    }

    root_->children.clear();
    currentFolder_ = nullptr;

    auto parseFolder = [&](const QJsonObject& fo, Folder* parent, auto&& selfRef) -> Folder* {
        Folder* folder = addFolderInternal(parent, fo.value(QStringLiteral("name")).toString(QStringLiteral("Folder")));
        folder->visible = fo.value(QStringLiteral("visible")).toBool(true);

        const QJsonValue geoVal = fo.value(QStringLiteral("geojson"));
        if (geoVal.isObject()) {
            GeoJsonDocument folderDoc;
            QString err;
            if (GeoJsonIO::parseFeatureCollection(geoVal.toObject(), &folderDoc, &err)) {
                for (auto& ftr : folderDoc.features) {
                    ftr.visible = true;
                }
                folder->doc = std::move(folderDoc);
            }
            else {
                setLastError(QStringLiteral("load: folder geojson invalid: ") + err);
                return static_cast<Folder*>(nullptr);
            }
        }

        const QJsonValue subFoldersVal = fo.value(QStringLiteral("folders"));
        if (subFoldersVal.isArray()) {
            const auto arr = subFoldersVal.toArray();
            for (const auto& sv : arr) {
                if (!sv.isObject()) {
                    continue;
                }
                if (!selfRef(sv.toObject(), folder, selfRef)) {
                    return static_cast<Folder*>(nullptr);
                }
            }
        }
        return folder;
    };

    const QJsonArray foldersArr = foldersVal.toArray();
    for (const auto& fv : foldersArr) {
        if (!fv.isObject()) {
            continue;
        }
        if (!parseFolder(fv.toObject(), root_.get(), parseFolder)) {
            return false;
        }
    }

    if (!root_->children.empty()) {
        currentFolder_ = root_->children.front().get();
    } else {
        currentFolder_ = addFolderInternal(root_.get(), QStringLiteral("Folder 1"));
    }

    currentFile_ = path;
    cancelDrawing();
    clearSelection();
    syncModelFromDocument();
    rebuildTreeModel();
    emit currentFileChanged();
    emit documentChanged();
    emit currentFolderChanged();
    emit fileLoaded(path);
    return true;
}

bool GeoJsonController::saveKgt(const QString& path)
{
    clearLastError();

    QJsonObject rootObj;
    rootObj.insert(QStringLiteral("type"), QStringLiteral("KoggerGeometryTree"));
    rootObj.insert(QStringLiteral("version"), 1);

    auto folderToJson = [&](const Folder* folder, auto&& selfRef) -> QJsonObject {
        QJsonObject fo;
        fo.insert(QStringLiteral("name"), folder->name);
        fo.insert(QStringLiteral("visible"), folder->visible);
        fo.insert(QStringLiteral("geojson"), GeoJsonIO::writeFeatureCollection(folder->doc));

        if (!folder->children.empty()) {
            QJsonArray childArr;
            for (const auto& cptr : folder->children) {
                childArr.append(selfRef(cptr.get(), selfRef));
            }
            fo.insert(QStringLiteral("folders"), childArr);
        }
        return fo;
    };

    QJsonArray folders;
    for (const auto& fptr : root_->children) {
        folders.append(folderToJson(fptr.get(), folderToJson));
    }

    rootObj.insert(QStringLiteral("folders"), folders);

    QJsonDocument outDoc(rootObj);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(QStringLiteral("save: cannot open file for writing"));
        return false;
    }

    const QByteArray bytes = outDoc.toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size()) {
        setLastError(QStringLiteral("save: short write"));
        return false;
    }

    currentFile_ = path;
    emit currentFileChanged();
    return true;
}

bool GeoJsonController::importGeoJsonToFolder(const QString& path, const QString& folderId)
{
    clearLastError();
    Folder* folder = findFolderById(folderId);
    if (!folder) {
        setLastError(QStringLiteral("import: folder not found"));
        return false;
    }

    auto res = GeoJsonIO::loadFromFile(path);
    if (!res.ok) {
        setLastError(res.error);
        return false;
    }

    for (auto& ftr : res.doc.features) {
        ftr.visible = true;
        folder->doc.features.push_back(ftr);
    }

    if (folder == currentFolder_) {
        syncModelFromDocument();
    }

    auto folderDepth = [&](const Folder* f) -> int {
        int depth = 0;
        const Folder* p = f ? f->parent : nullptr;
        while (p && p != root_.get()) {
            ++depth;
            p = p->parent;
        }
        return depth;
    };

    const int depth = folderDepth(folder);
    const int startIndex = folder->doc.features.size() - res.doc.features.size();
    for (int i = 0; i < res.doc.features.size(); ++i) {
        const auto& f = folder->doc.features.at(startIndex + i);
        GeoJsonTreeNode fn;
        fn.id = f.id;
        fn.parentId = folder->id;
        fn.name = QStringLiteral("Feature");
        fn.geomType = GeoJsonFeatureModel::typeToString(f.geomType);
        fn.vertexCount = f.coords.size();
        fn.depth = depth + 1;
        fn.isFolder = false;
        fn.visible = f.visible;
        treeModel_.insertNode(fn);
    }
    treeModel_.updateNodeVertexCount(folder->id, folder->doc.features.size());
    emit documentChanged();
    emit fileLoaded(path);
    return true;
}

bool GeoJsonController::exportFolder(const QString& path, const QString& folderId)
{
    clearLastError();
    Folder* folder = findFolderById(folderId);
    if (!folder) {
        setLastError(QStringLiteral("export: folder not found"));
        return false;
    }

    QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();

    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        if (folderHasChildren(folder)) {
            setLastError(QStringLiteral("export: folder has subfolders, cannot export to GeoJSON"));
            return false;
        }
        QString err;
        if (!GeoJsonIO::saveToFile(path, folder->doc, &err)) {
            setLastError(err);
            return false;
        }
        return true;
    }

    if (ext == QStringLiteral("kgt")) {
        QJsonObject rootObj;
        rootObj.insert(QStringLiteral("type"), QStringLiteral("KoggerGeometryTree"));
        rootObj.insert(QStringLiteral("version"), 1);

        auto folderToJson = [&](const Folder* f, auto&& selfRef) -> QJsonObject {
            QJsonObject fo;
            fo.insert(QStringLiteral("name"), f->name);
            fo.insert(QStringLiteral("visible"), f->visible);
            fo.insert(QStringLiteral("geojson"), GeoJsonIO::writeFeatureCollection(f->doc));
            if (!f->children.empty()) {
                QJsonArray childArr;
                for (const auto& cptr : f->children) {
                    childArr.append(selfRef(cptr.get(), selfRef));
                }
                fo.insert(QStringLiteral("folders"), childArr);
            }
            return fo;
        };

        QJsonArray folders;
        folders.append(folderToJson(folder, folderToJson));
        rootObj.insert(QStringLiteral("folders"), folders);

        QJsonDocument outDoc(rootObj);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setLastError(QStringLiteral("export: cannot open file for writing"));
            return false;
        }
        const QByteArray bytes = outDoc.toJson(QJsonDocument::Indented);
        if (f.write(bytes) != bytes.size()) {
            setLastError(QStringLiteral("export: short write"));
            return false;
        }
        return true;
    }

    setLastError(QStringLiteral("export: unsupported extension"));
    return false;
}

void GeoJsonController::setCurrentFolder(const QString& folderId)
{
    Folder* folder = findFolderById(folderId);
    if (!folder || folder == currentFolder_) {
        return;
    }
    currentFolder_ = folder;
    syncModelFromDocument();
    emit currentFolderChanged();
    emit documentChanged();
}

void GeoJsonController::selectNode(const QString& nodeId, bool isFolder, const QString& parentId)
{
    selectedNodeId_ = nodeId;
    selectedNodeIsFolder_ = isFolder;

    if (isFolder) {
        setCurrentFolder(nodeId);
        selectedFeatureId_.clear();
        selectedVertexIndex_ = -1;
    } else {
        if (!parentId.isEmpty()) {
            setCurrentFolder(parentId);
        }
        selectedFeatureId_ = nodeId;
        selectedVertexIndex_ = -1;
    }

    emit selectionChanged();
}

void GeoJsonController::selectIndex(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    const QVariant idVar = treeModel_.roleData(index, GeoJsonTreeModel::IdRole);
    const QVariant isFolderVar = treeModel_.roleData(index, GeoJsonTreeModel::IsFolderRole);
    const QVariant parentVar = treeModel_.roleData(index, GeoJsonTreeModel::ParentIdRole);
    if (!idVar.isValid() || !isFolderVar.isValid()) {
        return;
    }

    const QString id = idVar.toString();
    const bool isFolder = isFolderVar.toBool();
    const QString parentId = parentVar.isValid() ? parentVar.toString() : QString();

    selectNode(id, isFolder, parentId);
}

void GeoJsonController::addFolderToRoot()
{
    Folder* f = addFolderInternal(root_.get(), autoFolderName(root_.get()));
    if (f) {
        selectNode(f->id, true, QString());
    }
    if (f) {
        GeoJsonTreeNode node;
        node.id = f->id;
        node.parentId = QString();
        node.name = f->name;
        node.geomType = QStringLiteral("Folder");
        node.vertexCount = f->doc.features.size();
        node.depth = 0;
        node.isFolder = true;
        node.visible = f->visible;
        treeModel_.insertNode(node);
    }
}

void GeoJsonController::addFolderToCurrent()
{
    Folder* parent = currentFolder_ ? currentFolder_ : root_.get();
    Folder* f = addFolderInternal(parent, autoFolderName(parent));
    if (f) {
        selectNode(f->id, true, QString());
    }
    if (f) {
        int depth = 0;
        Folder* p = f->parent;
        while (p && p != root_.get()) {
            ++depth;
            p = p->parent;
        }

        GeoJsonTreeNode node;
        node.id = f->id;
        node.parentId = (f->parent && f->parent != root_.get()) ? f->parent->id : QString();
        node.name = f->name;
        node.geomType = QStringLiteral("Folder");
        node.vertexCount = f->doc.features.size();
        node.depth = depth;
        node.isFolder = true;
        node.visible = f->visible;
        treeModel_.insertNode(node);
    }
}

void GeoJsonController::toggleFolderExpanded(const QString& folderId)
{
    Folder* folder = findFolderById(folderId);
    if (!folder) {
        return;
    }
}

void GeoJsonController::setNodeVisible(const QString& nodeId, bool isFolder, bool visible)
{
    if (isFolder) {
        Folder* folder = findFolderById(nodeId);
        if (!folder) {
            return;
        }
        folder->visible = visible;
    } else {
        GeoJsonFeature* f = findFeatureByIdGlobal(nodeId);
        if (!f) {
            return;
        }
        f->visible = visible;
    }
    treeModel_.updateNodeVisible(nodeId, visible);
    emit documentChanged();
}

void GeoJsonController::finishDrawing()
{
    if (!isDrawTool(tool_) || draft_.isEmpty()) {
        return;
    }
    if (!currentFolder_) {
        return;
    }

    const auto geomType = toolToGeomType(tool_);

    if (geomType == GeoJsonGeometryType::LineString && draft_.size() < 2) {
        return;
    }
    if (geomType == GeoJsonGeometryType::Polygon && draft_.size() < 3) {
        return;
    }

    GeoJsonFeature f;
    f.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    f.geomType = geomType;
    f.coords = draft_;
    f.visible = true;
    f.style = defaultStyleFor(geomType);

    if (geomType == GeoJsonGeometryType::Polygon) {
        ensurePolygonClosed(f);
    }

    currentFolder_->doc.features.push_back(f);
    if (currentFolder_ == findFolderById(currentFolder_->id)) {
        model_.upsertFeature(f);
    }

    cancelDrawing();
    clearSelection();
    int depth = 0;
    Folder* p = currentFolder_->parent;
    while (p && p != root_.get()) {
        ++depth;
        p = p->parent;
    }

    GeoJsonTreeNode fn;
    fn.id = f.id;
    fn.parentId = currentFolder_->id;
    fn.name = QStringLiteral("Feature");
    fn.geomType = GeoJsonFeatureModel::typeToString(f.geomType);
    fn.vertexCount = f.coords.size();
    fn.depth = depth + 1;
    fn.isFolder = false;
    fn.visible = f.visible;
    treeModel_.insertNode(fn);
    treeModel_.updateNodeVertexCount(currentFolder_->id, currentFolder_->doc.features.size());
    emit documentChanged();
}

void GeoJsonController::cancelDrawing()
{
    const bool hadDrawing = drawing() || previewActive_;
    draft_.clear();
    previewActive_ = false;

    if (hadDrawing) {
        emit drawingChanged();
        emit documentChanged();
    }
}

void GeoJsonController::undoLastVertex()
{
    if (!isDrawTool(tool_) || draft_.isEmpty()) {
        return;
    }
    draft_.removeLast();
    emit drawingChanged();
    emit documentChanged();
}

void GeoJsonController::deleteSelectedFeature()
{
    if (selectedFeatureId_.isEmpty()) {
        return;
    }
    if (!removeFeatureById(selectedFeatureId_)) {
        return;
    }
    const QString removedId = selectedFeatureId_;
    clearSelection();
    syncModelFromDocument();
    treeModel_.removeNode(removedId);
    if (currentFolder_) {
        treeModel_.updateNodeVertexCount(currentFolder_->id, currentFolder_->doc.features.size());
    }
    emit documentChanged();
}

void GeoJsonController::selectFeature(const QString& id)
{
    if (id == selectedFeatureId_) {
        return;
    }
    selectedFeatureId_ = id;
    selectedNodeId_ = id;
    selectedNodeIsFolder_ = false;
    selectedVertexIndex_ = -1;
    emit selectionChanged();
}

void GeoJsonController::selectVertex(const QString& featureId, int vertexIndex)
{
    if (featureId == selectedFeatureId_ && vertexIndex == selectedVertexIndex_) {
        return;
    }
    selectedFeatureId_ = featureId;
    selectedNodeId_ = featureId;
    selectedNodeIsFolder_ = false;
    selectedVertexIndex_ = vertexIndex;
    emit selectionChanged();
}

void GeoJsonController::setPreview(const GeoJsonCoord& c)
{
    if (!isDrawTool(tool_)) {
        return;
    }
    preview_ = c;
    previewActive_ = true;
    emit documentChanged();
}

void GeoJsonController::clearPreview()
{
    if (!previewActive_) {
        return;
    }
    previewActive_ = false;
    emit documentChanged();
}

void GeoJsonController::addDraftVertex(const GeoJsonCoord& c)
{
    if (!isDrawTool(tool_)) {
        return;
    }

    draft_.push_back(c);
    emit drawingChanged();
    emit documentChanged();
}

void GeoJsonController::setDraft(const QVector<GeoJsonCoord>& coords)
{
    if (draft_ == coords) {
        return;
    }
    draft_ = coords;
    emit drawingChanged();
    emit documentChanged();
}

bool GeoJsonController::updateVertex(const QString& featureId, int vertexIndex, const GeoJsonCoord& c)
{
    auto* f = findFeatureById(featureId);
    if (!f) {
        return false;
    }
    if (vertexIndex < 0 || vertexIndex >= f->coords.size()) {
        return false;
    }

    const bool keepHasZ = f->coords[vertexIndex].hasZ;
    f->coords[vertexIndex] = c;
    if (!keepHasZ) {
        f->coords[vertexIndex].hasZ = false;
        f->coords[vertexIndex].z = 0.0;
    }
    if (f->geomType == GeoJsonGeometryType::Polygon) {
        ensurePolygonClosed(*f);
    }
    model_.upsertFeature(*f);
    emit documentChanged();
    return true;
}

QVector<const GeoJsonFeature*> GeoJsonController::visibleFeatures() const
{
    QVector<const GeoJsonFeature*> out;
    if (!root_) {
        return out;
    }

    struct StackItem {
        const Folder* folder;
        bool parentVisible;
    };

    QVector<StackItem> stack;
    for (const auto& fptr : root_->children) {
        stack.push_back({fptr.get(), true});
    }

    while (!stack.isEmpty()) {
        const auto it = stack.takeLast();
        const Folder* folder = it.folder;
        const bool folderVisible = it.parentVisible && folder->visible;

        if (folderVisible) {
            for (const auto& f : folder->doc.features) {
                if (f.visible) {
                    out.push_back(&f);
                }
            }
        }

        for (const auto& cptr : folder->children) {
            stack.push_back({cptr.get(), folderVisible});
        }
    }

    return out;
}

GeoJsonStyle GeoJsonController::defaultStyleFor(GeoJsonGeometryType t)
{
    GeoJsonStyle s;
    switch (t) {
    case GeoJsonGeometryType::Point:
        s.markerColor = QColor(QStringLiteral("#ff3b30"));
        s.markerSizePx = 11.0;
        break;
    case GeoJsonGeometryType::LineString:
        s.stroke = QColor(QStringLiteral("#00bcd4"));
        s.strokeWidthPx = 3.0;
        s.strokeOpacity = 1.0;
        break;
    case GeoJsonGeometryType::Polygon:
        s.stroke = QColor(QStringLiteral("#00c853"));
        s.strokeWidthPx = 2.0;
        s.strokeOpacity = 1.0;
        s.fill = QColor(QStringLiteral("#00c853"));
        s.fillOpacity = 0.25;
        break;
    }
    return s;
}

GeoJsonGeometryType GeoJsonController::toolToGeomType(Tool t)
{
    switch (t) {
    case DrawPoint:   return GeoJsonGeometryType::Point;
    case DrawLine:    return GeoJsonGeometryType::LineString;
    case DrawPolygon: return GeoJsonGeometryType::Polygon;
    default:          return GeoJsonGeometryType::Point;
    }
}

bool GeoJsonController::isDrawTool(Tool t)
{
    return t == DrawPoint || t == DrawLine || t == DrawPolygon;
}

void GeoJsonController::ensurePolygonClosed(GeoJsonFeature& f)
{
    if (f.geomType != GeoJsonGeometryType::Polygon) {
        return;
    }
    if (f.coords.size() < 3) {
        return;
    }

    const auto& first = f.coords.first();
    const auto& last = f.coords.last();
    const bool sameLon = qFuzzyCompare(1.0 + first.lon, 1.0 + last.lon);
    const bool sameLat = qFuzzyCompare(1.0 + first.lat, 1.0 + last.lat);
    const bool sameZ = (!first.hasZ && !last.hasZ) || (first.hasZ && last.hasZ && qFuzzyCompare(1.0 + first.z, 1.0 + last.z));

    if (!(sameLon && sameLat && sameZ)) {
        f.coords.push_back(first);
    } else if (f.coords.size() >= 4) {
        f.coords.last() = first;
    }
}

void GeoJsonController::setLastError(QString err)
{
    if (lastError_ == err) {
        return;
    }
    lastError_ = std::move(err);
    emit lastErrorChanged();
}

void GeoJsonController::clearLastError()
{
    if (lastError_.isEmpty()) {
        return;
    }
    lastError_.clear();
    emit lastErrorChanged();
}

void GeoJsonController::clearSelection()
{
    if (selectedFeatureId_.isEmpty() && selectedVertexIndex_ == -1 && selectedNodeId_.isEmpty()) {
        return;
    }
    selectedFeatureId_.clear();
    selectedNodeId_.clear();
    selectedNodeIsFolder_ = false;
    selectedVertexIndex_ = -1;
    emit selectionChanged();
}

void GeoJsonController::syncModelFromDocument()
{
    if (!currentFolder_) {
        model_.setFeatures({});
        return;
    }
    model_.setFeatures(currentFolder_->doc.features);
}

bool GeoJsonController::removeFeatureById(const QString& id)
{
    if (!currentFolder_) {
        return false;
    }
    for (int i = 0; i < currentFolder_->doc.features.size(); ++i) {
        if (currentFolder_->doc.features.at(i).id == id) {
            currentFolder_->doc.features.removeAt(i);
            model_.removeById(id);
            return true;
        }
    }
    return false;
}

GeoJsonFeature* GeoJsonController::findFeatureById(const QString& id)
{
    if (!currentFolder_) {
        return nullptr;
    }
    for (auto& f : currentFolder_->doc.features) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

GeoJsonFeature* GeoJsonController::findFeatureByIdGlobal(const QString& id)
{
    if (!root_) {
        return nullptr;
    }

    QVector<Folder*> stack;
    for (const auto& fptr : root_->children) {
        stack.push_back(fptr.get());
    }

    while (!stack.isEmpty()) {
        Folder* f = stack.takeLast();
        for (auto& feat : f->doc.features) {
            if (feat.id == id) {
                return &feat;
            }
        }
        for (const auto& cptr : f->children) {
            stack.push_back(cptr.get());
        }
    }

    return nullptr;
}

const GeoJsonFeature* GeoJsonController::findFeatureById(const QString& id) const
{
    if (!currentFolder_) {
        return nullptr;
    }
    for (const auto& f : currentFolder_->doc.features) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

GeoJsonController::Folder* GeoJsonController::findFolderById(const QString& id) const
{
    if (!root_) {
        return nullptr;
    }
    if (root_->id == id) {
        return root_.get();
    }

    QVector<Folder*> stack;
    for (const auto& fptr : root_->children) {
        stack.push_back(fptr.get());
    }

    while (!stack.isEmpty()) {
        Folder* f = stack.takeLast();
        if (f->id == id) {
            return f;
        }
        for (const auto& cptr : f->children) {
            stack.push_back(cptr.get());
        }
    }
    return nullptr;
}

void GeoJsonController::rebuildTreeModel()
{
    QVector<GeoJsonTreeNode> nodes;
    if (root_) {
        for (const auto& fptr : root_->children) {
            rebuildTreeNodes(fptr.get(), 0, nodes);
        }
    }
    treeModel_.setNodes(std::move(nodes));
}

void GeoJsonController::rebuildTreeNodes(Folder* folder, int depth, QVector<GeoJsonTreeNode>& out) const
{
    if (!folder) {
        return;
    }

    GeoJsonTreeNode node;
    node.id = folder->id;
    node.parentId = folder->parent ? folder->parent->id : QString();
    node.name = folder->name;
    node.geomType = QStringLiteral("Folder");
    node.vertexCount = folder->doc.features.size();
    node.depth = depth;
    node.isFolder = true;
    node.visible = folder->visible;
    out.push_back(node);

    for (const auto& f : folder->doc.features) {
        GeoJsonTreeNode fn;
        fn.id = f.id;
        fn.parentId = folder->id;
        fn.name = QStringLiteral("Feature");
        fn.geomType = GeoJsonFeatureModel::typeToString(f.geomType);
        fn.vertexCount = f.coords.size();
        fn.depth = depth + 1;
        fn.isFolder = false;
        fn.visible = f.visible;
        out.push_back(fn);
    }

    for (const auto& cptr : folder->children) {
        rebuildTreeNodes(cptr.get(), depth + 1, out);
    }
}

GeoJsonController::Folder* GeoJsonController::addFolderInternal(Folder* parent, const QString& name)
{
    if (!parent) {
        return nullptr;
    }
    auto f = std::make_unique<Folder>();
    f->id = makeFolderId();
    f->name = name;
    f->visible = true;
    // f->expanded = true;
    f->parent = parent;
    Folder* raw = f.get();
    parent->children.push_back(std::move(f));
    return raw;
}

QString GeoJsonController::autoFolderName(Folder* parent) const
{
    int idx = 1;
    while (true) {
        const QString name = QStringLiteral("Folder %1").arg(idx);
        bool exists = false;
        if (parent) {
            for (const auto& cptr : parent->children) {
                if (cptr->name == name) {
                    exists = true;
                    break;
                }
            }
        }
        if (!exists) {
            return name;
        }
        ++idx;
    }
}

bool GeoJsonController::folderHasChildren(const Folder* folder) const
{
    return folder && !folder->children.empty();
}

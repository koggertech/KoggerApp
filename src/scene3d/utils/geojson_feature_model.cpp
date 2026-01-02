#include "geojson_feature_model.h"

GeoJsonFeatureModel::GeoJsonFeatureModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int GeoJsonFeatureModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return features_.size();
}

QVariant GeoJsonFeatureModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= features_.size()) {
        return {};
    }
    const auto& f = features_.at(index.row());
    switch (role) {
    case IdRole:          return f.id;
    case NameRole:        return f.name;
    case TypeRole:        return typeToString(f.geomType);
    case VertexCountRole: return f.coords.size();
    case CoordsRole: {
        QVariantList out;
        out.reserve(f.coords.size());
        for (const auto& c : f.coords) {
            QVariantMap m;
            m.insert(QStringLiteral("lat"), c.lat);
            m.insert(QStringLiteral("lon"), c.lon);
            m.insert(QStringLiteral("z"), c.hasZ ? c.z : 0.0);
            m.insert(QStringLiteral("hasZ"), c.hasZ);
            out.append(m);
        }
        return out;
    }
    case StrokeRole:      return f.style.stroke;
    case StrokeWidthRole: return f.style.strokeWidthPx;
    case StrokeOpacityRole: return f.style.strokeOpacity;
    case FillRole:        return f.style.fill;
    case FillOpacityRole: return f.style.fillOpacity;
    case MarkerColorRole: return f.style.markerColor;
    case MarkerSizeRole:  return f.style.markerSizePx;
    default:              return {};
    }
}

QHash<int, QByteArray> GeoJsonFeatureModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {TypeRole, "geomType"},
        {VertexCountRole, "vertexCount"},
        {CoordsRole, "coords"},
        {StrokeRole, "stroke"},
        {StrokeWidthRole, "strokeWidth"},
        {StrokeOpacityRole, "strokeOpacity"},
        {FillRole, "fill"},
        {FillOpacityRole, "fillOpacity"},
        {MarkerColorRole, "markerColor"},
        {MarkerSizeRole, "markerSize"},
    };
}

const QVector<GeoJsonFeature>& GeoJsonFeatureModel::features() const
{
    return features_;
}

void GeoJsonFeatureModel::setFeatures(QVector<GeoJsonFeature> features)
{
    beginResetModel();
    features_ = std::move(features);
    endResetModel();
}

void GeoJsonFeatureModel::upsertFeature(const GeoJsonFeature& feature)
{
    for (int i = 0; i < features_.size(); ++i) {
        if (features_.at(i).id == feature.id) {
            features_[i] = feature;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }

    beginInsertRows(QModelIndex(), features_.size(), features_.size());
    features_.push_back(feature);
    endInsertRows();
}

bool GeoJsonFeatureModel::removeById(const QString& id)
{
    for (int i = 0; i < features_.size(); ++i) {
        if (features_.at(i).id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            features_.removeAt(i);
            endRemoveRows();
            return true;
        }
    }
    return false;
}

GeoJsonFeature* GeoJsonFeatureModel::findById(const QString& id)
{
    for (auto& f : features_) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

const GeoJsonFeature* GeoJsonFeatureModel::findById(const QString& id) const
{
    for (const auto& f : features_) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

QString GeoJsonFeatureModel::typeToString(GeoJsonGeometryType t)
{
    switch (t) {
    case GeoJsonGeometryType::Point:      return QStringLiteral("Point");
    case GeoJsonGeometryType::LineString: return QStringLiteral("LineString");
    case GeoJsonGeometryType::Polygon:    return QStringLiteral("Polygon");
    }
    return QStringLiteral("Unknown");
}

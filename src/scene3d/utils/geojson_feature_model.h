#pragma once

#include <QAbstractListModel>

#include "geojson_defs.h"

class GeoJsonFeatureModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        VertexCountRole,
        CoordsRole,
        StrokeRole,
        StrokeWidthRole,
        StrokeOpacityRole,
        FillRole,
        FillOpacityRole,
        MarkerColorRole,
        MarkerSizeRole,
    };

    explicit GeoJsonFeatureModel(QObject* parent = nullptr);

    static QString typeToString(GeoJsonGeometryType t);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<GeoJsonFeature>& features() const;
    void setFeatures(QVector<GeoJsonFeature> features);
    void upsertFeature(const GeoJsonFeature& feature);
    bool removeById(const QString& id);
    GeoJsonFeature* findById(const QString& id);
    const GeoJsonFeature* findById(const QString& id) const;

private:
    QVector<GeoJsonFeature> features_;
};

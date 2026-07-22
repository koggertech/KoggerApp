#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QPointF>
#include <QElapsedTimer>

#include "geojson_defs.h"
#include "dataset_defs.h"

class GraphicsScene3dView;
class RulerTool;

class RulerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool drawing READ drawing NOTIFY stateChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY stateChanged)
    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY stateChanged)

public:
    explicit RulerController(GraphicsScene3dView* view, RulerTool* tool, QObject* parent = nullptr);

    bool enabled() const;
    bool drawing() const;
    bool selected() const;
    bool hasGeometry() const;

    void setEnabled(bool enabled);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void finishDrawing();
    Q_INVOKABLE void cancelDrawing();
    Q_INVOKABLE void deleteSelected();

    void onLeftClick(qreal x, qreal y);
    void onMouseMove(Qt::MouseButtons button, qreal x, qreal y);
    bool onKey(Qt::Key key);
    void onPointerCanceled();
    void rebuildIfNeeded();
    bool pick(qreal x, qreal y) const;

signals:
    void enabledChanged();
    void stateChanged();

private:
    void setDrawing(bool drawing);
    void setSelected(bool selected);
    void resetInteraction();
    void addPoint(const QVector3D& p);
    void rebuildCommitted();
    void updateGeometry();
    void buildSegmentStrips(const GeoJsonCoord& a,
                            const GeoJsonCoord& b,
                            bool perspective,
                            QVector<QVector<QVector3D>>& strips,
                            QVector<QVector3D>& mids,
                            QVector<double>& segMeters) const;
    QVector3D toScene(double latDeg, double lonDeg) const;
    double geoDistanceMeters(const GeoJsonCoord& a, const GeoJsonCoord& b) const;

    GraphicsScene3dView* view_{nullptr};
    RulerTool* tool_{nullptr};

    bool enabled_{false};
    bool drawing_{false};
    bool selected_{false};

    QVector<GeoJsonCoord> geoPoints_;
    GeoJsonCoord previewGeo_;
    bool previewActive_{false};

    LLARef lastViewRef_;
    bool lastPerspective_{false};

    QVector<QVector3D> committedVertices_;
    QVector<QVector<QVector3D>> committedStrips_;
    QVector<QVector3D> committedMids_;
    QVector<double> committedSeg_;

    QElapsedTimer lastClickTimer_;
    QPointF lastClickPos_{0.0, 0.0};
    bool hasLastClick_{false};
};

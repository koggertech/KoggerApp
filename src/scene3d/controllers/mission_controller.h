#pragma once

#include <optional>

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <QTimer>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

#include "dataset_defs.h"
#include "mission_defs.h"
#include "mission_model.h"

class GraphicsScene3dView;

#include "mission_layer.h"

namespace mission {
class MissionPlanController;
struct GeneratedPath;
}

class MissionController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    editing        READ editing                          NOTIFY editingChanged)
    Q_PROPERTY(int     tool           READ tool         WRITE setTool         NOTIFY toolChanged)
    Q_PROPERTY(QString selectedId     READ selectedId                       NOTIFY selectionChanged)
    Q_PROPERTY(int     selectedVertex READ selectedVertex                   NOTIFY selectionChanged)
    Q_PROPERTY(bool    dragging       READ dragging                         NOTIFY draggingChanged)
    Q_PROPERTY(int     draftCount     READ draftCount                       NOTIFY draftChanged)
    Q_PROPERTY(bool    draftReady     READ draftReady                       NOTIFY draftChanged)

public:
    enum Tool : int
    {
        ToolNone = 0,
        ToolHome = 1,
        ToolWaypoint = 2,
        ToolSurvey = 3,
        ToolCorridor = 4,
        ToolRally = 5,
        ToolSurveyTrace = 6,
        ToolCorridorTrace = 7
    };
    Q_ENUM(Tool)

    MissionController(GraphicsScene3dView* view, MissionLayer* layer, QObject* parent = nullptr);

    void setPlan(mission::MissionPlanController* plan);
    mission::MissionPlanController* plan() const { return plan_; }

    bool editing() const { return editing_; }
    void setEditing(bool editing);
    int tool() const { return tool_; }
    void setTool(int tool);
    QString selectedId() const { return selectedId_; }
    int selectedVertex() const { return selectedVertex_; }
    bool dragging() const { return dragging_; }
    int draftCount() const { return draft_.size(); }
    bool draftReady() const;

    bool onPress(qreal x, qreal y);
    void onDrag(const QVector3D& scenePoint);
    void onRelease(const QVector3D& scenePoint, bool hasPoint, bool wasMoved);
    bool onKey(Qt::Key key);
    void onPointerCanceled();
    void rebuildIfNeeded();
    bool sceneBounds(QVector3D& minOut, QVector3D& maxOut) const;
    bool itemBounds(const QString& id, QVector3D& minOut, QVector3D& maxOut) const;

    Q_INVOKABLE void select(const QString& id, int vertex = -1);
    Q_INVOKABLE void showItem(const QString& id);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void placeSurveyTemplate();
    Q_INVOKABLE void placeSurveyCircle();
    Q_INVOKABLE void placeCorridorTemplate();
    Q_INVOKABLE void fitToPlan();
    Q_INVOKABLE void finishDraft();
    Q_INVOKABLE void cancelDraft();
    Q_INVOKABLE void undoDraftVertex();

signals:
    void editingChanged();
    void toolChanged();
    void selectionChanged();
    void draggingChanged();
    void draftChanged();

private:
    enum class HitKind : int
    {
        None,
        Vertex,
        Midpoint,
        Center,
        DeleteVertex,
        RouteMidpoint,
        Body
    };

    struct Hit
    {
        HitKind kind = HitKind::None;
        QString id;
        int index = -1;
        int insertIndex = -1;
        QVector3D world;
    };

    struct Anchors
    {
        bool valid = false;
        QVector3D mid;
    };

    struct ShapeHandles
    {
        bool valid = false;
        bool closed = false;
        QVector<mission::GeoPoint> verts;
        QVector<QVector3D> scene;
        QVector<QVector3D> midpoints;
        QVector3D center;
        QVector2D centerOffset;
    };

    static QVector<mission::GeoPoint> shapeVertices(const mission::MissionItem& item);
    ShapeHandles selectedShapeHandles() const;
    QVector<Anchors> routeAnchors() const;
    std::optional<QVector3D> selectedPointWorld() const;
    void appendGeodesic(MissionLayer::RenderData& rd, const mission::GeoPoint& a, const mission::GeoPoint& b, const QColor& color, float widthPx, bool arrow) const;
    void appendGeneratedMarkers(MissionLayer::RenderData& rd, const mission::GeneratedPath& g, int ordinal, const QColor& color) const;
    Hit hitTest(qreal x, qreal y) const;
    QPointF toScreen(const QVector3D& world) const;
    QVector3D toScene(const mission::GeoPoint& p) const;
    mission::GeoPoint toGeo(const QVector3D& p) const;
    QVector<mission::GeoPoint> itemVertices(const QString& id) const;
    bool itemHasVertices(const QString& id) const;
    QString itemType(const QString& id) const;
    double hitRadius() const;
    void beginDrag(const Hit& hit, const QVector3D& scenePoint);
    void endDrag();
    void rebuild();
    void markDirty();
    void setDragging(bool dragging);
    bool viewExtent(QVector3D& center, float& halfX, float& halfY) const;
    void addSurveyShape(const QVector<mission::GeoPoint>& polygon, double spacing);

    GraphicsScene3dView* view_{nullptr};
    MissionLayer* layer_{nullptr};
    mission::MissionPlanController* plan_{nullptr};

    bool editing_{false};
    bool dirty_{true};
    int tool_{ToolNone};
    QString selectedId_;
    int selectedVertex_{-1};

    QVector<mission::GeoPoint> draft_;
    bool dragging_{false};
    bool dragMoved_{false};
    Hit dragHit_;
    QVector3D dragStartScene_;
    QVector<QVector3D> dragStartVertices_;

    LLARef lastViewRef_;
    bool lastPerspective_{false};

    QString flashId_;
    QElapsedTimer flashClock_;
    QTimer flashTimer_;
};

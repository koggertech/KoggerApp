#pragma once

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QVector3D>

#include "dataset_defs.h"
#include "mission_defs.h"
#include "mission_layer.h"
#include "mission_model.h"

class GraphicsScene3dView;

namespace mission {
class MissionRunTracker;
}

/**
 * The "Mission" 3D layer: a read-only picture of the mission on the vehicle
 * (MissionRunTracker snapshot) — legs already flown dimmed, the current one highlighted, the
 * rest ahead, a line from the vehicle to its target, the vehicle home "H", fence outlines and
 * rally points. Drawn into its own MissionLayer while layerVisible and the editor is closed.
 * A tap on a navigation point selects it (selectedSeq, with its screen position kept up to date
 * for the QML card that offers "Go here").
 */
class MissionRunController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    layerVisible     READ layerVisible WRITE setLayerVisible NOTIFY layerVisibleChanged)
    Q_PROPERTY(bool    shown            READ shown                              NOTIFY shownChanged)
    /** What the layer draws besides the route: fence outlines, rally points, the vehicle home, the vehicle → target line, point labels. */
    Q_PROPERTY(bool    showFence        READ showFence    WRITE setShowFence    NOTIFY drawOptionsChanged)
    Q_PROPERTY(bool    showRally        READ showRally    WRITE setShowRally    NOTIFY drawOptionsChanged)
    Q_PROPERTY(bool    showHome         READ showHome     WRITE setShowHome     NOTIFY drawOptionsChanged)
    Q_PROPERTY(bool    showVehicleLine  READ showVehicleLine WRITE setShowVehicleLine NOTIFY drawOptionsChanged)
    Q_PROPERTY(bool    showLabels       READ showLabels   WRITE setShowLabels   NOTIFY drawOptionsChanged)
    /** Route look: legs ahead, the current leg (also the target ring and the line to it), legs flown — opaque colours, the layer applies its own transparency per role; widths in px (0.5…12). */
    Q_PROPERTY(QColor  aheadColor       READ aheadColor   WRITE setAheadColor   NOTIFY drawOptionsChanged)
    Q_PROPERTY(QColor  currentColor     READ currentColor WRITE setCurrentColor NOTIFY drawOptionsChanged)
    Q_PROPERTY(QColor  doneColor        READ doneColor    WRITE setDoneColor    NOTIFY drawOptionsChanged)
    Q_PROPERTY(double  routeWidth       READ routeWidth   WRITE setRouteWidth   NOTIFY drawOptionsChanged)
    Q_PROPERTY(double  currentWidth     READ currentWidth WRITE setCurrentWidth NOTIFY drawOptionsChanged)
    Q_PROPERTY(int     selectedSeq      READ selectedSeq                        NOTIFY selectionChanged)
    Q_PROPERTY(int     selectedJumpSeq  READ selectedJumpSeq                    NOTIFY selectionChanged)
    Q_PROPERTY(int     selectedNav      READ selectedNav                        NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedLabel    READ selectedLabel                      NOTIFY selectionChanged)
    Q_PROPERTY(QPointF selectedScreen   READ selectedScreen                     NOTIFY selectedScreenChanged)
    Q_PROPERTY(bool    selectedOnScreen READ selectedOnScreen                   NOTIFY selectedScreenChanged)

public:
    MissionRunController(GraphicsScene3dView* view, MissionLayer* layer, QObject* parent = nullptr);

    void setTracker(mission::MissionRunTracker* tracker);
    void setEditorActive(bool active);

    bool layerVisible() const { return layerVisible_; }
    void setLayerVisible(bool visible);
    bool shown() const;
    bool showFence() const { return showFence_; }
    bool showRally() const { return showRally_; }
    bool showHome() const { return showHome_; }
    bool showVehicleLine() const { return showVehicleLine_; }
    bool showLabels() const { return showLabels_; }
    void setShowFence(bool show);
    void setShowRally(bool show);
    void setShowHome(bool show);
    void setShowVehicleLine(bool show);
    void setShowLabels(bool show);
    QColor aheadColor() const { return aheadColor_; }
    QColor currentColor() const { return currentColor_; }
    QColor doneColor() const { return doneColor_; }
    double routeWidth() const { return routeWidth_; }
    double currentWidth() const { return currentWidth_; }
    void setAheadColor(const QColor& color);
    void setCurrentColor(const QColor& color);
    void setDoneColor(const QColor& color);
    void setRouteWidth(double width);
    void setCurrentWidth(double width);
    int selectedSeq() const { return selectedSeq_; }
    int selectedJumpSeq() const;
    int selectedNav() const;
    QString selectedLabel() const;
    QPointF selectedScreen() const { return selectedScreen_; }
    bool selectedOnScreen() const { return selectedOnScreen_; }

    /** Selects the navigation point under (x, y); false when nothing was hit (the selection is then cleared). */
    bool onTap(qreal x, qreal y);
    void rebuildIfNeeded();

    Q_INVOKABLE void clearSelection();

signals:
    void layerVisibleChanged();
    void drawOptionsChanged();
    void shownChanged();
    void selectionChanged();
    void selectedScreenChanged();

private:
    void rebuild();
    void markDirty();
    void setDrawOption(bool& option, bool value);
    void setColorOption(QColor& option, const QColor& value);
    void setWidthOption(double& option, double value);
    void updateShown();
    void updateSelectedScreen();
    void onSnapshotChanged();
    void appendLeg(MissionLayer::RenderData& rd, const mission::GeoPoint& a, const mission::GeoPoint& b, const QColor& color, float widthPx, bool arrow) const;
    QVector3D toScene(const mission::GeoPoint& p) const;
    QPointF toScreen(const QVector3D& world) const;

    GraphicsScene3dView* view_{nullptr};
    MissionLayer* layer_{nullptr};
    mission::MissionRunTracker* tracker_{nullptr};
    bool layerVisible_{true};
    bool showFence_{true};
    bool showRally_{true};
    bool showHome_{true};
    bool showVehicleLine_{true};
    bool showLabels_{true};
    QColor aheadColor_{59, 130, 246};
    QColor currentColor_{250, 204, 21};
    QColor doneColor_{148, 163, 184};
    double routeWidth_{2.5};
    double currentWidth_{3.5};
    bool editorActive_{false};
    bool shown_{false};
    bool dirty_{true};
    LLARef lastViewRef_;
    bool lastPerspective_{true};
    int selectedSeq_{-1};
    QPointF selectedScreen_;
    bool selectedOnScreen_{false};
    bool screenNotifyQueued_{false};
    QVector<mission::FencePolygon> fence_;
    QVector<mission::GeoPoint> rally_;
};

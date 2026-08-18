#pragma once

#include <QColor>
#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QVector>
#include <QVector3D>

#include "dataset_defs.h"
#include "id_binnary.h"
#include "usbl_layer.h"

class GraphicsScene3dView;
class UsblLayer;

// Beacons and the acoustic head on the map: what is remembered, and where it is drawn.
//
// WHY THE HISTORY IS GEO AND NOT SCENE COORDINATES. The scene is NED against the CAMERA's
// reference (`viewLlaRef_`) under the current projection mode, and both of those move: the frame
// is rebased after ~5 km of travel, and the NED conversion itself differs between perspective and
// ortho. A stored QVector3D silently means something different afterwards. RulerController and
// GeoJsonController hold geo for exactly this reason; so does this. `rebuildIfNeeded` is the
// other half of the deal -- it re-projects only when one of those two actually changed, which is
// what keeps an unbounded history affordable: between rebuilds the renderer's per-frame copy of
// the vertex vectors is a refcount bump, not a copy.
//
// WHAT DECIDES WHICH BEACONS APPEAR is the interrogation plan, and the plan lives in QML
// (UsblPlanStore, whose rules are pure JS asserted by tools/qml_test). So this accumulates for
// every address that answers and PRESENTS only what `setNodes` names. A beacon that answers
// before its node exists therefore already has a track the moment the node is added -- the
// accumulation costs nothing extra, since it is keyed by address either way.
//
// THE COLOUR ARRIVES FROM QML for the same reason: UsblFieldLogic.ADDRESS_COLORS is the one table
// that ties a row in the settings pane to a row in the on-scene panel to a ball on the map, and a
// second copy down here is a second thing to forget to change.
class UsblLayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit UsblLayerController(GraphicsScene3dView* view, UsblLayer* layer, QObject* parent = nullptr);

    bool enabled() const;
    void setEnabled(bool enabled);

    // [{ addr: int, active: bool, color: string }] in the plan's own order. Total: an address
    // absent from the list stops being drawn, which is what removing a node has to mean.
    Q_INVOKABLE void setNodes(const QVariantList& nodes);
    Q_INVOKABLE void clear();

    void rebuildIfNeeded();

public Q_SLOTS:
    void onUsblSolution(const IDBinUsblSolution::UsblSolution& solution);

Q_SIGNALS:
    void enabledChanged();

private:
    struct Fix
    {
        double lat{0.0};
        double lon{0.0};
        float depth{NAN};
    };

    struct Node
    {
        QColor color;
        bool active{true};
    };

    // Full re-projection of every remembered fix. Only for the events that invalidate the whole
    // frame -- a camera rebase, a projection switch, a new plan. A new fix appends instead.
    void rebuild();
    void push();
    // The surface point (z pinned to 0) for one geo position.
    QVector3D toSurface(double latDeg, double lonDeg) const;
    static void applyFix(UsblLayer::Beacon& beacon, const QVector3D& surface, float depth);

    GraphicsScene3dView* view_{nullptr};
    UsblLayer* layer_{nullptr};

    bool enabled_{true};

    QHash<int, QVector<Fix>> beacons_;
    QVector<Fix> head_;
    float headYaw_{NAN};

    QHash<int, Node> nodes_;
    QVector<int> order_;

    // What the layer currently holds, kept here so a new fix is one appended point rather than a
    // re-projection of the history behind it.
    UsblLayer::RenderData scene_;
    QHash<int, int> sceneIndex_;

    LLARef lastViewRef_;
    bool lastPerspective_{false};
};

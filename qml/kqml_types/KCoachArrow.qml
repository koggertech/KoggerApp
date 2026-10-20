import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

Item {
    id: root

    property string direction: "down"
    property real tipX: 0
    property real tipY: 0
    property real length: Math.round(44 * AppPalette.scale)
    property bool bounce: true
    property color color: AppPalette.accent
    property color outlineColor: AppPalette.card

    readonly property real _headH: Math.round(root.length * 0.42)
    readonly property real _shaft: Math.max(4, Math.round(root.length * 0.2))
    readonly property real _amplitude: Math.round(8 * AppPalette.scale)
    readonly property real travel: root.bounce ? root._amplitude : 0
    readonly property var _dir: ({
        "down":  Qt.point(0, 1),
        "up":    Qt.point(0, -1),
        "left":  Qt.point(-1, 0),
        "right": Qt.point(1, 0)
    })[root.direction] || Qt.point(0, 1)
    property real _bounceOffset: 0
    readonly property real _round: Math.max(2, Math.round(root.length * 0.08))
    readonly property real _outline: Math.max(1, Math.round(2 * AppPalette.scale))
    readonly property string _path: {
        const r = root._round
        const cx = root.width / 2
        const half = Math.max(0.5, root._shaft / 2 - r)
        const baseY = root.height - root._headH
        const tipY = root.height - r
        const p = function (x, y) { return Math.round(x * 100) / 100 + " " + Math.round(y * 100) / 100 }
        return "M " + p(cx - half, r)
             + " L " + p(cx + half, r)
             + " L " + p(cx + half, baseY)
             + " L " + p(root.width - r, baseY)
             + " L " + p(cx, tipY)
             + " L " + p(r, baseY)
             + " L " + p(cx - half, baseY)
             + " Z"
    }

    width: Math.round(root.length * 0.62)
    height: root.length
    x: root.tipX - root._dir.x * root.length / 2 - width / 2
    y: root.tipY - root._dir.y * root.length / 2 - height / 2
    rotation: root.direction === "up" ? 180 : root.direction === "left" ? 90 : root.direction === "right" ? -90 : 0

    Shape {
        id: arrowShape
        width: root.width
        height: root.height
        y: -root._bounceOffset
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: root.outlineColor
            strokeColor: root.outlineColor
            strokeWidth: root._round * 2 + root._outline * 2
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap
            PathSvg { path: root._path }
        }

        ShapePath {
            fillColor: root.color
            strokeColor: root.color
            strokeWidth: root._round * 2
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap
            PathSvg { path: root._path }
        }
    }

    SequentialAnimation {
        running: root.bounce && root.visible
        loops: Animation.Infinite
        onRunningChanged: if (!running) root._bounceOffset = 0
        NumberAnimation { target: root; property: "_bounceOffset"; from: 0; to: root._amplitude; duration: 520; easing.type: Easing.InOutSine }
        NumberAnimation { target: root; property: "_bounceOffset"; from: root._amplitude; to: 0; duration: 520; easing.type: Easing.InOutSine }
    }
}

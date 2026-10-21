import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

Item {
    id: root

    property var holes: []
    property bool dimEnabled: true
    property real dimOpacity: 0.72
    property color dimBase: AppPalette.dim
    property bool ringEnabled: true
    property bool pulse: true
    property real pulseSpread: Math.round(10 * AppPalette.scale)
    property color ringColor: AppPalette.accent

    readonly property rect primaryRect: Qt.rect(root._x, root._y, root._w, root._h)

    readonly property bool _hasPrimary: root.holes.length > 0
    readonly property var _first: root._hasPrimary ? root.holes[0] : null

    property real _x: root._first ? root._first.x : root.width / 2
    property real _y: root._first ? root._first.y : root.height / 2
    property real _w: root._first ? root._first.width : 0
    property real _h: root._first ? root._first.height : 0
    property real _r: root._first ? root._first.radius : 0
    property bool _morphing: false
    readonly property real _bleed: 16

    Behavior on _x { enabled: root._morphing; NumberAnimation { duration: Anim.popupMs; easing.type: Anim.easingStd } }
    Behavior on _y { enabled: root._morphing; NumberAnimation { duration: Anim.popupMs; easing.type: Anim.easingStd } }
    Behavior on _w { enabled: root._morphing; NumberAnimation { duration: Anim.popupMs; easing.type: Anim.easingStd } }
    Behavior on _h { enabled: root._morphing; NumberAnimation { duration: Anim.popupMs; easing.type: Anim.easingStd } }
    Behavior on _r { enabled: root._morphing; NumberAnimation { duration: Anim.popupMs; easing.type: Anim.easingStd } }

    function morph() {
        root._morphing = true
        morphTimer.restart()
    }

    function _num(v) {
        return Math.round(v * 100) / 100
    }

    function _roundRect(x, y, w, h, r) {
        if (w <= 0 || h <= 0)
            return ""
        const rr = Math.max(0, Math.min(r, w / 2, h / 2))
        if (rr <= 0)
            return "M " + _num(x) + " " + _num(y) + " H " + _num(x + w) + " V " + _num(y + h) + " H " + _num(x) + " Z "
        const a = " A " + _num(rr) + " " + _num(rr) + " 0 0 1 "
        return "M " + _num(x + rr) + " " + _num(y)
             + " H " + _num(x + w - rr) + a + _num(x + w) + " " + _num(y + rr)
             + " V " + _num(y + h - rr) + a + _num(x + w - rr) + " " + _num(y + h)
             + " H " + _num(x + rr) + a + _num(x) + " " + _num(y + h - rr)
             + " V " + _num(y + rr) + a + _num(x + rr) + " " + _num(y) + " Z "
    }

    readonly property string _path: {
        const m = root._bleed
        let p = "M " + (-m) + " " + (-m) + " H " + _num(root.width + m) + " V " + _num(root.height + m) + " H " + (-m) + " Z "
        if (root._hasPrimary)
            p += _roundRect(root._x, root._y, root._w, root._h, root._r)
        for (let i = 1; i < root.holes.length; ++i) {
            const h = root.holes[i]
            p += _roundRect(h.x, h.y, h.width, h.height, h.radius)
        }
        return p
    }

    Timer {
        id: morphTimer
        interval: Anim.popupMs + 40
        onTriggered: root._morphing = false
    }

    Shape {
        anchors.fill: parent
        visible: root.dimEnabled
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillRule: ShapePath.OddEvenFill
            fillColor: Qt.rgba(root.dimBase.r, root.dimBase.g, root.dimBase.b, root.dimOpacity)
            strokeWidth: -1
            strokeColor: "transparent"
            PathSvg { path: root._path }
        }
    }

    Repeater {
        model: root.holes.length

        Rectangle {
            required property int index
            readonly property var hole: root.holes[index]
            visible: root.ringEnabled && !!hole
            x: index === 0 ? root._x : hole.x
            y: index === 0 ? root._y : hole.y
            width: index === 0 ? root._w : hole.width
            height: index === 0 ? root._h : hole.height
            radius: Math.min(index === 0 ? root._r : hole.radius, width / 2, height / 2)
            color: "transparent"
            border.width: Math.max(1, Math.round(2 * AppPalette.scale))
            border.color: root.ringColor
            opacity: 0.9

            Rectangle {
                id: pulseRing
                property real grow: 0
                anchors.centerIn: parent
                width: parent.width + grow * 2
                height: parent.height + grow * 2
                radius: parent.radius + grow
                color: "transparent"
                border.width: parent.border.width
                border.color: root.ringColor
                opacity: 0
                visible: root.pulse

                SequentialAnimation {
                    running: pulseRing.visible && root.visible
                    loops: Animation.Infinite
                    ParallelAnimation {
                        NumberAnimation { target: pulseRing; property: "opacity"; from: 0.85; to: 0; duration: 1100; easing.type: Anim.easingStd }
                        NumberAnimation { target: pulseRing; property: "grow"; from: 0; to: root.pulseSpread; duration: 1100; easing.type: Anim.easingStd }
                    }
                    PauseAnimation { duration: 350 }
                }
            }
        }
    }
}

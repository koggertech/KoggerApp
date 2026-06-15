import QtQuick 2.15

// Centralized tap / double-tap area used across the app.
//
// Why not just MouseArea.onDoubleClicked or TapHandler.onDoubleTapped?
//   • MouseArea.onDoubleClicked uses Qt's mouse-tuned thresholds: 5 px
//     movement, ~400 ms. Synthesised touch events on Android easily exceed
//     the distance threshold (finger jitter), so the gesture is missed.
//   • TapHandler.onDoubleTapped pulls its interval/distance from QStyleHints
//     too — same problem on Android. There's no API to widen it.
//
// Solution: drive recognition manually from TapHandler.onTapped — every
// successful tap fires once, and we apply our own generous window
// (interval + distance) to detect doubles. This way the thresholds are
// fully under our control and identical across mouse / touch / touchpad.
//
// Usage:
//     KTapArea {
//         anchors.fill: parent      // default — usually omitted
//         cursorPointing: true      // show hand cursor on mouse hover (default true)
//         onTapped: doSingle()      // optional
//         onDoubleTapped: doDouble()
//         onLongPressed: doHold()   // optional
//     }
Item {
    id: root

    property bool cursorPointing: true
    property bool active: true

    // Tunable detection window for double-tap. Distance comes from AppPalette
    // so the "Test" settings group can dial it live; interval is local since
    // it isn't currently exposed via a slider.
    property int doubleTapIntervalMs: 500
    property real doubleTapDistancePx: AppPalette.doubleTapDistancePx

    signal tapped()
    signal doubleTapped()
    signal longPressed()

    anchors.fill: parent ? parent : undefined

    // Debug overlay — paints a translucent border over the active area so
    // it's obvious where taps actually land. Off by default; flip from QML
    // (`KTapArea { debugVisualize: true; ... }`) when troubleshooting.
    property bool debugVisualize: false
    Rectangle {
        visible: root.debugVisualize
        anchors.fill: parent
        color: "transparent"
        border.color: "#FF00FF"
        border.width: 2
        z: 1000
    }

    HoverHandler {
        enabled: root.active && root.cursorPointing
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: tap
        enabled: root.active
        acceptedDevices: PointerDevice.Mouse
                         | PointerDevice.TouchScreen
                         | PointerDevice.TouchPad
        // Release-within-bounds tolerates finger movement while pressed —
        // the tap is counted as long as release happens inside the area.
        gesturePolicy: TapHandler.ReleaseWithinBounds

        property real _lastTapMs: 0
        property point _lastTapPos: Qt.point(-10000, -10000)

        onTapped: function(eventPoint) {
            // Always fire single-tap; consumers that only care about
            // double-tap simply leave onTapped unbound.
            root.tapped()

            var now = Date.now()
            var pos = eventPoint && eventPoint.position
                      ? eventPoint.position
                      : Qt.point(0, 0)
            var dx = pos.x - _lastTapPos.x
            var dy = pos.y - _lastTapPos.y
            var distSq = dx * dx + dy * dy
            var maxDistSq = root.doubleTapDistancePx * root.doubleTapDistancePx

            if ((now - _lastTapMs) <= root.doubleTapIntervalMs && distSq <= maxDistSq) {
                root.doubleTapped()
                _lastTapMs = 0  // consume so a triple-tap doesn't fire two doubles
                _lastTapPos  = Qt.point(-10000, -10000)
            } else {
                _lastTapMs = now
                _lastTapPos = pos
            }
        }

        onLongPressed: root.longPressed()
    }
}

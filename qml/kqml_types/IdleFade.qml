import QtQuick 2.15

// Auto-dim helper for scene control groups: stays fully opaque while the
// consumer is hovered (or freshly poked), then fades to `idleOpacity` after
// `delay` ms of inactivity. Non-visual (QtObject) so it adds nothing to a
// Layout. Usage:
//   HoverHandler { id: hov }
//   IdleFade { id: fade; hovered: hov.hovered }
//   opacity: fade.value
//   Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
QtObject {
    id: root

    property bool hovered: false
    property int delay: 2600
    property real idleOpacity: 0.4

    readonly property real value: (hovered || idleTimer.running) ? 1.0 : idleOpacity

    onHoveredChanged: hovered ? idleTimer.stop() : idleTimer.restart()

    function poke() { idleTimer.restart() }

    property Timer idleTimer: Timer { interval: root.delay; running: true }
}

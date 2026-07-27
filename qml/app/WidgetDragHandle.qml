import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property Item dragTarget: null
    property real diameter: Math.round(24 * AppPalette.appScale)
    property color fillColor: AppPalette.controlRaised
    property color borderColor: AppPalette.border
    property color dotColor: AppPalette.textSecond

    signal released()

    readonly property bool dragActive: dragArea.drag.active

    width: diameter
    height: diameter

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: dragArea.pressed ? Qt.lighter(root.fillColor, 1.2) : root.fillColor
        border.width: Math.max(1, Math.round(AppPalette.scale))
        border.color: root.borderColor
    }

    KDragBar {
        anchors.centerIn: parent
        orientation: "horizontal"
        showCapsule: false
        dotColor: root.dotColor
        barLength: Math.round(16 * AppPalette.appScale)
        barThickness: Math.round(12 * AppPalette.appScale)
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        cursorShape: dragArea.pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        drag.target: root.dragTarget
        preventStealing: true
        onReleased: root.released()
    }
}

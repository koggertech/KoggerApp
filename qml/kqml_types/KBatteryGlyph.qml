import QtQuick
import kqml_types 1.0

Item {
    id: root

    property int percent: -1
    property color ink: AppPalette.textSecond
    property color fillColor: ink
    property real unit: AppPalette.scale
    readonly property int stroke: Math.max(1, Math.round(unit))

    implicitWidth: Math.round(14 * unit)
    implicitHeight: Math.round(8 * unit)

    Rectangle {
        id: body
        width: parent.width - cap.width
        height: parent.height
        radius: Math.round(2 * root.unit)
        color: "transparent"
        border.width: root.stroke
        border.color: root.ink

        Rectangle {
            readonly property int inset: root.stroke * 2
            x: inset
            y: inset
            height: parent.height - inset * 2
            width: Math.round((parent.width - inset * 2) * Math.max(0, Math.min(100, root.percent)) / 100)
            visible: root.percent > 0
            radius: 1
            color: root.fillColor
        }
    }
    Rectangle {
        id: cap
        anchors.left: body.right
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(1, Math.round(2 * root.unit))
        height: Math.round(parent.height * 0.45)
        radius: 1
        color: root.ink
    }
}

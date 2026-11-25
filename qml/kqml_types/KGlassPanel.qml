import QtQuick

Item {
    id: panel

    property alias source: backdrop.source
    property alias originSpace: backdrop.originSpace
    property alias origin: backdrop.origin
    property real radius: 0
    property color fillColor: AppPalette.bg
    property real fillAlpha: AppPalette.glassAlpha
    property color borderColor: "transparent"
    property real borderWidth: 0

    function tint(c: color, a: real): color {
        return backdrop.tint(c, a)
    }

    KGlassBackdrop {
        id: backdrop
        anchors.fill: parent
        originSpace: panel.parent
        origin: Qt.point(panel.x, panel.y)
        radius: panel.radius
    }

    Rectangle {
        anchors.fill: parent
        radius: panel.radius
        color: backdrop.tint(panel.fillColor, panel.fillAlpha)
        border.color: panel.borderColor
        border.width: panel.borderWidth
    }
}

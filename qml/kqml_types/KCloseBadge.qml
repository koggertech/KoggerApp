import QtQuick 2.15
import kqml_types 1.0

// Small "close" badge for the bottom-right quadrant of an active tool button:
// signals the tool is open and a tap closes it. Visual overlay only (no
// MouseArea) — pointer events fall through to the parent button's hit area.
// The cross is drawn geometrically (two rounded bars) for a crisp, perfectly
// centered look at any size.
Item {
    id: badge
    readonly property real _d: Math.round((parent ? parent.width : 28) * 0.44)
    property color crossColor: AppPalette.text
    property color fillColor: AppPalette.bgDeep
    property color ringColor: AppPalette.border

    width: _d
    height: _d
    anchors.right: parent ? parent.right : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.rightMargin: Math.round(_d * 0.06)
    anchors.bottomMargin: Math.round(_d * 0.06)

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: badge.fillColor
        border.width: Math.max(1, Math.round(width * 0.05))
        border.color: badge.ringColor
    }

    Item {
        anchors.centerIn: parent
        width: Math.round(badge._d * 0.56)
        height: width
        readonly property int _t: Math.max(2, Math.round(width * 0.15))

        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: parent._t
            radius: height / 2
            color: badge.crossColor
            rotation: 45
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: parent._t
            radius: height / 2
            color: badge.crossColor
            rotation: -45
        }
    }
}

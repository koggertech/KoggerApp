import QtQuick
import kqml_types 1.0

Item {
    id: stick

    property Item pad: null
    property int bit: 0
    property real axisX: 0
    property real axisY: 0
    property real deadzone: 0
    readonly property bool down: pad ? pad.isDown(bit) : false
    readonly property real magnitude: Math.sqrt(axisX * axisX + axisY * axisY)
    readonly property bool live: magnitude > deadzone
    readonly property real reach: width / 2 - 6
    readonly property real dotScale: magnitude > 1 ? 1 / magnitude : 1

    width: 76
    height: 76

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: AppPalette.bgDeep
        border.color: stick.live ? AppPalette.accent : AppPalette.borderHover
        border.width: 2
    }

    Rectangle {
        width: 50
        height: 50
        radius: 25
        x: (stick.width - width) / 2 + stick.axisX * 16
        y: (stick.height - height) / 2 + stick.axisY * 16
        color: stick.down ? AppPalette.accent : AppPalette.controlRaised
        border.color: stick.down ? Qt.lighter(AppPalette.accent, 1.3) : AppPalette.borderFocus
        border.width: 2
    }

    Rectangle {
        readonly property real size: 2 * stick.reach * stick.deadzone
        visible: stick.deadzone > 0
        width: size
        height: size
        radius: size / 2
        x: (stick.width - size) / 2
        y: (stick.height - size) / 2
        color: Qt.rgba(AppPalette.textSecond.r, AppPalette.textSecond.g, AppPalette.textSecond.b, 0.25)
        border.color: AppPalette.textSecond
        border.width: 1.5
    }

    Rectangle {
        width: 8
        height: 8
        radius: 4
        x: stick.width / 2 + stick.axisX * stick.dotScale * stick.reach - width / 2
        y: stick.height / 2 + stick.axisY * stick.dotScale * stick.reach - height / 2
        color: stick.live ? AppPalette.accent : AppPalette.textSecond
        border.color: AppPalette.bgDeep
        border.width: 1
    }
}

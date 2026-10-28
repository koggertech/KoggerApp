import QtQuick
import kqml_types 1.0

Rectangle {
    id: trigger

    property real value: 0
    property real deadzone: 0
    property string label: ""
    readonly property bool live: value > deadzone
    readonly property real inset: border.width
    readonly property real innerHeight: height - 2 * inset
    readonly property real innerRadius: Math.max(0, radius - inset)

    width: 90
    height: 30
    radius: 8
    color: AppPalette.controlRaised
    border.color: live ? AppPalette.accent : AppPalette.borderHover
    border.width: 2

    Item {
        x: trigger.inset
        y: trigger.inset + trigger.innerHeight - height
        width: trigger.width - 2 * trigger.inset
        height: trigger.innerHeight * Math.max(0, Math.min(1, trigger.value))
        clip: true

        Rectangle {
            y: parent.height - height
            width: parent.width
            height: trigger.innerHeight
            radius: trigger.innerRadius
            color: trigger.live ? AppPalette.accent : Qt.rgba(AppPalette.accent.r, AppPalette.accent.g, AppPalette.accent.b, 0.4)
        }
    }

    Item {
        id: zoneLine
        x: trigger.inset
        y: trigger.inset + trigger.innerHeight * (1 - trigger.deadzone) - height / 2
        width: trigger.width - 2 * trigger.inset
        height: 2
        visible: trigger.deadzone > 0
        clip: true

        Rectangle {
            y: trigger.inset - zoneLine.y
            width: parent.width
            height: trigger.innerHeight
            radius: trigger.innerRadius
            color: AppPalette.textSecond
        }
    }

    Text {
        anchors.centerIn: parent
        text: trigger.label
        color: AppPalette.text
        font.pixelSize: 13
        font.bold: true
    }
}

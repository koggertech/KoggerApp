import QtQuick 2.15
import QtQuick.Controls 2.15

Switch {
    id: control

    property int rowHeight: 38
    property string toolTipText: text
    property int switchHorizontalPadding: 10
    property int cornerRadius: 8
    property color textColor: AppPalette.textSecond
    property color backgroundColor: AppPalette.bg
    property color borderColor: AppPalette.border
    property color accentColor: AppPalette.accentBg
    property color accentBorderColor: AppPalette.accentBorder
    property color trackOffColor: AppPalette.trackOff
    property color trackOffBorderColor: AppPalette.trackOffBorder
    property color knobColor: AppPalette.knob
    property int trackWidth: 44
    property int trackHeight: 24
    property int trackSpacing: 10
    property bool highlighted: false
    property int flashToken: 0
    property color highlightBorderColor: AppPalette.accentBorder

    implicitWidth: 260
    implicitHeight: rowHeight
    opacity: enabled ? 1.0 : 0.55

    indicator: Rectangle {
        width: control.trackWidth
        height: control.trackHeight
        radius: height / 2
        x: control.width - width - control.switchHorizontalPadding
        y: (control.height - height) / 2
        color: control.checked ? control.accentColor : control.trackOffColor
        border.width: 1
        border.color: control.checked ? control.accentBorderColor : control.trackOffBorderColor

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }

        Rectangle {
            width: 20
            height: 20
            radius: width / 2
            y: 2
            x: control.checked ? parent.width - width - 2 : 2
            color: control.knobColor
            border.width: 1
            border.color: "#00000022"

            Behavior on x {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    contentItem: Item {
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: control.switchHorizontalPadding
            anchors.rightMargin: control.switchHorizontalPadding + control.trackWidth + control.trackSpacing
            text: control.text
            color: control.textColor
            font.pixelSize: 14
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        id: bg
        radius: control.cornerRadius
        color: control.hovered ? AppPalette.bgHover : control.backgroundColor
        border.width: 1
        border.color: control.borderColor

        Rectangle {
            id: highlightOverlay
            anchors.fill: parent
            radius: bg.radius
            color: "transparent"
            border.width: 2
            border.color: control.highlightBorderColor
            opacity: 0
            visible: control.highlighted
        }

        SequentialAnimation {
            id: highlightPulse
            running: false
            NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.92; duration: 90; easing.type: Easing.OutQuad }
            NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.26; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.0; duration: 260; easing.type: Easing.OutCubic }
        }
    }

    onFlashTokenChanged: {
        if (highlighted)
            highlightPulse.restart()
    }

    onHighlightedChanged: {
        if (!highlighted)
            highlightOverlay.opacity = 0.0
    }

    KToolTip {
        text: control.toolTipText
        targetItem: control
        shown: control.hovered && control.enabled
    }
}

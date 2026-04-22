import QtQuick 2.15
import QtQuick.Controls 2.15

CheckBox {
    id: control

    property int rowHeight: 38
    property string toolTipText: text
    property int cornerRadius: 8
    property color textColor: AppPalette.textSecond
    property color backgroundColor: AppPalette.bg
    property color borderColor: AppPalette.border
    property color accentColor: AppPalette.accentBg
    property color accentBorderColor: AppPalette.accentBorder
    property color trackOffColor: AppPalette.trackOff
    property color trackOffBorderColor: AppPalette.trackOffBorder
    property color knobColor: AppPalette.knob

    horizontalPadding: 10
    implicitWidth: 260
    implicitHeight: rowHeight
    opacity: enabled ? 1.0 : 0.55
    hoverEnabled: true

    indicator: Rectangle {
        width: 44
        height: 24
        radius: height / 2
        x: control.width - width - control.horizontalPadding
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
            anchors.leftMargin: control.horizontalPadding
            anchors.rightMargin: control.horizontalPadding + 54
            text: control.text
            color: control.textColor
            font.pixelSize: 14
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        radius: control.cornerRadius
        color: control.hovered ? AppPalette.bgHover : control.backgroundColor
        border.width: 1
        border.color: control.borderColor
    }

    KToolTip {
        text: control.toolTipText
        targetItem: control
        shown: control.hovered && control.enabled
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: control

    property bool danger: false
    property string toolTipText: text
    property int cornerRadius: 6
    property int fontPixelSize: 14
    property bool bold: true
    property color normalBg: AppPalette.card
    property color normalBorder: AppPalette.border
    property color hoverBg: AppPalette.cardHover
    property color hoverBorder: AppPalette.borderHover
    property color checkedBg: AppPalette.accentBg
    property color checkedBorder: AppPalette.borderHover
    property color dangerBg: AppPalette.dangerBg
    property color dangerHoverBg: AppPalette.dangerHover
    property color dangerBorder: AppPalette.dangerBorder
    property color textColor: danger ? AppPalette.dangerText : AppPalette.text
    property real hoverWhiteness: 0.08

    horizontalPadding: 14
    verticalPadding: 7
    implicitWidth: Math.max(64, label.implicitWidth + horizontalPadding * 2)
    implicitHeight: Math.max(30, label.implicitHeight + verticalPadding * 2)
    opacity: enabled ? 1.0 : 0.45
    hoverEnabled: true
    scale: pressed ? 0.985 : (hovered ? 1.02 : 1.0)

    Behavior on scale {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    contentItem: Text {
        id: label
        text: control.text
        color: control.hovered ? Qt.lighter(control.textColor, 1.08) : control.textColor
        font.pixelSize: control.fontPixelSize
        font.bold: control.bold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }
    }

    background: Rectangle {
        id: bg
        radius: control.cornerRadius
        color: {
            if (control.danger)
                return control.hovered ? control.dangerHoverBg : control.dangerBg
            if (control.checkable && control.checked)
                return control.checkedBg
            return control.hovered ? control.hoverBg : control.normalBg
        }
        border.width: 1
        border.color: {
            if (control.danger)
                return control.dangerBorder
            if ((control.checkable && control.checked) || control.hovered)
                return control.hoverBorder
            return control.normalBorder
        }

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: bg.radius
            color: "#FFFFFF"
            opacity: control.pressed ? 0.02 : (control.hovered ? control.hoverWhiteness : 0.0)

            Behavior on opacity {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    KToolTip {
        text: control.toolTipText
        targetItem: control
        shown: control.hovered && control.enabled
    }
}

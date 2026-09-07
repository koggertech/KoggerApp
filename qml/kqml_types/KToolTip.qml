import QtQuick 2.15
import QtQuick.Controls 2.15

ToolTip {
    id: root

    property Item targetItem: null
    property bool shown: false

    visible: shown && text !== ""
    delay: 520
    timeout: 3200
    x: parent ? Math.round((parent.width - implicitWidth) / 2) : 0
    y: parent ? -implicitHeight - Math.round(8 * AppPalette.scale) : 0
    padding: Math.round(8 * AppPalette.scale)

    transformOrigin: y < 0 ? Item.Bottom : Item.Top

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0; to: 1.0
                duration: Anim.tooltipMs
                easing.type: Anim.tooltipEasing
            }
            NumberAnimation {
                property: "scale"
                from: Anim.tooltipEnterScale; to: 1.0
                duration: Anim.tooltipMs
                easing.type: Anim.tooltipEnterEasing
                easing.overshoot: Anim.tooltipOvershoot
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0; to: 0.0
                duration: Anim.tooltipExitMs
                easing.type: Anim.tooltipEasing
            }
            NumberAnimation {
                property: "scale"
                from: 1.0; to: Anim.tooltipExitScale
                duration: Anim.tooltipExitMs
                easing.type: Anim.tooltipEasing
            }
        }
    }

    contentItem: Text {
        text: root.text
        color: AppPalette.tooltipText
        font.pixelSize: Math.round(12 * AppPalette.scale)
        wrapMode: Text.NoWrap
    }

    background: Rectangle {
        radius: Math.round(6 * AppPalette.scale)
        color: AppPalette.tooltipBg
        border.width: 1
        border.color: AppPalette.tooltipBorder
    }
}

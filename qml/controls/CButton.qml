import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: control
    text: qsTr("Ok")
    checkable: false
    highlighted: true
    implicitHeight: theme.controlHeight
    hoverEnabled: true   // rely on built-in hover
    property bool active: (!control.checkable) || (control.checked && control.checkable)
    property var backColor: active ? theme.controlSolidBackColor : theme.controlBackColor
    property int borderRadius: 2
    readonly property bool hoverActive: control.hovered && control.enabled
    readonly property bool solidVisual: control.down || control.active
    readonly property color foregroundColor: !control.enabled
                                            ? theme.disabledTextColor
                                            : (control.solidVisual ? theme.textSolidColor : theme.textColor)

    palette.buttonText: foregroundColor
    palette.brightText: foregroundColor
    icon.color: foregroundColor

    contentItem: CText {
        text: control.text
        color: control.foregroundColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        id: backRect

        implicitHeight: control.height
        implicitWidth: implicitHeight
        radius: borderRadius
        color: control.down ? theme.controlSolidBackColor
                            : hoverActive ? theme.hoveredBackColor
                                           : backColor
        border.color: control.down ? theme.controlSolidBorderColor
                                   : hoverActive ? theme.controlSolidBorderColor
                                                 : theme.controlSolidBorderColor
        border.width: (!control.checkable && control.down) ? 2 : hoverActive ? 1 : 0

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
        Behavior on border.width { NumberAnimation { duration: 80 } }

        Rectangle {
            anchors.fill: parent
            radius: borderRadius
            color: theme.hoveredBackColor
            opacity: hoverActive ? 0.35 : 0

            Behavior on opacity { NumberAnimation { duration: 100 } }
        }
    }
}

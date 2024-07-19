import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: control
    text: qsTr("Ok")
    checkable: false
    highlighted: true
    implicitHeight: theme.controlHeight
    property bool active: (!control.checkable) || (control.checked && control.checkable)
    property var backColor: active ? theme.controlSolidBackColor : theme.controlBackColor
    property int borderRadius: 2

    contentItem: CText {
        text: control.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        id: backRect

        implicitHeight: control.height
        implicitWidth: implicitHeight
        radius: borderRadius
        color: backColor
        border.color: theme.controlSolidBorderColor
        border.width: (!control.checkable && control.down) ? 2 : 0
    }
}

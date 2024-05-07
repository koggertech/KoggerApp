import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: control
    text: "Ok"
    checkable: false
    highlighted: true
    implicitHeight: theme.controlHeight
    property bool active: (!control.checkable) || (control.checked && control.checkable)

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
        radius: 2
        color: active ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: theme.controlSolidBorderColor
        border.width: (!control.checkable && control.down) ? 2 : 0
    }
}

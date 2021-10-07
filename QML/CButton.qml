import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: control
    text: "Ok"
    highlighted: true
    implicitHeight: theme.controlHeight

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
        radius: 1
        color: control.down ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: control.down ? theme.controlSolidBorderColor : theme.controlBorderColor
        border.width: 1
    }
}

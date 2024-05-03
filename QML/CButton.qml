import QtQuick 2.15
import QtQuick.Controls 2.15

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
        radius: 2
        color: control.down || control.checked ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: control.down || control.checked ? theme.controlSolidBorderColor : theme.controlBorderColor
        border.width: 0
    }
}

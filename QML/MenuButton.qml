import QtQuick 2.1
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false

    id: control
    Layout.preferredHeight: theme.controlHeight
    padding: 0

    icon.color: theme.textColor

    background: Rectangle {
        id: backRect
        radius: 1
        height: parent.height
        width: parent.width
        color: control.active ? theme.controlBorderColor : theme.menuBackColor
        border.color: theme.controlBorderColor
        border.width: 1
    }

//    contentItem: CText {
//        text: control.text
//        horizontalAlignment: Text.AlignHCenter
//        verticalAlignment: Text.AlignTop
//        font.pointSize: 20
//    }
}

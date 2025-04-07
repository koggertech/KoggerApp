import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false
    property int borderWidth: 0

    id: control
    Layout.preferredHeight: theme.controlHeight
    padding: 0

    icon.color: theme.textColor

    background: Rectangle {
        id: backRect
        radius: 2
        height: parent.height
        width: parent.width
        color: (control.active) ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: theme.controlBorderColor
        border.width: borderWidth
    }

//    contentItem: CText {
//        text: control.text
//        horizontalAlignment: Text.AlignHCenter
//        verticalAlignment: Text.AlignTop
//        font.pointSize: 20
//    }
}

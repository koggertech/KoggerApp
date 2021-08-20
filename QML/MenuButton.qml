import QtQuick 2.1
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false

    id: control
    Layout.preferredHeight: 30
    padding: 0

    icon.color: theme.textColor

    background: Rectangle {
        id: backRect
        color: "#303030"
        border.color: "#505050"
        border.width: 1
        radius: 1
    }

//    contentItem: CText {
//        text: control.text
//        horizontalAlignment: Text.AlignHCenter
//        verticalAlignment: Text.AlignTop
//        font.pointSize: 20
//    }

    onActiveChanged: {
        if(control.active) {
            backRect.color =  "#606060"
        } else {
            backRect.color =  "#303030"
        }
    }
}

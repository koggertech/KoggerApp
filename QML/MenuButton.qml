import QtQuick 2.1
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false

    id: control
    Layout.preferredWidth: 58
    Layout.preferredHeight: 28
    Layout.alignment: Qt.AlignTop
    opacity: 0.8

    font.pointSize: 10;

    background: Rectangle {
        id: backRect
        color: "#303030"
        border.color: "#505050"
        border.width: 1
        radius: 1
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: 1.0
        color: "#BBBBBB"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
    }

    onActiveChanged: {
        if(control.active) {
            backRect.color =  "#606060"
            opacity = 1
        } else {
            backRect.color =  "#303030"
            opacity = 0.8
        }
    }
}

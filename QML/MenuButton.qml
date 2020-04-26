import QtQuick 2.1
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false

    id: control
    Layout.preferredWidth: 35
    Layout.preferredHeight: 30
    Layout.alignment: Qt.AlignTop
    opacity: 0.8

    font.family: "Roboto"; font.pointSize: 13;

    background: TabBackStyle {
        active: control.active
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: 1.0
        color: "#FF7030"
        horizontalAlignment: Text.AlignHCenter
//        verticalAlignment: Text.AlignTop
        elide: Text.ElideRight
    }

    onActiveChanged: {
        if(control.active) {
            Layout.preferredWidth = 35
            Layout.preferredHeight = 35
            opacity = 1.0
        } else {
            Layout.preferredWidth = 35
            Layout.preferredHeight = 30
            opacity = 0.8
        }
    }
}

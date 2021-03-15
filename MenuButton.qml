import QtQuick 2.1
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Button {
    property bool active: false

    id: control
    Layout.preferredWidth: 50
    Layout.preferredHeight: 28
    Layout.alignment: Qt.AlignTop
    opacity: 0.5

    font.pointSize: 12;

    background: Rectangle {
        color: "#505050"

        border.color: "#606060"
        border.width: 1
        radius: 2
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: 1.0
        color: "#999999"
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    onActiveChanged: {
        if(control.active) {
            opacity = 0.8
        } else {
            opacity = 0.5
        }
    }
}

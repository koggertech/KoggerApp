import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: customPopup
    property string popupText: qsTr("Custom popup")

    x: parent.mouseX
    y: parent.mouseY + 10
    visible: false
    background: null

    Rectangle {
        color: theme.controlBackColor
        opacity: 0.8
        anchors.centerIn: parent
        radius: 5
        implicitWidth: textItem.implicitWidth + 5
        implicitHeight: textItem.implicitHeight + 5

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                id: textItem
                text: popupText
                color: "white"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: customPopup
    property string popupText: qsTr("Custom popup")
    property string popupPosition: "bottomRight" // topLeft, topRight, bottomLeft, bottomRight
    property point popupOffset: Qt.point(10, 10)

    x: {
        switch (popupPosition) {
        case "topLeft":     return parent.mouseX - contentItem.implicitWidth - popupOffset.x
        case "bottomLeft":  return parent.mouseX - contentItem.implicitWidth - popupOffset.x
        case "topRight":    return parent.mouseX + popupOffset.x
        case "bottomRight": return parent.mouseX + popupOffset.x
        default:            return parent.mouseX + popupOffset.x
        }
    }
    y: {
        switch (popupPosition) {
        case "topLeft":
        case "topRight":   return parent.mouseY - contentItem.implicitHeight - popupOffset.y
        case "bottomLeft":
        case "bottomRight":return parent.mouseY + popupOffset.y
        default:           return parent.mouseY + popupOffset.y
        }
    }

    visible: false
    background: null

    Rectangle {
        id: contentItem
        color: theme.controlBackColor
        opacity: 0.8
        anchors.centerIn: parent
        radius: 5
        implicitWidth: textItem.implicitWidth + 10
        implicitHeight: textItem.implicitHeight + 10

        Column {
            anchors.centerIn: parent
            spacing: 5

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

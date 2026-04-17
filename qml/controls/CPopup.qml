import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: customPopup
    property string popupText: qsTr("Custom popup")
    property string popupPosition: "bottomRight" // topLeft, topRight, bottomLeft, bottomRight
    property point popupOffset: Qt.point(10, 10)
    property real cursorX: parent ? parent.width * 0.5 : 0
    property real cursorY: parent ? parent.height * 0.5 : 0

    readonly property color bubbleColor: theme.tooltipBackColor
    readonly property color bubbleBorderColor: theme.tooltipBorderColor
    readonly property color bubbleTextColor: theme.tooltipTextColor

    x: {
        const popupWidth = contentItem ? contentItem.implicitWidth : 0
        switch (popupPosition) {
        case "topLeft":     return cursorX - popupWidth - popupOffset.x
        case "bottomLeft":  return cursorX - popupWidth - popupOffset.x
        case "topRight":    return cursorX + popupOffset.x
        case "bottomRight": return cursorX + popupOffset.x
        default:             return cursorX + popupOffset.x
        }
    }

    y: {
        const popupHeight = contentItem ? contentItem.implicitHeight : 0
        switch (popupPosition) {
        case "topLeft":
        case "topRight":    return cursorY - popupHeight - popupOffset.y
        case "bottomLeft":
        case "bottomRight": return cursorY + popupOffset.y
        default:             return cursorY + popupOffset.y
        }
    }

    visible: false
    padding: 0
    background: null

    contentItem: Rectangle {
        id: popupBubble
        color: customPopup.bubbleColor
        border.color: customPopup.bubbleBorderColor
        border.width: 1
        radius: 5
        implicitWidth: textItem.implicitWidth + 10
        implicitHeight: textItem.implicitHeight + 10

        Text {
            id: textItem
            anchors.centerIn: parent
            text: popupText
            color: customPopup.bubbleTextColor
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }
}

import QtQuick 2.15

Item {
    id: hoverArea
    anchors.fill: parent

    anchors.margins: -2

    property alias containsMouse: hoverHandler.hovered
    property string toolTipText: qsTr("Tooltip")
    property string popupPosition: "bottomRight"
    property point popupOffset: Qt.point(10, 10)
    property real cursorX: hoverHandler.point.position.x
    property real cursorY: hoverHandler.point.position.y

    onContainsMouseChanged: {
        if (containsMouse) {
            tooltipTimer.start()
        } else {
            tooltipTimer.stop()
            customToolTip.close()
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    Timer {
        id: tooltipTimer
        interval: 1000
        repeat: false
        onTriggered: {
            customToolTip.open()
        }
    }

    CPopup {
        id: customToolTip
        popupText: toolTipText
        popupPosition: hoverArea.popupPosition
        popupOffset: hoverArea.popupOffset
        cursorX: hoverArea.cursorX
        cursorY: hoverArea.cursorY
    }
}

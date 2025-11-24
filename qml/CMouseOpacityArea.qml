import QtQuick 2.15

MouseArea {
    id: hoverArea
    anchors.fill: parent
    hoverEnabled: true

    anchors.margins: -2

    property bool isMouseAccepted: false
    property string toolTipText: qsTr("Tooltip")
    property string popupPosition: "bottomRight"
    property point popupOffset: Qt.point(10, 10)

    onPressed: function(mouse) {
        if (mouse.source === 2) {
             tooltipTimer.stop()
             customToolTip.close()
        }

        mouse.accepted = isMouseAccepted
    }
    onReleased: function(mouse) {
        mouse.accepted = isMouseAccepted
    }
    onClicked: function(mouse) {
        if (mouse.source === 2) {
             tooltipTimer.stop()
             customToolTip.close()
        }

        mouse.accepted = isMouseAccepted
    }
    onEntered: {
        tooltipTimer.start()
    }
    onExited: {
        tooltipTimer.stop()
        customToolTip.close()
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
    }
}

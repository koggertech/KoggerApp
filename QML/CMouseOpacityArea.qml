import QtQuick 2.15

MouseArea {
    id: hoverArea
    anchors.fill: parent
    hoverEnabled: true
    property bool isMouseAccepted: false
    property string toolTipText: qsTr("Tooltip")
    anchors.margins: -2

    onPressed: mouse.accepted = isMouseAccepted
    onReleased: mouse.accepted = isMouseAccepted
    onClicked: mouse.accepted = isMouseAccepted

    CPopup {
        id: customToolTip
        popupText: toolTipText
    }

    onEntered: {
        customToolTip.open()
    }
    onExited: {
        customToolTip.close()
    }
}

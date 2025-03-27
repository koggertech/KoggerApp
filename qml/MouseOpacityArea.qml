import QtQuick 2.15

MouseArea {
    id: mouseArea
    propagateComposedEvents: true
    anchors.fill: parent
    hoverEnabled: true
    onPressed: mouse.accepted = isMouseAccepted
    onReleased: mouse.accepted = isMouseAccepted
    onClicked: mouse.accepted = isMouseAccepted

    property real offOpacity: 0.5
    property bool isMouseAccepted: false

    onContainsMouseChanged: {
        parent.opacity = containsMouse ? 1 : offOpacity
    }

    Component.onCompleted: {
        parent.opacity = containsMouse ? 1 : offOpacity
    }
}

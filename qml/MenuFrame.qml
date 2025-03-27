import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: control

    property int offsetOpacityArea: 0
    property int margins: 5
    property int horizontalMargins: margins
    property int verticalMargins: margins
    property int spacing: 10
    property color backgroundColor: theme.menuBackColor

    implicitWidth: columnItem.width + horizontalMargins*2
    implicitHeight: columnItem.height + verticalMargins*2

    default property alias content: columnItem.data
    property bool isMouseAccepted: true
    property bool isDraggable: false
    property bool isOpacityControlled: false
    property real offOpacity: 0.5
    property bool isHovered: mouseOpacityArea.containsMouse

    Rectangle {
        id: backgroundRect
        color: backgroundColor
        anchors.fill: columnItem
        anchors.leftMargin: -horizontalMargins
        anchors.rightMargin: -horizontalMargins
        anchors.topMargin: -verticalMargins
        anchors.bottomMargin: -verticalMargins
        opacity: columnItem.opacity
        radius: 2
    }

    MouseArea {
        id: mouseDragArea
        propagateComposedEvents: true
        anchors.fill: columnItem
        onPressed: mouse.accepted = true
        onReleased: mouse.accepted = true
        onClicked: mouse.accepted = true

        drag.target: isDraggable ? columnItem : undefined
        drag.axis: Drag.XandYAxis
    }

    Item  {
        id: columnItem
        x: horizontalMargins
        y: verticalMargins
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }

    MouseArea {
        id: mouseOpacityArea
        // enabled: control.visible
        propagateComposedEvents: true
        anchors.fill: columnItem
        anchors.margins: -10 - control.offsetOpacityArea
        hoverEnabled: true
        onPressed: mouse.accepted = false
        onReleased: mouse.accepted = false
        onClicked: mouse.accepted = false

        onContainsMouseChanged: {
            columnItem.opacity = !isOpacityControlled || containsMouse ? 1 : offOpacity
        }

        Component.onCompleted: {
            columnItem.opacity = !isOpacityControlled || containsMouse ? 1 : offOpacity
        }
    }
}

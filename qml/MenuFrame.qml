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
    property bool isHovered: hoverHandler.hovered
    property bool anchorsReleasedForDrag: false

    function releaseAnchorsForDrag() {
        if (anchorsReleasedForDrag || !control.parent) {
            return
        }

        var p = control.mapToItem(control.parent, 0, 0)

        control.anchors.left = undefined
        control.anchors.right = undefined
        control.anchors.top = undefined
        control.anchors.bottom = undefined
        control.anchors.horizontalCenter = undefined
        control.anchors.verticalCenter = undefined
        control.anchors.fill = undefined
        control.anchors.centerIn = undefined

        control.x = p.x
        control.y = p.y
        anchorsReleasedForDrag = true
    }

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
        anchors.fill: backgroundRect
        onPressed:  function(mouse) {
            if (isDraggable) {
                control.releaseAnchorsForDrag()
            }
            mouse.accepted = true
        }
        onReleased: function(mouse) { mouse.accepted = true }
        onClicked:  function(mouse) { mouse.accepted = true }

        drag.target: isDraggable ? control : undefined
        drag.axis: Drag.XandYAxis
    }

    Item  {
        id: columnItem
        x: horizontalMargins
        y: verticalMargins
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        opacity: (!control.isOpacityControlled || hoverHandler.hovered) ? 1 : control.offOpacity
    }

    Item {
        id: hoverArea
        anchors.fill: backgroundRect
        anchors.margins: -control.offsetOpacityArea
        HoverHandler {
            id: hoverHandler
        }
    }
}

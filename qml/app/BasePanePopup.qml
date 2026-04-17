import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Item {
    id: root

    property bool popupVisible: true
    property string title: ""
    property real expandedWidth: 640
    property real expandedHeight: 480
    property real collapsedSize: 36
    property real popupWidth: collapsed ? collapsedSize : expandedWidth
    property real popupHeight: collapsed ? collapsedSize : expandedHeight
    property real panelX: 0
    property real panelY: 0
    property bool collapsed: false
    property int popupMargin: 16
    property bool dragEnabled: true
    property bool suspendSignals: false
    property bool collapseButtonDragged: false
    property color panelColor: "#0B1220"
    property color panelBorderColor: "#93C5FD77"

    signal collapsedToggled(bool collapsed)
    signal positionCommitted(real x, real y, real popupWidth, real popupHeight)
    signal sizeCommitted(real expandedWidth, real expandedHeight)
    signal closeRequested()
    signal popupDoubleClicked()

    default property alias popupContent: contentHost.data

    property real dragStartMouseX: 0
    property real dragStartMouseY: 0
    property real dragStartPanelX: 0
    property real dragStartPanelY: 0

    visible: popupVisible
    z: 260

    function clampX(value) {
        var spacing = Math.max(0, popupMargin)
        var minX = spacing
        var maxX = width - popupWidth - spacing
        if (maxX < minX) {
            minX = 0
            maxX = Math.max(0, width - popupWidth)
        }
        return Math.max(minX, Math.min(value, maxX))
    }

    function clampY(value) {
        var spacing = Math.max(0, popupMargin)
        var minY = spacing
        var maxY = height - popupHeight - spacing
        if (maxY < minY) {
            minY = 0
            maxY = Math.max(0, height - popupHeight)
        }
        return Math.max(minY, Math.min(value, maxY))
    }

    function commitPosition() {
        if (suspendSignals)
            return
        positionCommitted(panelX, panelY, popupWidth, popupHeight)
    }

    function toggleCollapsedFromButton() {
        var anchorRight = panelX + popupWidth
        collapsed = !collapsed
        panelX = Math.round(clampX(anchorRight - popupWidth))
        panelY = Math.round(clampY(panelY))
        commitPosition()
    }

    onCollapsedChanged: {
        panelX = clampX(panelX)
        panelY = clampY(panelY)
        if (suspendSignals)
            return
        collapsedToggled(collapsed)
    }

    onExpandedWidthChanged: {
        if (Math.abs(expandedWidth - 640) > 0.01) {
            expandedWidth = 640
            return
        }
        panelX = clampX(panelX)
    }

    onExpandedHeightChanged: {
        if (Math.abs(expandedHeight - 480) > 0.01) {
            expandedHeight = 480
            return
        }
        panelY = clampY(panelY)
    }

    onPopupWidthChanged: panelX = clampX(panelX)
    onPopupHeightChanged: panelY = clampY(panelY)
    onWidthChanged: panelX = clampX(panelX)
    onHeightChanged: panelY = clampY(panelY)

    Component.onCompleted: {
        expandedWidth = 640
        expandedHeight = 480
        panelX = clampX(panelX)
        panelY = clampY(panelY)
    }

    Rectangle {
        id: panel

        visible: root.popupVisible
        x: root.panelX
        y: root.panelY
        width: root.popupWidth
        height: root.popupHeight
        radius: 10
        color: root.collapsed ? "transparent" : root.panelColor
        border.width: root.collapsed ? 0 : 1
        border.color: root.panelBorderColor
        z: 1
        layer.enabled: true
        layer.smooth: true

        MouseArea {
            // Blocks pointer/wheel events from leaking to content under the popup.
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: false
            preventStealing: true
            propagateComposedEvents: false
            z: 0

            onPressed: function(mouse) {
                mouse.accepted = true
                if (!root.dragEnabled || mouse.button !== Qt.LeftButton)
                    return
                var pointer = mapToItem(root, mouse.x, mouse.y)
                root.dragStartMouseX = pointer.x
                root.dragStartMouseY = pointer.y
                root.dragStartPanelX = root.panelX
                root.dragStartPanelY = root.panelY
            }
            onReleased: function(mouse) {
                mouse.accepted = true
                root.commitPosition()
            }
            onClicked: function(mouse) { mouse.accepted = true }
            onDoubleClicked: function(mouse) {
                mouse.accepted = true
                root.popupDoubleClicked()
            }
            onPositionChanged: function(mouse) {
                mouse.accepted = true
                if (!root.dragEnabled || !pressed)
                    return
                var pointer = mapToItem(root, mouse.x, mouse.y)
                root.panelX = Math.round(root.clampX(root.dragStartPanelX + pointer.x - root.dragStartMouseX))
                root.panelY = Math.round(root.clampY(root.dragStartPanelY + pointer.y - root.dragStartMouseY))
            }
            onWheel: function(wheel) { wheel.accepted = true }
            onCanceled: root.commitPosition()
        }

        KButton {
            id: collapseButton

            width: 32
            height: 30
            text: root.collapsed ? "\u25A1" : "\u2014"
            x: parent.width - width - 2
            y: 2
            fontPixelSize: 15
            horizontalPadding: 0
            verticalPadding: 0
            cornerRadius: 6
            normalBg: "#1E293BA6"
            hoverBg: "#0F172ACC"
            normalBorder: "#334155"
            hoverBorder: "#475569"
            textColor: "#E2E8F0EE"
            z: 4
            onClicked: {
                if (root.collapseButtonDragged) {
                    root.collapseButtonDragged = false
                    return
                }
                root.toggleCollapsedFromButton()
            }

            DragHandler {
                id: collapseButtonDrag

                target: null
                enabled: root.dragEnabled
                xAxis.enabled: true
                yAxis.enabled: true

                property real startPanelX: 0
                property real startPanelY: 0
                property bool moved: false

                onActiveChanged: {
                    if (active) {
                        startPanelX = root.panelX
                        startPanelY = root.panelY
                        moved = false
                        return
                    }

                    if (moved)
                        root.collapseButtonDragged = true

                    root.commitPosition()
                }

                onTranslationChanged: {
                    if (!active)
                        return

                    if (Math.abs(translation.x) > 1 || Math.abs(translation.y) > 1)
                        moved = true

                    root.panelX = Math.round(root.clampX(startPanelX + translation.x))
                    root.panelY = Math.round(root.clampY(startPanelY + translation.y))
                }
            }
        }

        Item {
            id: contentHost

            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            anchors.topMargin: 8
            anchors.bottomMargin: 8
            visible: !root.collapsed
            z: 2
        }
    }
}

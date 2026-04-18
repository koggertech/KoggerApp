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
    property bool collapseButtonVisible: true
    property bool fullscreenMode: false
    property color panelColor: "#0B1220"
    property color panelBorderColor: "#93C5FD77"

    readonly property real headerHeight: 32
    readonly property real contentPadding: 4

    signal collapsedToggled(bool collapsed)
    signal positionCommitted(real x, real y, real popupWidth, real popupHeight)
    signal sizeCommitted(real expandedWidth, real expandedHeight)
    signal closeRequested()
    signal popupDoubleClicked()

    default property alias popupContent: contentHost.data

    visible: popupVisible

    function clampX(value) {
        var spacing = Math.max(0, popupMargin)
        var minX = spacing
        var maxX = width - popupWidth - spacing
        if (maxX < minX) { minX = 0; maxX = Math.max(0, width - popupWidth) }
        return Math.max(minX, Math.min(value, maxX))
    }

    function clampY(value) {
        var spacing = Math.max(0, popupMargin)
        var minY = spacing
        var maxY = height - popupHeight - spacing
        if (maxY < minY) { minY = 0; maxY = Math.max(0, height - popupHeight) }
        return Math.max(minY, Math.min(value, maxY))
    }

    function commitPosition() {
        if (suspendSignals) return
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
        if (suspendSignals) return
        collapsedToggled(collapsed)
    }

    onExpandedWidthChanged: {
        if (Math.abs(expandedWidth - 640) > 0.01) { expandedWidth = 640; return }
        panelX = clampX(panelX)
    }

    onExpandedHeightChanged: {
        if (Math.abs(expandedHeight - 480) > 0.01) { expandedHeight = 480; return }
        panelY = clampY(panelY)
    }

    property real lastParentWidth: 0
    property real lastParentHeight: 0

    function rescaleX(oldParentWidth) {
        var sp = Math.max(0, popupMargin)
        var oldMin = sp; var oldMax = oldParentWidth - popupWidth - sp
        if (oldMax < oldMin) { oldMin = 0; oldMax = Math.max(0, oldParentWidth - popupWidth) }
        var t = (oldMax > oldMin) ? Math.max(0, Math.min(1, (panelX - oldMin) / (oldMax - oldMin))) : 0
        var newMin = sp; var newMax = width - popupWidth - sp
        if (newMax < newMin) { newMin = 0; newMax = Math.max(0, width - popupWidth) }
        panelX = clampX(newMin + t * (newMax - newMin))
    }

    function rescaleY(oldParentHeight) {
        var sp = Math.max(0, popupMargin)
        var oldMin = sp; var oldMax = oldParentHeight - popupHeight - sp
        if (oldMax < oldMin) { oldMin = 0; oldMax = Math.max(0, oldParentHeight - popupHeight) }
        var t = (oldMax > oldMin) ? Math.max(0, Math.min(1, (panelY - oldMin) / (oldMax - oldMin))) : 0
        var newMin = sp; var newMax = height - popupHeight - sp
        if (newMax < newMin) { newMin = 0; newMax = Math.max(0, height - popupHeight) }
        panelY = clampY(newMin + t * (newMax - newMin))
    }

    onPopupWidthChanged:  panelX = clampX(panelX)
    onPopupHeightChanged: panelY = clampY(panelY)
    onWidthChanged: {
        if (lastParentWidth > 0 && width > 0) rescaleX(lastParentWidth)
        else panelX = clampX(panelX)
        lastParentWidth = width
    }
    onHeightChanged: {
        if (lastParentHeight > 0 && height > 0) rescaleY(lastParentHeight)
        else panelY = clampY(panelY)
        lastParentHeight = height
    }
    onFullscreenModeChanged: if (fullscreenMode) collapsed = false

    Component.onCompleted: {
        expandedWidth = 640
        expandedHeight = 480
        panelX = clampX(panelX)
        panelY = clampY(panelY)
        lastParentWidth = width
        lastParentHeight = height
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
        border.width: root.collapsed || root.fullscreenMode ? 0 : 1
        border.color: root.panelBorderColor
        z: 1
        layer.enabled: true
        layer.smooth: true

        states: State {
            name: "fullscreen"
            when: root.fullscreenMode
            PropertyChanges {
                target: panel
                x: 0
                y: 0
                width: root.width
                height: root.height
                radius: 0
            }
        }

        transitions: [
            Transition {
                to: "fullscreen"
                NumberAnimation {
                    properties: "x,y,width,height,radius"
                    duration: 260
                    easing.type: Easing.OutCubic
                }
            },
            Transition {
                from: "fullscreen"
                NumberAnimation {
                    properties: "x,y,width,height,radius"
                    duration: 220
                    easing.type: Easing.InOutQuad
                }
            }
        ]

        // Blocks pointer/wheel events from leaking under the popup.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: false
            preventStealing: true
            propagateComposedEvents: false
            z: 0
            onPressed:       function(mouse) { mouse.accepted = true }
            onReleased:      function(mouse) { mouse.accepted = true }
            onClicked:       function(mouse) { mouse.accepted = true }
            onDoubleClicked: function(mouse) { mouse.accepted = true; root.popupDoubleClicked() }
            onWheel:         function(wheel)  { wheel.accepted = true }
        }

        // Drag handle header strip — only area that initiates popup drag.
        Item {
            id: headerStrip
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root.headerHeight
            z: 5
            opacity: root.fullscreenMode ? 0.0 : 1.0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
            }

            // Grip dots — visible hint that this area is draggable.
            Row {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: root.collapseButtonVisible ? -20 : 0
                spacing: 5
                visible: !root.collapsed
                Repeater {
                    model: 4
                    delegate: Rectangle {
                        width: 4; height: 4; radius: 2
                        color: "#475569"
                    }
                }
            }

            DragHandler {
                id: headerDrag
                target: null
                enabled: root.dragEnabled && !root.fullscreenMode
                xAxis.enabled: true
                yAxis.enabled: true

                property real startX: 0
                property real startY: 0

                onActiveChanged: {
                    if (active) {
                        startX = root.panelX
                        startY = root.panelY
                    } else {
                        root.commitPosition()
                    }
                }

                onTranslationChanged: {
                    if (!active) return
                    root.panelX = Math.round(root.clampX(startX + translation.x))
                    root.panelY = Math.round(root.clampY(startY + translation.y))
                }
            }

            KButton {
                id: collapseButton
                visible: root.collapseButtonVisible
                width: 32
                height: 30
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 2
                text: root.collapsed ? "\u25A1" : "\u2014"
                fontPixelSize: 15
                horizontalPadding: 0
                verticalPadding: 0
                cornerRadius: 6
                normalBg: "#1E293BA6"
                hoverBg: "#0F172ACC"
                normalBorder: "#334155"
                hoverBorder: "#475569"
                textColor: "#E2E8F0EE"
                z: 6
                onClicked: root.toggleCollapsedFromButton()
            }
        }

        Item {
            id: contentHost

            anchors.fill: parent
            anchors.leftMargin: root.fullscreenMode ? 0 : root.contentPadding
            anchors.rightMargin: root.fullscreenMode ? 0 : root.contentPadding
            anchors.topMargin: root.fullscreenMode ? 0 : root.headerHeight
            anchors.bottomMargin: root.fullscreenMode ? 0 : root.contentPadding
            visible: !root.collapsed
            z: 2

            Behavior on anchors.topMargin {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            Behavior on anchors.leftMargin {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            Behavior on anchors.rightMargin {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            Behavior on anchors.bottomMargin {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
        }
    }
}

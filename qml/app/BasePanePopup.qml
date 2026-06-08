import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Item {
    id: root

    property bool popupVisible: true
    property string title: ""
    property real expandedWidth: 640
    property real expandedHeight: 480
    property real collapsedSize: Math.round(36 * AppPalette.scale)
    property real popupWidth: collapsed ? collapsedSize : expandedWidth
    property real popupHeight: collapsed ? collapsedSize : expandedHeight
    property real panelX: 0
    property real panelY: 0
    property bool collapsed: false
    property bool contentHighlighted: false
    property int popupMargin: 16
    property bool dragEnabled: true
    property bool resizeEnabled: true
    property int headerDragBarLength: 0
    property bool overlayChrome: false
    property bool suspendSignals: false
    property bool collapseButtonVisible: true
    property bool fullscreenMode: false
    property color panelColor: "#0B1220"
    property color panelBorderColor: "#93C5FD77"

    readonly property real headerHeight: Math.round(32 * AppPalette.scale)
    readonly property real contentPadding: Tokens.spaceXs

    property bool  _snapActive: false
    property real  _snapPreviewX: 0
    property real  _snapPreviewY: 0

    property real  _resizePreviewW: expandedWidth
    property real  _resizePreviewH: expandedHeight

    property var   siblingBoundsList: []
    property real  siblingSnapGap: 8
    property real  siblingSnapThreshold: 60
    property bool  siblingSnapAlignTop: false

    property bool  _siblingSnapActive: false
    property real  _siblingSnapX: 0
    property real  _siblingSnapY: 0

    property bool  _chromeRevealed: false
    readonly property int chromeHideMs: 1600
    readonly property bool chromeShown: !overlayChrome
                                        || _chromeRevealed
                                        || headerDrag.active
                                        || resizeDrag.active

    Timer {
        id: chromeHideTimer
        interval: root.chromeHideMs
        onTriggered: root._chromeRevealed = false
    }

    Connections {
        target: root
        function onVisibleChanged() {
            if (root.visible && root.overlayChrome) {
                root._chromeRevealed = true
                chromeHideTimer.restart()
            }
        }
    }

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
        var maxX = width - expandedWidth - spacing
        if (maxX < minX) { minX = 0; maxX = Math.max(0, width - expandedWidth) }
        return Math.max(minX, Math.min(value, maxX))
    }

    function clampY(value) {
        var spacing = Math.max(0, popupMargin)
        var minY = spacing
        var maxY = height - expandedHeight - spacing
        if (maxY < minY) { minY = 0; maxY = Math.max(0, height - expandedHeight) }
        return Math.max(minY, Math.min(value, maxY))
    }

    readonly property real snapThreshold: 100

    function snapToGrid(x, y) {
        var cx = x + expandedWidth / 2
        var cy = y + expandedHeight / 2
        var W  = width,  H  = height
        var hw = expandedWidth / 2, hh = expandedHeight / 2
        var pts = [
            Qt.point(hw,     hh     ),  // top-left corner
            Qt.point(W - hw, hh     ),  // top-right corner
            Qt.point(hw,     H - hh ),  // bottom-left corner
            Qt.point(W - hw, H - hh ),  // bottom-right corner
            Qt.point(W * 0.5, H * 0.5) // center
        ]
        var best2 = snapThreshold * snapThreshold
        var bx = x, by = y, found = false
        for (var i = 0; i < pts.length; i++) {
            var dx = cx - pts[i].x, dy = cy - pts[i].y
            var d2 = dx * dx + dy * dy
            if (d2 < best2) { best2 = d2; bx = pts[i].x; by = pts[i].y; found = true }
        }
        return found ? Qt.point(clampX(bx - hw), clampY(by - hh))
                     : Qt.point(x, y)
    }

    function _overlapsAnySibling(x, y) {
        var list = siblingBoundsList
        if (!list) return false
        var aw = expandedWidth, ah = expandedHeight
        for (var i = 0; i < list.length; i++) {
            var b = list[i]
            if (!b || b.width <= 0 || b.height <= 0) continue
            if (x < b.x + b.width && x + aw > b.x && y < b.y + b.height && y + ah > b.y)
                return true
        }
        return false
    }

    function _snapForBounds(x, y, b) {
        var bw = b.width, bh = b.height
        if (bw <= 0 || bh <= 0) return null
        var bx = b.x, by = b.y
        var aw = expandedWidth, ah = expandedHeight
        var gap = siblingSnapGap
        var thr = siblingSnapThreshold
        var leftTarget   = bx - gap - aw
        var rightTarget  = bx + bw + gap
        var topTarget    = by - gap - ah
        var bottomTarget = by + bh + gap
        var overlapping = (x < bx + bw && x + aw > bx && y < by + bh && y + ah > by)
        var effectiveThr = overlapping ? 1e9 : thr
        var yCenterDist = Math.abs((y + ah / 2) - (by + bh / 2))
        var xCenterDist = Math.abs((x + aw / 2) - (bx + bw / 2))
        var yProximity = overlapping || (yCenterDist < (ah / 2 + bh / 2 + thr))
        var xProximity = overlapping || (xCenterDist < (aw / 2 + bw / 2 + thr))
        var snapY = siblingSnapAlignTop
                    ? ((Math.abs(y - by) < thr) ? clampY(by) : clampY(y))
                    : ((yCenterDist < thr) ? clampY(by + bh / 2 - ah / 2) : clampY(y))
        var snapX = (xCenterDist < thr) ? clampX(bx + bw / 2 - aw / 2) : clampX(x)
        var dLeft   = Math.abs(x - leftTarget)
        var dRight  = Math.abs(x - rightTarget)
        var dTop    = Math.abs(y - topTarget)
        var dBottom = Math.abs(y - bottomTarget)
        var lx = clampX(leftTarget),  rx = clampX(rightTarget)
        var ty = clampY(topTarget),   by2 = clampY(bottomTarget)
        var best = null, bestDist = effectiveThr
        if (yProximity) {
            if (dLeft  < bestDist && !(lx < bx+bw && lx+aw > bx && snapY < by+bh && snapY+ah > by))
                { bestDist = dLeft;  best = Qt.point(lx, snapY) }
            if (dRight < bestDist && !(rx < bx+bw && rx+aw > bx && snapY < by+bh && snapY+ah > by))
                { bestDist = dRight; best = Qt.point(rx, snapY) }
        }
        if (xProximity) {
            if (dTop    < bestDist && !(snapX < bx+bw && snapX+aw > bx && ty  < by+bh && ty+ah  > by))
                { bestDist = dTop;    best = Qt.point(snapX, ty) }
            if (dBottom < bestDist && !(snapX < bx+bw && snapX+aw > bx && by2 < by+bh && by2+ah > by))
                { bestDist = dBottom; best = Qt.point(snapX, by2) }
        }
        return best === null ? null : { point: best, dist: bestDist }
    }

    function computeSiblingSnap(x, y) {
        var list = siblingBoundsList
        if (!list || !list.length) return null
        var winner = null, winnerDist = Infinity
        for (var i = 0; i < list.length; i++) {
            var b = list[i]
            if (!b) continue
            var r = _snapForBounds(x, y, b)
            if (r !== null && r.dist < winnerDist && !_overlapsAnySibling(r.point.x, r.point.y)) {
                winnerDist = r.dist
                winner = r.point
            }
        }
        return winner
    }

    function resolveOverlapWithSibling() {
        if (!popupVisible) return
        var list = siblingBoundsList
        if (!list || !list.length) return
        if (!_overlapsAnySibling(panelX, panelY)) return
        var aw = expandedWidth, ah = expandedHeight
        var gap = siblingSnapGap
        var candidates = []
        for (var i = 0; i < list.length; i++) {
            var b = list[i]
            if (!b || b.width <= 0 || b.height <= 0) continue
            candidates.push(Qt.point(clampX(b.x - gap - aw), panelY))
            candidates.push(Qt.point(panelX, clampY(b.y - gap - ah)))
            candidates.push(Qt.point(clampX(b.x + b.width + gap), panelY))
            candidates.push(Qt.point(panelX, clampY(b.y + b.height + gap)))
        }
        var best = null, bestDist = Infinity
        for (var j = 0; j < candidates.length; j++) {
            var c = candidates[j]
            if (_overlapsAnySibling(c.x, c.y)) continue
            var d = Math.abs(c.x - panelX) + Math.abs(c.y - panelY)
            if (d < bestDist) { bestDist = d; best = c }
        }
        if (best === null) return
        panelX = best.x
        panelY = best.y
        commitPosition()
    }

    function commitPosition() {
        if (suspendSignals) return
        positionCommitted(panelX, panelY, expandedWidth, expandedHeight)
    }

    function toggleCollapsedFromButton() {
        collapsed = !collapsed
        commitPosition()
    }

    onCollapsedChanged: {
        panelX = clampX(panelX)
        panelY = clampY(panelY)
        if (suspendSignals) return
        collapsedToggled(collapsed)
    }

    readonly property var resizeSnapSizes: [
        Qt.size(320, 240),
        Qt.size(480, 360),
        Qt.size(640, 480),
        Qt.size(800, 600),
        Qt.size(960, 720)
    ]

    onExpandedWidthChanged:  panelX = clampX(panelX)
    onExpandedHeightChanged: panelY = clampY(panelY)

    property real lastParentWidth: 0
    property real lastParentHeight: 0

    function rescaleX(oldParentWidth) {
        var sp = Math.max(0, popupMargin)
        var oldMin = sp; var oldMax = oldParentWidth - expandedWidth - sp
        if (oldMax < oldMin) { oldMin = 0; oldMax = Math.max(0, oldParentWidth - expandedWidth) }
        var t = (oldMax > oldMin) ? Math.max(0, Math.min(1, (panelX - oldMin) / (oldMax - oldMin))) : 0
        var newMin = sp; var newMax = width - expandedWidth - sp
        if (newMax < newMin) { newMin = 0; newMax = Math.max(0, width - expandedWidth) }
        panelX = clampX(newMin + t * (newMax - newMin))
    }

    function rescaleY(oldParentHeight) {
        var sp = Math.max(0, popupMargin)
        var oldMin = sp; var oldMax = oldParentHeight - expandedHeight - sp
        if (oldMax < oldMin) { oldMin = 0; oldMax = Math.max(0, oldParentHeight - expandedHeight) }
        var t = (oldMax > oldMin) ? Math.max(0, Math.min(1, (panelY - oldMin) / (oldMax - oldMin))) : 0
        var newMin = sp; var newMax = height - expandedHeight - sp
        if (newMax < newMin) { newMin = 0; newMax = Math.max(0, height - expandedHeight) }
        panelY = clampY(newMin + t * (newMax - newMin))
    }

    onWidthChanged: {
        if (width <= 0) return
        if (lastParentWidth > 0) rescaleX(lastParentWidth)
        else panelX = clampX(panelX)
        lastParentWidth = width
        Qt.callLater(commitPosition)
        Qt.callLater(resolveOverlapWithSibling)
    }
    onHeightChanged: {
        if (height <= 0) return
        if (lastParentHeight > 0) rescaleY(lastParentHeight)
        else panelY = clampY(panelY)
        lastParentHeight = height
        Qt.callLater(commitPosition)
        Qt.callLater(resolveOverlapWithSibling)
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
        id: resizeGhost
        visible: root.popupVisible && resizeDrag.active
        x: root.panelX
        y: root.panelY
        width: root._resizePreviewW
        height: root._resizePreviewH
        radius: 10
        color: "transparent"
        border.color: "#93C5FD"
        border.width: 3
        opacity: 0.75
        z: 2
    }

    Rectangle {
        id: snapGhost
        visible: root.popupVisible && (root._snapActive || root._siblingSnapActive)
        x: root._siblingSnapActive ? root._siblingSnapX : root._snapPreviewX
        y: root._siblingSnapActive ? root._siblingSnapY : root._snapPreviewY
        width: root.expandedWidth
        height: root.expandedHeight
        radius: 10
        color: "transparent"
        border.color: root._siblingSnapActive ? "#86EFAC" : "#93C5FD"
        border.width: 2
        opacity: 0.55
        z: 0
    }

    Rectangle {
        id: panel

        visible: root.popupVisible
        x: root.collapsed ? root.panelX + root.expandedWidth - root.collapsedSize : root.panelX
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

        HoverHandler {
            id: chromeHover
            onHoveredChanged: {
                if (hovered) {
                    root._chromeRevealed = true
                    chromeHideTimer.stop()
                } else {
                    chromeHideTimer.restart()
                }
            }
        }

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
                    easing.type: Easing.OutCubic
                }
            }
        ]

        // Blocks pointer/wheel events from leaking under the popup.
        // The mouse-path double-click still fires via MouseArea.onDoubleClicked
        // (works fine with a real mouse on desktop); for touch we use a
        // sibling TapHandler that has a touch-friendly recognizer.
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

        TapHandler {
            acceptedDevices: PointerDevice.TouchScreen
            onDoubleTapped: root.popupDoubleClicked()
        }

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
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            Item {
                id: dragGrip
                anchors.horizontalCenter: parent.horizontalCenter   // строго по центру сверху
                anchors.verticalCenter: parent.verticalCenter
                width: grip.width
                height: parent.height
                opacity: (root.chromeShown && !root.collapsed) ? 1.0 : 0.0
                visible: opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }

                KDragBar {
                    id: grip
                    anchors.centerIn: parent
                    orientation: "horizontal"
                    barColor: root.overlayChrome ? "transparent" : AppPalette.card
                    barLength: root.headerDragBarLength > 0
                               ? root.headerDragBarLength
                               : Math.round(AppPalette.dragBarLengthPx * AppPalette.scale)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    propagateComposedEvents: false
                    hoverEnabled: false
                    cursorShape: Qt.OpenHandCursor
                    onPressed:       function(mouse) { mouse.accepted = true }
                    onReleased:      function(mouse) { mouse.accepted = true }
                    onClicked:       function(mouse) { mouse.accepted = true }
                    onDoubleClicked: function(mouse) { mouse.accepted = true }
                    onWheel:         function(wheel)  { wheel.accepted = false }
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
                        if (root._siblingSnapActive) {
                            root.panelX = Math.round(root._siblingSnapX)
                            root.panelY = Math.round(root._siblingSnapY)
                        } else if (root._snapActive) {
                            root.panelX = Math.round(root._snapPreviewX)
                            root.panelY = Math.round(root._snapPreviewY)
                        }
                        root._siblingSnapActive = false
                        root._snapActive = false
                        root.commitPosition()
                        Qt.callLater(root.resolveOverlapWithSibling)
                    }
                }

                onTranslationChanged: {
                    if (!active) return
                    var rawX = root.clampX(startX + translation.x)
                    var rawY = root.clampY(startY + translation.y)
                    var sibSnap = root.computeSiblingSnap(rawX, rawY)
                    if (sibSnap !== null) {
                        root._siblingSnapActive = true
                        root._siblingSnapX = sibSnap.x
                        root._siblingSnapY = sibSnap.y
                        root._snapActive = false
                    } else {
                        root._siblingSnapActive = false
                        var snapped = root.snapToGrid(rawX, rawY)
                        var isSnapping = Math.abs(snapped.x - rawX) > 0.1 || Math.abs(snapped.y - rawY) > 0.1
                        root._snapActive   = isSnapping
                        root._snapPreviewX = snapped.x
                        root._snapPreviewY = snapped.y
                    }
                    root.panelX = Math.round(rawX)
                    root.panelY = Math.round(rawY)
                }
                }
            }

            KButton {
                id: collapseButton
                opacity: (root.collapsed || root.chromeShown) ? 1.0 : 0.0
                visible: root.collapseButtonVisible && opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
                width: Math.round(32 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: Tokens.spaceXxs
                text: root.collapsed ? "\u25A1" : "\u2014"
                fontPixelSize: Math.round(15 * AppPalette.scale)
                horizontalPadding: 0
                verticalPadding: 0
                cornerRadius: Tokens.radiusMd
                normalBg: root.overlayChrome ? "transparent" : "#1E293B"
                hoverBg: root.overlayChrome ? "#0F172A99" : "#0F172A"
                normalBorder: "#334155"
                hoverBorder: "#475569"
                textColor: "#E2E8F0"
                z: 6
                onClicked: root.toggleCollapsedFromButton()
            }
        }

        Item {
            id: contentHost

            anchors.fill: parent
            anchors.leftMargin: root.fullscreenMode ? 0 : root.contentPadding
            anchors.rightMargin: root.fullscreenMode ? 0 : root.contentPadding
            anchors.topMargin: root.fullscreenMode ? 0 : (root.overlayChrome ? root.contentPadding : root.headerHeight)
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

        Rectangle {
            anchors.fill: contentHost
            color: "#FFFFFF"
            opacity: root.contentHighlighted && !root.collapsed ? 0.16 : 0.0
            visible: opacity > 0
            z: 3
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }

        Item {
            id: resizeHandle
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.round(26 * AppPalette.scale)
            height: Math.round(26 * AppPalette.scale)
            opacity: root.chromeShown ? 1.0 : 0.0
            visible: !root.collapsed && !root.fullscreenMode && root.resizeEnabled && opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
            z: 10

            // Grip: 6 dots in bottom-right triangle pattern
            Repeater {
                model: [{x:13,y:3},{x:8,y:8},{x:13,y:8},{x:3,y:13},{x:8,y:13},{x:13,y:13}]
                Rectangle {
                    x: modelData.x * AppPalette.scale
                    y: modelData.y * AppPalette.scale
                    width: Math.max(1, Math.round(2 * AppPalette.scale))
                    height: Math.max(1, Math.round(2 * AppPalette.scale))
                    radius: Math.max(1, Math.round(1 * AppPalette.scale))
                    color: resizeDrag.active ? "#93C5FD" : (resizeHover.hovered ? "#94A3B8" : "#475569")
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                propagateComposedEvents: false
                hoverEnabled: false
                cursorShape: Qt.SizeFDiagCursor
                onPressed:       function(mouse) { mouse.accepted = true }
                onReleased:      function(mouse) { mouse.accepted = true }
                onClicked:       function(mouse) { mouse.accepted = true }
                onDoubleClicked: function(mouse) { mouse.accepted = true }
                onWheel:         function(wheel)  { wheel.accepted = false }
            }

            HoverHandler {
                id: resizeHover
            }

            DragHandler {
                id: resizeDrag
                target: null

                property real startW: 0
                property real startH: 0

                onActiveChanged: {
                    if (active) {
                        startW = root.expandedWidth
                        startH = root.expandedHeight
                        root._resizePreviewW = root.expandedWidth
                        root._resizePreviewH = root.expandedHeight
                    } else {
                        root.expandedWidth  = root._resizePreviewW
                        root.expandedHeight = root._resizePreviewH
                        root.sizeCommitted(root.expandedWidth, root.expandedHeight)
                        Qt.callLater(root.resolveOverlapWithSibling)
                    }
                }

                onTranslationChanged: {
                    if (!active) return
                    var sizes = root.resizeSnapSizes
                    var rawW = Math.max(sizes[0].width,  Math.min(sizes[sizes.length-1].width,  startW + translation.x))
                    var rawH = Math.max(sizes[0].height, Math.min(sizes[sizes.length-1].height, startH + translation.y))
                    var best = sizes[0]
                    var bestD = (rawW - best.width) * (rawW - best.width) + (rawH - best.height) * (rawH - best.height)
                    for (var i = 1; i < sizes.length; i++) {
                        var d = (rawW - sizes[i].width) * (rawW - sizes[i].width) + (rawH - sizes[i].height) * (rawH - sizes[i].height)
                        if (d < bestD) { bestD = d; best = sizes[i] }
                    }
                    root._resizePreviewW = best.width
                    root._resizePreviewH = best.height
                }
            }
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    property var layoutSnapshot: null
    property var popupLinks: []
    property color frameColor: AppPalette.border
    property color baseColor: AppPalette.bgDeep
    property real frameRadius: 6
    property real frameWidth: 1
    property int contentMargin: 4
    property real splitGap: 4
    property color pane2DFill: "#2563EB"
    property color pane2DStroke: AppPalette.accentBorder
    property color pane3DFill: "#16A34A"
    property color pane3DStroke: "#86EFAC"
    property color popupFill: "#EF4444"
    property color popupStroke: "#FCA5A5"
    property real popupMinPaneSize: 8
    property int popupMinMarkSize: 3
    property int redrawDebounceMs: 16

    radius: root.frameRadius
    color: root.baseColor
    border.width: root.frameWidth
    border.color: root.frameColor

    function scheduleRedraw() {
        if (root.redrawDebounceMs <= 0) {
            previewCanvas.requestPaint()
            return
        }
        redrawTimer.restart()
    }

    onLayoutSnapshotChanged: scheduleRedraw()
    onPopupLinksChanged: scheduleRedraw()
    onWidthChanged: scheduleRedraw()
    onHeightChanged: scheduleRedraw()
    Component.onCompleted: scheduleRedraw()

    Timer {
        id: redrawTimer
        interval: root.redrawDebounceMs
        repeat: false
        onTriggered: previewCanvas.requestPaint()
    }

    Canvas {
        id: previewCanvas
        anchors.fill: parent
        anchors.margins: root.contentMargin

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var gap = Math.max(0, root.splitGap)
            var popupHosts = {}

            if (Array.isArray(root.popupLinks)) {
                for (var i = 0; i < root.popupLinks.length; ++i) {
                    var link = root.popupLinks[i]
                    if (!link || typeof link !== "object")
                        continue
                    var hostPaneId = Math.round(link.hostPaneId)
                    if (hostPaneId > 0)
                        popupHosts[hostPaneId] = true
                }
            }

            function pane(x, y, w, h, mode) {
                var is3D = mode === "3D"
                ctx.fillStyle = is3D ? root.pane3DFill : root.pane2DFill
                ctx.strokeStyle = is3D ? root.pane3DStroke : root.pane2DStroke
                ctx.lineWidth = 1
                ctx.fillRect(x, y, w, h)
                if (w > 1 && h > 1)
                    ctx.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1)
            }

            function popupMark(x, y, w, h) {
                if (w < root.popupMinPaneSize || h < root.popupMinPaneSize)
                    return
                var size = Math.max(root.popupMinMarkSize, Math.min(8, Math.floor(Math.min(w, h) * 0.25)))
                var px = x + w - size - 2
                var py = y + 2
                ctx.fillStyle = root.popupFill
                ctx.fillRect(px, py, size, size)
                ctx.strokeStyle = root.popupStroke
                ctx.lineWidth = 1
                ctx.strokeRect(px + 0.5, py + 0.5, size - 1, size - 1)
            }

            function drawNode(node, x, y, w, h) {
                if (!node)
                    return

                if (node.type === "leaf") {
                    pane(x, y, w, h, node.mode === "3D" ? "3D" : "2D")
                    var paneId = typeof node.paneId === "number" ? Math.round(node.paneId) : -1
                    if (paneId > 0 && popupHosts[paneId] === true)
                        popupMark(x, y, w, h)
                    return
                }

                var ratio = typeof node.ratio === "number" ? node.ratio : 0.5
                ratio = Math.max(0.001, Math.min(0.999, ratio))

                if (node.orientation === "vertical") {
                    var firstWidth = Math.max(0, w * ratio - gap / 2)
                    var secondWidth = Math.max(0, w - firstWidth - gap)
                    drawNode(node.first, x, y, firstWidth, h)
                    drawNode(node.second, x + firstWidth + gap, y, secondWidth, h)
                } else {
                    var firstHeight = Math.max(0, h * ratio - gap / 2)
                    var secondHeight = Math.max(0, h - firstHeight - gap)
                    drawNode(node.first, x, y, w, firstHeight)
                    drawNode(node.second, x, y + firstHeight + gap, w, secondHeight)
                }
            }

            drawNode(root.layoutSnapshot, 0, 0, width, height)
        }
    }
}

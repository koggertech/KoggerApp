import QtQuick 2.15
import kqml_types 1.0

// What the HOST is doing about a node: off, idle, or a request in flight.
// Takes the codes UsblNodeLogic.operationCode returns; see
// docs/KoggerApp-Docs/usbl-node-row.md for what the three marks mean.
Rectangle {
    id: badge

    property string code: "idle"            // "off" | "idle" | "waiting"
    // Square, and sized by the caller: the settings pane is in AppPalette.scale
    // space and the on-scene panel is in appScale space.
    property real diameter: Tokens.chipH

    readonly property bool _live: badge.code === "waiting"
    // Repaint trigger. A theme switch can change the mark's ink without changing
    // anything else the Canvas is bound to, and a Canvas does not track that.
    readonly property color _ink: _live ? AppPalette.accentText : AppPalette.textMuted

    width: diameter
    height: diameter
    radius: Tokens.radiusSm
    color: _live ? AppPalette.accent : "transparent"
    border.width: _live ? 0 : Math.max(1, Math.round(1 * AppPalette.scale))
    border.color: AppPalette.border
    opacity: badge.code === "off" ? 0.62 : 1.0
    Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }

    Canvas {
        id: mark
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return
            ctx.clearRect(0, 0, width, height)
            var cx = width / 2, cy = height / 2, d = badge.diameter

            if (badge.code === "off") {
                // Drawn as arc segments rather than with a dash pattern: Context2D's
                // dash support is not uniform across the Qt versions this builds on,
                // and a silently-solid ring would read as Idle.
                ctx.strokeStyle = AppPalette.textMuted
                ctx.lineWidth = Math.max(1, d * 0.0625)
                var r = d * 0.233
                for (var i = 0; i < 6; ++i) {
                    var a0 = i * Math.PI / 3 + 0.18
                    ctx.beginPath()
                    ctx.arc(cx, cy, r, a0, a0 + Math.PI / 3 - 0.36)
                    ctx.stroke()
                }
                return
            }

            if (badge.code === "waiting") {
                // An emitter: a dot with one ring leaving it. Ink is accentText, so
                // the mark stays legible on whatever accent the theme resolves to.
                ctx.fillStyle = AppPalette.accentText
                ctx.strokeStyle = AppPalette.accentText
                ctx.beginPath()
                ctx.arc(cx, cy, d * 0.1, 0, 2 * Math.PI)
                ctx.fill()
                ctx.lineWidth = Math.max(1, d * 0.0667)
                ctx.beginPath()
                ctx.arc(cx, cy, d * 0.267, 0, 2 * Math.PI)
                ctx.stroke()
                return
            }

            ctx.fillStyle = AppPalette.textMuted
            ctx.beginPath()
            ctx.arc(cx, cy, d * 0.233, 0, 2 * Math.PI)
            ctx.fill()
        }
    }

    onCodeChanged: mark.requestPaint()
    onDiameterChanged: mark.requestPaint()
    onWidthChanged: mark.requestPaint()
    onHeightChanged: mark.requestPaint()
    on_InkChanged: mark.requestPaint()
    Component.onCompleted: mark.requestPaint()
}

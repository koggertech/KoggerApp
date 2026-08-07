import QtQuick 2.15
import kqml_types 1.0

// A command prompt: a chevron and an underscore. Wherever a command id is shown -- the panel's
// status bar, the pane's chips, the pane's extended rows -- this is the mark in front of it, so
// the id is recognisable as a command without the word.
//
// A lone chevron was drawn and rejected: DisclosureIndicator draws exactly that shape in the
// pane's own node row, and one shape may not mean two things.
Canvas {
    id: mark

    property color ink: AppPalette.text

    antialiasing: true

    onPaint: {
        var ctx = getContext("2d")
        if (!ctx)
            return
        ctx.clearRect(0, 0, width, height)
        var s = Math.min(width, height)
        ctx.strokeStyle = mark.ink
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.lineWidth = Math.max(1.2, s * 0.129)
        ctx.beginPath()
        ctx.moveTo(s * 0.186, s * 0.257)
        ctx.lineTo(s * 0.471, s * 0.5)
        ctx.lineTo(s * 0.186, s * 0.743)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(s * 0.557, s * 0.771)
        ctx.lineTo(s * 0.871, s * 0.771)
        ctx.stroke()
    }

    onInkChanged: mark.requestPaint()
    onWidthChanged: mark.requestPaint()
    onHeightChanged: mark.requestPaint()
    Component.onCompleted: mark.requestPaint()
}

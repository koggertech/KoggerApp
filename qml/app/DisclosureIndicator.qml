import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property bool expanded: false
    property color indicatorColor: AppPalette.textSecond

    implicitWidth: 10
    implicitHeight: 10

    Canvas {
        id: indicatorCanvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = root.indicatorColor
            ctx.beginPath()

            if (root.expanded) {
                // Down triangle.
                ctx.moveTo(1, 3)
                ctx.lineTo(width - 1, 3)
                ctx.lineTo(width / 2, height - 1)
            } else {
                // Right triangle.
                ctx.moveTo(3, 1)
                ctx.lineTo(width - 1, height / 2)
                ctx.lineTo(3, height - 1)
            }

            ctx.closePath()
            ctx.fill()
        }
    }

    onExpandedChanged: indicatorCanvas.requestPaint()
    onIndicatorColorChanged: indicatorCanvas.requestPaint()
    onWidthChanged: indicatorCanvas.requestPaint()
    onHeightChanged: indicatorCanvas.requestPaint()
}

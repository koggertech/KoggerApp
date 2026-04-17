import QtQuick 2.15
import QtQuick.Controls 2.15
import "../controls"
import "../menus"

Slider {
    id: control

    property color borderColor: theme.textColor
    property color handleColor: control.pressed ? theme.textSolidColor : borderColor
    property real handleLengthScale: 0.68
    property real handleThicknessScale: 1.25
    property int handleLength: Math.max(14, Math.round(theme.controlHeight * 0.76 * handleLengthScale))
    property int handleThickness: Math.max(8, Math.round(theme.controlHeight * 0.76 * handleThicknessScale))
    property int lineInset: Math.max(2, Math.round(theme.controlHeight * 0.08))

    from: 0
    to: 100
    value: 0
    stepSize: 1
    snapMode: Slider.SnapAlways
    orientation: Qt.Horizontal
    live: true
    touchDragThreshold: 0
    implicitWidth: Math.round(theme.controlHeight * 6.0)
    implicitHeight: Math.max(handleThickness + 4, Math.round(theme.controlHeight * 1.02))
    leftPadding: Math.round(handleLength * 0.65)
    rightPadding: Math.round(handleLength * 0.65)
    topPadding: 0
    bottomPadding: 0

    function repaint() {
        overlay.requestPaint()
    }

    onVisualPositionChanged: repaint()
    onPressedChanged: repaint()
    onWidthChanged: repaint()
    onHeightChanged: repaint()
    Component.onCompleted: repaint()

    Connections {
        target: theme

        function onThemeIDChanged() {
            control.repaint()
        }
    }

    background: Item {}

    handle: Item {
        implicitWidth: control.handleLength
        implicitHeight: control.implicitHeight
    }

    Canvas {
        id: overlay
        anchors.fill: parent
        contextType: "2d"

        onPaint: {
            context.reset()
            context.lineWidth = 1
            context.strokeStyle = control.borderColor
            context.fillStyle = control.handleColor

            const centerY = Math.round(height / 2)
            const centerX = Math.round(control.leftPadding + control.visualPosition * control.availableWidth)
            const halfLength = Math.max(8, Math.round(control.handleLength / 2))
            const quarterLength = Math.max(4, Math.round(control.handleLength / 4))
            const halfThickness = Math.max(4, Math.round(control.handleThickness / 2))
            const lineLeft = control.lineInset
            const lineRight = Math.max(lineLeft, Math.round(width - control.lineInset))
            const handleLeft = centerX - halfLength
            const handleRight = centerX + halfLength
            const bodyLeft = centerX - quarterLength
            const bodyRight = centerX + quarterLength

            context.beginPath()
            context.moveTo(lineLeft, centerY)
            context.lineTo(Math.max(lineLeft, handleLeft), centerY)
            context.moveTo(Math.min(lineRight, handleRight), centerY)
            context.lineTo(lineRight, centerY)
            context.stroke()

            context.beginPath()
            context.moveTo(handleLeft, centerY)
            context.lineTo(bodyLeft, centerY - halfThickness)
            context.lineTo(bodyRight, centerY - halfThickness)
            context.lineTo(handleRight, centerY)
            context.lineTo(bodyRight, centerY + halfThickness)
            context.lineTo(bodyLeft, centerY + halfThickness)
            context.closePath()
            context.fill()
            context.stroke()
        }
    }
}




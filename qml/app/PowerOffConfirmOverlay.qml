import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property bool active: false
    signal confirmed()

    anchors.fill: parent
    z: ZOrder.powerOffOverlay
    visible: opacity > 0.01
    enabled: active
    opacity: active ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    onActiveChanged: if (!active) knob.x = bar._inset

    Rectangle {
        anchors.fill: parent
        color: AppPalette.dim
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onClicked: root.active = false
            onWheel: function(w) { w.accepted = true }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Tokens.spaceLg

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Slide to power off")
            color: AppPalette.text
            font.pixelSize: Tokens.fontLg
            font.bold: true
        }

        Rectangle {
            id: bar
            readonly property int _inset: Math.round(4 * AppPalette.scale)
            readonly property int _knob: height - 2 * _inset
            readonly property real _maxX: width - _knob - _inset

            width: Math.min(root.width - 2 * Tokens.spaceXl, Math.round(360 * AppPalette.scale))
            height: Math.round(64 * AppPalette.scale)
            radius: height / 2
            color: AppPalette.card
            border.width: 1
            border.color: AppPalette.dangerBorder

            MouseArea { anchors.fill: parent; onClicked: {} }

            Canvas {
                id: arrowCanvas
                anchors.centerIn: parent
                width: Math.round(bar.height * 0.58)
                height: Math.round(bar.height * 0.36)
                onPaint: {
                    var ctx = getContext("2d")
                    if (!ctx)
                        return
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = AppPalette.textMuted
                    var lw = Math.max(2, Math.round(height * 0.16))
                    ctx.lineWidth = lw
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    var pad = lw
                    var midY = height / 2
                    var tipX = width - pad
                    var head = midY - pad
                    ctx.beginPath()
                    ctx.moveTo(pad, midY)
                    ctx.lineTo(tipX, midY)
                    ctx.moveTo(tipX - head, midY - head)
                    ctx.lineTo(tipX, midY)
                    ctx.lineTo(tipX - head, midY + head)
                    ctx.stroke()
                }
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()
            }

            Rectangle {
                id: knob
                x: bar._inset
                anchors.verticalCenter: parent.verticalCenter
                width: bar._knob
                height: bar._knob
                radius: width / 2
                color: knobMouse.drag.active ? AppPalette.dangerHover : AppPalette.dangerBorder

                Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

                MouseArea {
                    id: knobMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    drag.target: knob
                    drag.axis: Drag.XAxis
                    drag.minimumX: bar._inset
                    drag.maximumX: bar._maxX
                    onReleased: {
                        if (knob.x >= bar._maxX - 1) {
                            root.confirmed()
                            root.active = false
                        } else {
                            knob.x = bar._inset
                        }
                    }
                }
            }
        }
    }
}

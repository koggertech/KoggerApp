import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

Item {
    id: root

    property real deadlineMs: 0
    property int remainingMs: 0
    property bool running: true
    property string toolTipText: ""
    property real hitPadding: 0
    property int glyphPixelSize: Math.round(Math.min(width, height) * 0.45)
    property color ringColor: AppPalette.accent
    property real ringWidth: Math.max(2, Math.round(Math.min(width, height) * 0.08))

    property real _now: Date.now()
    readonly property bool _counting: running && deadlineMs > 0
    readonly property real leftMs: _counting ? Math.max(0, deadlineMs - _now) : Math.max(0, remainingMs)
    readonly property int seconds: Math.max(1, Math.ceil(leftMs / 1000))
    readonly property real secondFraction: _counting && leftMs > 0 ? ((Math.ceil(leftMs) - 1) % 1000 + 1) / 1000 : 1.0

    signal clicked()

    onDeadlineMsChanged: _now = Date.now()
    onRunningChanged: _now = Date.now()

    FrameAnimation {
        running: root._counting && root.visible
        onTriggered: root._now = Date.now()
    }

    KCircleIconButton {
        id: button
        anchors.fill: parent
        hitPadding: root.hitPadding
        fillColor: AppPalette.card
        fillHoverColor: AppPalette.cardHover
        borderWidth: 0
        toolTipText: root.toolTipText
        onClicked: root.clicked()
    }

    Text {
        id: digit
        x: Math.ceil(root.width / 2 - (digitMetrics.tightBoundingRect.x + digitMetrics.tightBoundingRect.width / 2))
        y: Math.round(root.height / 2 - (baselineOffset + digitMetrics.tightBoundingRect.y + digitMetrics.tightBoundingRect.height / 2))
        text: String(root.seconds)
        color: button.hovered ? Qt.lighter(AppPalette.text, Anim.hoverLighten) : AppPalette.text
        font.pixelSize: root.glyphPixelSize
        font.bold: true
    }

    TextMetrics {
        id: digitMetrics
        font: digit.font
        text: digit.text
    }

    Shape {
        anchors.fill: parent
        scale: button.backgroundScale
        preferredRendererType: Shape.CurveRenderer
        visible: root.secondFraction > 0.001

        ShapePath {
            strokeColor: root.ringColor
            strokeWidth: root.ringWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (Math.min(root.width, root.height) - root.ringWidth) / 2
                radiusY: (Math.min(root.width, root.height) - root.ringWidth) / 2
                startAngle: -90 + 360 * (1 - root.secondFraction)
                sweepAngle: 360 * root.secondFraction
            }
        }
    }
}

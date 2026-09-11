import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var controller: null
    property real maxTotalHeight: 0

    readonly property real step: controller ? controller.isobathStep : 0
    readonly property bool banded: controller ? controller.bandedColors : false
    readonly property var paletteColors: controller ? controller.surfacePaletteColors : []
    readonly property real depthMin: controller ? controller.surfaceDepthMin : -1
    readonly property real depthMax: controller ? controller.surfaceDepthMax : -1
    readonly property real depthSpan: depthMax - depthMin
    readonly property bool hasData: step > 0 && paletteColors.length > 1 && isFinite(depthMin) && isFinite(depthMax) && depthSpan > 1e-3

    readonly property real barWidth: Tokens.iconMd
    readonly property real barHeight: Math.max(Tokens.controlHXl * 2,
                                               Math.min(Tokens.controlHXl * 6, maxTotalHeight))
    readonly property real pad: Tokens.spaceSm
    readonly property real labelGap: Tokens.spaceXs
    readonly property real tickLength: Tokens.spaceXs
    readonly property real minLabelSpacing: Tokens.fontBase * 1.6
    readonly property real pxPerMetre: hasData ? barHeight / depthSpan : 0
    readonly property real labelStep: hasData ? step * Math.max(1, Math.ceil(minLabelSpacing / (step * pxPerMetre))) : 0
    readonly property int decimals: Math.abs(step - Math.round(step)) > 1e-4 ? 1 : 0
    readonly property real outlinePx: Math.max(1, Math.round(AppPalette.scale))

    readonly property var ticks: {
        if (!hasData)
            return []
        var list = []
        var first = Math.ceil(depthMin / labelStep) * labelStep
        var edgeGuard = minLabelSpacing / pxPerMetre
        for (var d = first; d <= depthMax + 1e-6; d += labelStep) {
            if (d - depthMin < edgeGuard || depthMax - d < edgeGuard)
                continue
            list.push(d)
        }
        return list
    }

    function yFor(depth) {
        return (depth - depthMin) * pxPerMetre
    }

    function colorAt(norm) {
        var n = paletteColors.length
        if (n === 0)
            return "transparent"
        var t = Math.max(0, Math.min(Math.max(0, Math.min(1, norm)) * n - 0.5, n - 1))
        var i0 = Math.floor(t)
        var i1 = Math.min(i0 + 1, n - 1)
        var c0 = Qt.color(paletteColors[i0])
        var c1 = Qt.color(paletteColors[i1])
        var l = t - i0
        return Qt.rgba(c0.r + (c1.r - c0.r) * l,
                       c0.g + (c1.g - c0.g) * l,
                       c0.b + (c1.b - c0.b) * l, 1)
    }

    component OutlinedText: Item {
        id: outlined
        property string text: ""
        property real outline: root.outlinePx

        implicitWidth: coreText.implicitWidth
        implicitHeight: coreText.implicitHeight
        width: implicitWidth
        height: implicitHeight

        Repeater {
            model: [Qt.point( 1.000,  0.000), Qt.point(-1.000,  0.000),
                    Qt.point( 0.000,  1.000), Qt.point( 0.000, -1.000),
                    Qt.point( 0.707,  0.707), Qt.point( 0.707, -0.707),
                    Qt.point(-0.707,  0.707), Qt.point(-0.707, -0.707)]

            delegate: Text {
                required property point modelData

                x: modelData.x * outlined.outline
                y: modelData.y * outlined.outline
                text: outlined.text
                color: "#000000"
                font.family: theme.textFont.family
                font.pixelSize: Tokens.fontBase
            }
        }

        Text {
            id: coreText
            text: outlined.text
            color: "#ffffff"
            font.family: theme.textFont.family
            font.pixelSize: Tokens.fontBase
        }
    }

    width: backing.width
    height: backing.height

    Rectangle {
        id: backing
        color: "#00000080"
        radius: Tokens.radiusSm
        width: root.barWidth + root.tickLength + root.labelGap + labelMetrics.width + root.pad * 2 + root.outlinePx * 2
        height: root.barHeight + root.pad * 2 + labelMetrics.height + root.outlinePx * 2

        TextMetrics {
            id: labelMetrics
            font.family: theme.textFont.family
            font.pixelSize: Tokens.fontBase
            text: root.hasData ? root.depthMax.toFixed(1) : "0"
        }

        Canvas {
            id: bar
            x: root.pad
            y: root.pad + labelMetrics.height / 2 + root.outlinePx
            width: root.barWidth
            height: root.barHeight

            readonly property var stops: root.paletteColors
            readonly property bool banded: root.banded
            readonly property real bandStep: root.step
            readonly property real dMin: root.depthMin
            readonly property real dMax: root.depthMax
            readonly property real outline: root.outlinePx

            onStopsChanged: requestPaint()
            onOutlineChanged: requestPaint()
            onBandedChanged: requestPaint()
            onBandStepChanged: requestPaint()
            onDMinChanged: requestPaint()
            onDMaxChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                if (stops.length < 2 || !root.hasData)
                    return

                if (banded) {
                    var first = Math.floor(dMin / bandStep)
                    var last = Math.floor(dMax / bandStep)
                    for (var k = first; k <= last; ++k) {
                        var top = Math.max(dMin, k * bandStep)
                        var bottom = Math.min(dMax, (k + 1) * bandStep)
                        if (bottom <= top)
                            continue
                        var centre = ((k + 0.5) * bandStep - dMin) / (dMax - dMin)
                        ctx.fillStyle = root.colorAt(centre)
                        var y0 = root.yFor(top)
                        var y1 = root.yFor(bottom)
                        ctx.fillRect(0, y0, width, y1 - y0)
                    }
                }
                else {
                    var grad = ctx.createLinearGradient(0, 0, 0, height)
                    for (var i = 0; i < stops.length; ++i)
                        grad.addColorStop((i + 0.5) / stops.length, stops[i])
                    ctx.fillStyle = grad
                    ctx.fillRect(0, 0, width, height)
                }

                ctx.strokeStyle = "#000000"
                ctx.lineWidth = root.outlinePx
                ctx.strokeRect(root.outlinePx / 2, root.outlinePx / 2, width - root.outlinePx, height - root.outlinePx)
            }
        }

        Repeater {
            model: root.ticks

            delegate: Item {
                required property real modelData

                x: bar.x + bar.width
                y: bar.y + root.yFor(modelData)
                width: root.tickLength + root.labelGap + labelMetrics.width
                height: 1

                Rectangle {
                    width: root.tickLength
                    height: root.outlinePx * 3
                    color: "#000000"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: root.tickLength
                    height: root.outlinePx
                    color: "#ffffff"
                    anchors.verticalCenter: parent.verticalCenter
                }

                OutlinedText {
                    x: root.tickLength + root.labelGap
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.toFixed(root.decimals)
                }
            }
        }

        OutlinedText {
            x: bar.x + bar.width + root.tickLength + root.labelGap
            y: bar.y - labelMetrics.height / 2
            text: root.hasData ? root.depthMin.toFixed(1) : ""
        }

        OutlinedText {
            x: bar.x + bar.width + root.tickLength + root.labelGap
            y: bar.y + bar.height - labelMetrics.height / 2
            text: root.hasData ? root.depthMax.toFixed(1) : ""
        }
    }
}

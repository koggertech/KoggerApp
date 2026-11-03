import QtQuick
import QtQuick.Shapes

Shape {
    id: symbol

    property string label: ""
    property color color: "white"
    property real lineWidth: Math.max(1.5, width * 0.16)
    readonly property string path: {
        var s = width
        var c = s / 2
        switch (label) {
        case "cross": {
            var a = s * 0.1
            var b = s * 0.9
            return "M " + a + " " + a + " L " + b + " " + b + " M " + b + " " + a + " L " + a + " " + b
        }
        case "circle":
            return "M 0 " + c + " A " + c + " " + c + " 0 1 0 " + s + " " + c
                 + " A " + c + " " + c + " 0 1 0 0 " + c
        case "square": {
            var lo = s * 0.08
            var hi = s * 0.92
            return "M " + lo + " " + lo + " L " + hi + " " + lo + " L " + hi + " " + hi + " L " + lo + " " + hi + " Z"
        }
        case "triangle": {
            var h = s * 0.92
            var half = h / Math.sqrt(3)
            var apex = c - h * 2 / 3
            var base = c + h / 3
            return "M " + c + " " + apex + " L " + (c + half) + " " + base + " L " + (c - half) + " " + base + " Z"
        }
        default:
            return ""
        }
    }

    height: width
    preferredRendererType: Shape.CurveRenderer

    ShapePath {
        strokeColor: symbol.color
        strokeWidth: symbol.lineWidth
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin

        PathSvg { path: symbol.path }
    }
}

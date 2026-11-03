import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

Row {
    id: root

    property int percent: -1
    property string powerState: ""
    readonly property bool charging: powerState === "charging" || powerState === "charged"
    readonly property bool known: percent >= 0 || charging
    property real unit: AppPalette.scale
    property real glyphScale: 1.4
    property int fontPixelSize: Tokens.fontSm
    property int warnPercent: 30
    property int criticalPercent: 10
    readonly property bool low: percent >= 0 && percent <= warnPercent && !charging
    readonly property color levelColor: percent < 0 ? AppPalette.textSecond
                                      : percent <= criticalPercent && !charging ? AppPalette.dangerText
                                      : low ? AppPalette.linkIdleText
                                      : AppPalette.linkOkText

    spacing: Math.round(4 * unit)

    KBatteryGlyph {
        anchors.verticalCenter: parent.verticalCenter
        percent: root.percent
        unit: root.unit * root.glyphScale
        ink: root.low ? root.levelColor : AppPalette.textSecond
        fillColor: root.levelColor
    }

    Shape {
        id: bolt
        anchors.verticalCenter: parent.verticalCenter
        visible: root.charging
        width: Math.round(7 * root.unit)
        height: Math.round(12 * root.unit)
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: -1
            fillColor: AppPalette.linkOkText
            startX: bolt.width * 0.62; startY: 0
            PathLine { x: 0; y: bolt.height * 0.58 }
            PathLine { x: bolt.width * 0.45; y: bolt.height * 0.58 }
            PathLine { x: bolt.width * 0.35; y: bolt.height }
            PathLine { x: bolt.width; y: bolt.height * 0.4 }
            PathLine { x: bolt.width * 0.55; y: bolt.height * 0.4 }
            PathLine { x: bolt.width * 0.7; y: 0 }
        }
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        visible: root.percent >= 0
        text: root.percent + " %"
        color: root.low ? root.levelColor : AppPalette.textStrong
        font.pixelSize: root.fontPixelSize
        font.bold: true
    }
}

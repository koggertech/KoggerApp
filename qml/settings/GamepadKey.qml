import QtQuick
import kqml_types 1.0

Rectangle {
    id: key

    property Item pad: null
    property int bit: 0
    property string label: ""
    property color tint: AppPalette.accent
    property bool tinted: false
    property int labelSize: tinted ? Math.round(Math.min(width, height) * 0.5) : 15
    readonly property bool down: pad ? pad.isDown(bit) : false
    readonly property bool available: pad ? pad.isAvailable(bit) : true
    readonly property bool symbol: label === "cross" || label === "circle" || label === "square" || label === "triangle"

    opacity: available ? 1.0 : 0.2
    radius: Math.min(width, height) / 2
    color: down ? tint : (tinted ? AppPalette.bgDeep : AppPalette.controlRaised)
    border.color: down ? Qt.lighter(tint, 1.3) : (tinted ? Qt.rgba(tint.r, tint.g, tint.b, 0.55) : AppPalette.borderHover)
    border.width: 2

    Text {
        anchors.centerIn: parent
        visible: !key.symbol
        text: key.label
        color: key.down ? "#FFFFFF" : (key.tinted ? key.tint : AppPalette.text)
        font.pixelSize: key.labelSize
        font.bold: true
    }

    GamepadSymbol {
        anchors.centerIn: parent
        width: Math.round(Math.min(key.width, key.height) * 0.44)
        visible: key.symbol
        label: key.label
        color: key.down ? "#FFFFFF" : key.tint
    }
}

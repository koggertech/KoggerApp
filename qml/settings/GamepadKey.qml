import QtQuick
import kqml_types 1.0

Rectangle {
    id: key

    property Item pad: null
    property int bit: 0
    property string label: ""
    property color tint: AppPalette.accent
    property bool tinted: false
    property int labelSize: 13
    readonly property bool down: pad ? pad.isDown(bit) : false
    readonly property bool available: pad ? pad.isAvailable(bit) : true
    readonly property string glyph: label === "cross" ? "✕"
                                  : label === "circle" ? "○"
                                  : label === "square" ? "□"
                                  : label === "triangle" ? "△"
                                  : ""

    opacity: available ? 1.0 : 0.2
    radius: Math.min(width, height) / 2
    color: down ? tint : (tinted ? Qt.rgba(tint.r, tint.g, tint.b, 0.28) : AppPalette.controlRaised)
    border.color: down ? Qt.lighter(tint, 1.3) : AppPalette.borderHover
    border.width: 2

    Text {
        anchors.centerIn: parent
        text: key.glyph.length ? key.glyph : key.label
        color: key.down ? "#FFFFFF" : (key.glyph.length ? key.tint : AppPalette.text)
        font.pixelSize: key.glyph.length ? Math.round(key.height * 0.6) : key.labelSize
        font.bold: true
    }
}

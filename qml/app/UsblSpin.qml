import QtQuick 2.15
import kqml_types 1.0

// KSpinBox wrapper that pushes edits out through `writeBack` and accepts external updates
// without echoing them back. Mirrors DeviceSettingsPage's inline DevSpin, which is not
// visible outside that file.
Item {
    id: root

    property int devValue: 0
    property int from: 0
    property int to: 100
    property int stepSize: 1
    property var writeBack: null
    property int fontPixelSize: Tokens.fontSm

    implicitWidth: Math.round(115 * AppPalette.scale)
    implicitHeight: Tokens.controlHMd

    property bool _in: false

    onDevValueChanged: {
        if (spin.value !== devValue) { _in = true; spin.value = devValue; _in = false }
    }
    Component.onCompleted: { _in = true; spin.value = devValue; _in = false }

    KSpinBox {
        id: spin
        anchors.fill: parent
        from: root.from; to: root.to; stepSize: root.stepSize
        fontPixelSize: root.fontPixelSize
        onValueModified: function (v) { if (!root._in && root.writeBack) root.writeBack(v) }
    }
}

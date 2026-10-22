import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    property string value: ""
    property string placeholder: ""
    property var validator: null
    property bool monospace: false
    property int fontPixelSize: Tokens.fontSm
    signal committed(string v)

    implicitWidth: Math.round(160 * AppPalette.scale)
    implicitHeight: Tokens.controlHMd
    radius: Tokens.radiusMd
    color: AppPalette.bg
    border.width: _in.activeFocus ? 1 : Tokens.cardBorderWidth
    border.color: _in.activeFocus ? AppPalette.accentBorder : AppPalette.border

    onValueChanged: if (!_in.activeFocus && _in.text !== value) _in.text = value
    Component.onCompleted: _in.text = root.value

    TextInput {
        id: _in
        anchors.fill: parent
        anchors.leftMargin: Tokens.spaceMd
        anchors.rightMargin: Tokens.spaceMd
        verticalAlignment: TextInput.AlignVCenter
        activeFocusOnTab: true
        clip: true
        selectByMouse: true
        color: AppPalette.text
        font.pixelSize: root.fontPixelSize
        font.family: root.monospace ? "monospace" : font.family
        inputMethodHints: Qt.ImhNoPredictiveText
        validator: root.validator
        onEditingFinished: root.committed(_in.text)
        Keys.onEscapePressed: { _in.text = root.value; _in.focus = false }
        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: _in.selectAll() }

        Text {
            visible: !_in.text.length && !_in.activeFocus
            anchors.verticalCenter: parent.verticalCenter
            text: root.placeholder
            color: AppPalette.textMuted
            font: _in.font
        }
    }
}

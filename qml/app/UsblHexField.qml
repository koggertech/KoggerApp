import QtQuick 2.15
import kqml_types 1.0

// Hex byte entry. There is no K* text field in the design system, so this reproduces the
// TextInput-in-a-Rectangle pattern used by the connection editor.
//
// `value` is applied imperatively rather than bound: a binding on TextInput.text is
// destroyed by the first keystroke, which would silently stop external updates from
// landing afterwards.
Rectangle {
    id: root

    property string value: ""
    property string placeholder: ""
    signal committed(string v)

    implicitWidth: Math.round(140 * AppPalette.scale)
    implicitHeight: Tokens.controlHMd
    radius: Tokens.radiusSm
    color: AppPalette.bg
    border.width: Tokens.cardBorderWidth
    border.color: _in.activeFocus ? AppPalette.accentBorder : AppPalette.border

    onValueChanged: if (!_in.activeFocus && _in.text !== value) _in.text = value
    Component.onCompleted: _in.text = root.value

    TextInput {
        id: _in
        anchors.fill: parent
        anchors.leftMargin: Tokens.spaceSm
        anchors.rightMargin: Tokens.spaceSm
        verticalAlignment: TextInput.AlignVCenter
        activeFocusOnTab: true
        clip: true
        color: AppPalette.text
        font.pixelSize: Tokens.fontSm
        font.letterSpacing: 0.5
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        validator: RegularExpressionValidator { regularExpression: /[0-9a-fA-F ]*/ }
        onEditingFinished: root.committed(_in.text)

        Text {
            visible: !_in.text.length && !_in.activeFocus
            anchors.verticalCenter: parent.verticalCenter
            text: root.placeholder
            color: AppPalette.textMuted
            font: _in.font
        }
    }
}

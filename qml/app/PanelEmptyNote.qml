import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: note

    property alias text: label.text
    property real fontPixelSize: Tokens.fontBase
    property real padding: Tokens.spaceLg
    property real fillAlpha: 1.0

    implicitHeight: Math.ceil(label.implicitHeight) + padding * 2
    radius: Tokens.radiusLg
    color: Qt.rgba(AppPalette.card.r, AppPalette.card.g, AppPalette.card.b, fillAlpha)
    border.width: Tokens.cardBorderWidth
    border.color: AppPalette.border

    Text {
        id: label
        x: note.padding
        width: Math.max(0, note.width - note.padding * 2)
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: AppPalette.textSecond
        font.pixelSize: note.fontPixelSize
    }
}

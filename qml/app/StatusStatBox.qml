import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: box

    property string blabel: ""
    property string bvalue: ""
    property bool keyBox: false
    property bool errorBox: false
    property bool alertBox: false

    height: Math.round(42 * AppPalette.scale)
    radius: Tokens.radiusMd
    implicitWidth: sbCol.implicitWidth + Tokens.spaceMd * 2
    width: implicitWidth
    color: AppPalette.card
    border.width: 1
    border.color: (box.errorBox || box.alertBox) ? AppPalette.linkDownBorder
                : box.keyBox ? Qt.rgba(AppPalette.linkOkBorder.r, AppPalette.linkOkBorder.g, AppPalette.linkOkBorder.b, 0.45)
                : AppPalette.border

    Column {
        id: sbCol
        anchors.centerIn: parent
        spacing: Math.round(1 * AppPalette.scale)
        Text {
            visible: box.blabel.length > 0
            text: box.blabel
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
        }
        Text {
            text: box.bvalue
            font.pixelSize: Tokens.fontSm
            font.bold: true
            color: (box.errorBox || box.alertBox) ? AppPalette.linkDownText
                 : box.keyBox ? AppPalette.linkOkText : AppPalette.text
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: Math.round(2 * AppPalette.scale)
        border.color: AppPalette.linkDownBorder
        visible: box.errorBox
        SequentialAnimation on opacity {
            running: box.errorBox
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 0.12; duration: 560; easing.type: Easing.InOutSine }
            NumberAnimation { from: 0.12; to: 1.0; duration: 560; easing.type: Easing.InOutSine }
        }
    }
}

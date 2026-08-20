import QtQuick 2.15
import QtQuick.Window 2.15
import kqml_types 1.0

Window {
    id: root

    property int maxInstances: 2

    width: Math.round(460 * AppPalette.scale)
    height: Math.round(210 * AppPalette.scale)
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    visible: true
    title: qsTr("KoggerApp, KOGGER")
    color: AppPalette.bg

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Tokens.spaceXl
        spacing: Tokens.spaceLg

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontLg
            font.bold: true
            text: qsTr("Application limit reached")
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            lineHeight: 1.2
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontBase
            text: qsTr("Only %1 KoggerApp applications can be open at once. Close one of them to open a new one.")
                  .arg(root.maxInstances)
        }

        KButton {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.round(120 * AppPalette.scale)
            height: Tokens.controlHMd
            text: qsTr("Close")
            focus: true
            onClicked: Qt.quit()
            Keys.onReturnPressed: Qt.quit()
            Keys.onEnterPressed: Qt.quit()
            Keys.onEscapePressed: Qt.quit()
        }
    }
}

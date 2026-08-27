import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg
    readonly property real groupWidth: Math.max(0, width)

    SettingsGroup {
        id: videoGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Video")
        description: qsTr("Network video stream.")
        stateStore: root.store
        stateKey: "app.video"
        collapsedByDefault: true
        contentSpacing: Tokens.spaceMd

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceSm

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: urlField.activeFocus ? 1 : Tokens.cardBorderWidth
                border.color: urlField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: urlField
                    activeFocusOnTab: true
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceSm
                    anchors.rightMargin: Tokens.spaceXs
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    clip: true
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                    text: root.store ? root.store.videoUrl : ""

                    onAccepted: if (root.store) root.store.openVideoUrl(text)

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onDoubleTapped: urlField.selectAll()
                    }

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        visible: urlField.text.length === 0 && !urlField.activeFocus
                        text: "rtsp://192.168.1.10/stream"
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontBase
                    }
                }
            }

            KButton {
                Layout.preferredHeight: Tokens.controlHMd
                text: qsTr("Open")
                enabled: urlField.text.length > 0
                onClicked: if (root.store) root.store.openVideoUrl(urlField.text)
            }

            KButton {
                Layout.preferredHeight: Tokens.controlHMd
                text: qsTr("Stop")
                enabled: root.store ? root.store.videoActiveUrl.length > 0 : false
                onClicked: if (root.store) root.store.stopVideo()
            }
        }

        KTabBar {
            id: fitTab
            width: parent.width
            trackColor: AppPalette.bgDeep
            fontPixelSize: Tokens.fontBase
            options: [{ label: qsTr("Fit"),     value: 0 },
                      { label: qsTr("Fill"),    value: 1 },
                      { label: qsTr("Stretch"), value: 2 }]
            currentValue: root.store ? root.store.videoFillMode : 0
            onValueSelected: function(v) {
                if (root.store)
                    root.store.videoFillMode = v
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: root.store ? (root.store.videoActiveUrl.length > 0 && !root.store.videoPaneExists) : false
            text: qsTr("The stream is running, but no pane is set to Video. Choose the Video type for a pane to see it.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }

        Text {
            width: parent.width
            visible: root.store ? root.store.videoSourceWidth > 0 : false
            text: qsTr("Stream: ") + (root.store ? root.store.videoSourceWidth + "×" + root.store.videoSourceHeight : "")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }

    }
}

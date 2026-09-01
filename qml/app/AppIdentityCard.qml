import QtQuick 2.15
import kqml_types 1.0

Column {
    id: card

    property var store: null

    spacing: Tokens.spaceXs

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        source: "qrc:/kogger_app_logo.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        width: Math.round(150 * AppPalette.scale)
        sourceSize.width: Math.round(360 * AppPalette.scale)
        opacity: 0.85
    }

    Text {
        id: versionLabel
        anchors.horizontalCenter: parent.horizontalCenter
        text: Qt.application.displayName
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm

        property int taps: 0

        Timer {
            id: versionTapReset
            interval: 2500
            onTriggered: versionLabel.taps = 0
        }

        Timer {
            id: devModeNoticeCooldown
            interval: 3000
        }

        MouseArea {
            anchors.fill: parent
            anchors.margins: -Tokens.spaceSm
            onClicked: {
                if (!card.store)
                    return

                if (card.store.developerMode) {
                    if (!devModeNoticeCooldown.running) {
                        notifications.info(qsTr("Developer mode is already enabled"))
                        devModeNoticeCooldown.restart()
                    }
                    return
                }

                versionLabel.taps++
                versionTapReset.restart()

                var left = card.store.developerUnlockTaps - versionLabel.taps
                if (left === 1) {
                    notifications.info(qsTr("Tap once more to unlock developer features"))
                } else if (left <= 0) {
                    versionTapReset.stop()
                    versionLabel.taps = 0
                    card.store.developerMode = true
                    notifications.info(qsTr("Developer mode enabled"))
                }
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: "KOGGER LLC"
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }
}

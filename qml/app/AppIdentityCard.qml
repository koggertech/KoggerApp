import QtQuick 2.15
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

Column {
    id: card

    property var store: null
    readonly property int logoHeight: Math.round(36 * AppPalette.scale)
    readonly property int logoSourceHeight: Math.round(86 * AppPalette.scale)

    spacing: Tokens.spaceXs

    Row {
        anchors.horizontalCenter: parent.horizontalCenter

        Image {
            source: "qrc:/kogger-app-mark.png"
            height: card.logoHeight
            width: implicitHeight > 0 ? Math.round(height * implicitWidth / implicitHeight) : 0
            sourceSize.height: card.logoSourceHeight
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Item {
            height: card.logoHeight
            width: wordmarkImage.width

            Image {
                id: wordmarkImage
                source: "qrc:/kogger-app-wordmark.png"
                height: parent.height
                width: implicitHeight > 0 ? Math.round(height * implicitWidth / implicitHeight) : 0
                sourceSize.height: card.logoSourceHeight
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: false
            }

            ColorOverlay {
                anchors.fill: wordmarkImage
                source: wordmarkImage
                color: AppPalette.text
                smooth: true
            }
        }
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
                        notifications.info(qsTr("Developer mode is already enabled"), "", "developer")
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
                    notifications.info(qsTr("Developer mode enabled"), "", "developer")
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

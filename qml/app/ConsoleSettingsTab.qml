import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    KIsland {
        KIslandRow {
            label: qsTr("Show console")
            interactive: true
            onClicked: visibleSwitch.click()

            KSwitch {
                id: visibleSwitch
                flat: true
                checked: theme ? theme.consoleVisible : false
                onToggled: {
                    if (theme)
                        theme.consoleVisible = checked
                    checked = Qt.binding(function() { return theme ? theme.consoleVisible : false })
                }
            }
        }

        KIslandRow {
            label: qsTr("Colour marking")
            toolTipText: qsTr("Highlight log syntax with the app theme's colours")
            interactive: true
            onClicked: colorizeSwitch.click()

            KSwitch {
                id: colorizeSwitch
                flat: true
                checked: page.store ? page.store.consoleColorize : true
                onToggled: {
                    if (page.store)
                        page.store.consoleColorize = checked
                    checked = Qt.binding(function() { return page.store ? page.store.consoleColorize : true })
                }
            }
        }
    }

    KIsland {
        KIslandRow {
            label: qsTr("Binary protocol")
            toolTipText: qsTr("Log KP1/KP2 frames of the device protocol")
            interactive: true
            onClicked: protoBinSwitch.click()

            KSwitch {
                id: protoBinSwitch
                flat: true
                checked: page.store ? page.store.consoleProtoBin : false
                onToggled: {
                    if (page.store)
                        page.store.consoleProtoBin = checked
                    checked = Qt.binding(function() { return page.store ? page.store.consoleProtoBin : false })
                }
            }
        }

        KIslandRow {
            label: qsTr("NMEA sentences")
            toolTipText: qsTr("Log NMEA sentences received from the device")
            interactive: true
            onClicked: nmeaSwitch.click()

            KSwitch {
                id: nmeaSwitch
                flat: true
                checked: page.store ? page.store.consoleNmea : true
                onToggled: {
                    if (page.store)
                        page.store.consoleNmea = checked
                    checked = Qt.binding(function() { return page.store ? page.store.consoleNmea : true })
                }
            }
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceSm

        Row {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Keep last lines:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: page.store ? Math.round(page.store.consoleMaxRows) : ""
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        KSlider {
            id: rowsSlider
            width: parent.width
            from: 50; to: 4000; stepSize: 50
            showValueTip: false
            value: page.store ? page.store.consoleMaxRows : 500
            onValueModified: function(v) { if (page.store) page.store.consoleMaxRows = v }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Ring buffer — the console keeps only the newest lines; older ones are dropped.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }
    }
}

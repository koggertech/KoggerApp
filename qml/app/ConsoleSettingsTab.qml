import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    component ResetButton: KButton {
        text: qsTr("Default")
        toolTipText: qsTr("Reset to default")
    }

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

        Item {
            width: parent.width
            height: fontReset.height

            Text {
                id: fontLabel
                anchors.left: parent.left
                anchors.right: fontValue.left
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Log text size:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                elide: Text.ElideRight
            }
            Text {
                id: fontValue
                anchors.right: fontReset.left
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                text: page.store ? qsTr("%1 px").arg(Math.round(page.store.consoleFontSize)) : ""
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase; font.bold: true
            }
            ResetButton {
                id: fontReset
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                enabled: !!page.store && page.store.consoleFontSize !== page.store.consoleFontSizeDefault
                onClicked: if (page.store) page.store.consoleFontSize = page.store.consoleFontSizeDefault
            }
        }

        KSlider {
            id: fontSlider
            width: parent.width
            from: page.store ? page.store.consoleFontSizeMin : 9
            to: page.store ? page.store.consoleFontSizeMax : 22
            stepSize: 1
            showValueTip: false
            value: page.store ? page.store.consoleFontSize : 13
            onValueModified: function(v) { if (page.store) page.store.consoleFontSize = v }
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceSm

        Item {
            width: parent.width
            height: rowsReset.height

            Text {
                id: rowsLabel
                anchors.left: parent.left
                anchors.right: rowsValue.left
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Keep last lines:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                elide: Text.ElideRight
            }
            Text {
                id: rowsValue
                anchors.right: rowsReset.left
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                text: page.store ? Math.round(page.store.consoleMaxRows) : ""
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase; font.bold: true
            }
            ResetButton {
                id: rowsReset
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                enabled: !!page.store && page.store.consoleMaxRows !== page.store.consoleMaxRowsDefault
                onClicked: if (page.store) page.store.consoleMaxRows = page.store.consoleMaxRowsDefault
            }
        }

        KSlider {
            id: rowsSlider
            width: parent.width
            from: page.store ? page.store.consoleMaxRowsMin : 50
            to: page.store ? page.store.consoleMaxRowsMax : 4000
            stepSize: 50
            showValueTip: false
            value: page.store ? page.store.consoleMaxRows : 1500
            onValueModified: function(v) { if (page.store) page.store.consoleMaxRows = v }
        }
    }
}

import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import Qt.labs.settings 1.1

Item {
    id: menu
    implicitWidth: menuLayout.width

    property var lastItem: menuSettings
    property bool isConsoleVisible: consoleEnable.checked

    function itemChangeActive(currentItem) {
        if(currentItem !== null) {
            currentItem.active = !(currentItem.active)
        }

        if(lastItem !== null && lastItem !== currentItem) {
            lastItem.active = false
        }

        lastItem = currentItem
    }

    RowLayout {
        id: menuLayout
        Layout.fillWidth: true

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: theme.controlHeight*2
            Layout.topMargin: 10

            ColumnLayout {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MenuButton {
                    id: menuSettings
                    icon.source: "./code-working.svg"
                    Layout.fillWidth: true

                    onPressed: {
                        itemChangeActive(menuSettings)
                    }
                }

                MenuButton {
                    id: menuDisplay
                    Layout.fillWidth: true
                    icon.source: "./settings-outline.svg"

                    onPressed: {
                        itemChangeActive(menuDisplay)
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter

                MenuBlock {
                }


                CText {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    visible: chartEnable.checked
                    horizontalAlignment: Text.AlignHCenter
                    text: chartLevel.stopValue + " dB"
                    small: true
                }

                ChartLevel {
                    Layout.fillWidth: true
                    id: chartLevel
                    visible: chartEnable.checked
                    Layout.alignment: Qt.AlignHCenter

                    onStartValueChanged: {
                        core.setPlotStartLevel(startValue);
                    }

                    onStopValueChanged: {
                        core.setPlotStopLevel(stopValue);
                    }
                }

                CText {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 10
                    visible: chartEnable.checked
                    horizontalAlignment: Text.AlignHCenter

                    text: chartLevel.startValue + " dB"
                    small: true
                }
            }
        }

        DeviceSettingsViewer {
            id: devSettings
            Layout.alignment: Qt.AlignTop
            visible: menuSettings.active
            Layout.maximumHeight: menu.height
            width: theme.controlHeight*20
            y:0
        }

        DiplaySettingsViewer {
            id: appSettings
            Layout.alignment: Qt.AlignTop
            visible: menuDisplay.active
            Layout.maximumHeight: menu.height
            width: theme.controlHeight*20
            y:0
        }
    }
}

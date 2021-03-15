import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import Qt.labs.settings 1.1

Item {
    id: menu

    property var lastItem: null
    property int heightMenuViewer: 400
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

    Rectangle {
        opacity: 0.8
        color: "#252320"
        radius: 0
        x: 0
        y: 0
        width: menuLayout.width
        height: menuLayout.height
        border.color: "#353530"
        border.width: 0
    }

    ColumnLayout {
        id: menuLayout
        Layout.fillHeight: true

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 0

            MenuBlock {
                opacity: 0.3
                border.width: 1
            }

            MenuButton {
                id: menuSettings
                Layout.margins: 4
                text: "Set"

                onPressed: {
                    itemChangeActive(menuSettings)
                }

                DeviceSettingsViewer {
                    height: heightMenuViewer
                    width: 580
                    x: menuSettings.width + 10
                    y: -menuSettings.y
                }
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 4

            MenuBlock {
                opacity: 0.3
                border.width: 1
            }

            CCheck {
                id: chartEnable
                checked: true
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true

                font.pixelSize: 14
                text: "Chr"

                Settings {
                    property alias chartEnable: chartEnable.checked
                }

                onCheckedChanged: {
                    core.setChartVis(checked);
                }
                opacity: 0.8
            }

            Text {
                visible: chartEnable.checked
                Layout.alignment: Qt.AlignHCenter
                text: chartLevel.stopValue + " dB"
                font.pixelSize: 14
                color: "#909090"
            }

            ChartLevel {
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

            Text {
                visible: chartEnable.checked
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 8

                text: chartLevel.startValue + " dB"
                font.pixelSize: 14
                color: "#909090"
            }

            CCheck {
                id: distEnable
                checked: true
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.pixelSize: 14
                text: "Dst"

                Settings {
                    property alias distEnable: distEnable.checked
                }

                onCheckedChanged: {
                    core.setDistVis(checked);
                }
                opacity: 0.8
            }

            CCheck {
                id: oscilloscopeEnable
                checked: false
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.pixelSize: 14
                text: "Osc"

                Settings {
                    property alias oscilloscopeEnable: oscilloscopeEnable.checked
                }

                onCheckedChanged: {
                    core.setOscVis(checked);
                }

                opacity: 0.8
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 4

            MenuBlock {
                opacity: 0.3
                border.width: 1
            }

            CCheck {
                id: consoleEnable
                checked: false
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.pixelSize: 14
                text: "Cns"
                opacity: 0.8

                Settings {
                    property alias consoleEnable: consoleEnable.checked
                }
            }
        }
    }
}

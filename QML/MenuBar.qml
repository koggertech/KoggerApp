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

            ColumnLayout {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 10
                spacing: 0

                MenuButton {
                    id: menuSettings
                    Layout.margins: 4
                    text: "Set"

                    onPressed: {
                        itemChangeActive(menuSettings)
                    }
                }

                MenuButton {
                    id: menuDisplay
                    Layout.margins: 4
                    text: "Dsp"

                    onPressed: {
                        itemChangeActive(menuDisplay)
                    }
                }

                MenuButton {
                    id: menuExport
                    Layout.margins: 4
                    text: "Exp"

                    onPressed: {
                        itemChangeActive(menuExport)
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.margins: 4

                MenuBlock {
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
                        plot.setDistVis(checked);
                    }
                    opacity: 0.8
                }

                CCheck {
                    id: distProcEnable
                    checked: true
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    font.pixelSize: 14
                    text: "Prcs"

                    Settings {
                        property alias distProcEnable: distProcEnable.checked
                    }

                    onCheckedChanged: {
                        plot.setDistProcVis(checked);
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

        DeviceSettingsViewer {
            id: devSettings
            Layout.alignment: Qt.AlignTop
            visible: menuSettings.active
            height: menu.height
            width: 580
            y:0
        }

        DiplaySettingsViewer {
            id: menuDispVeawer
            Layout.alignment: Qt.AlignTop
            visible: menuDisplay.active
            height: menu.height
            width: 580
            y:0
        }

        ExportSettings{
            id: menuExportVeawer
            Layout.alignment: Qt.AlignTop
            visible: menuExport.active
            height: menu.height
            width: 580
            y:0
        }
    }
}

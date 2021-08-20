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
            Layout.preferredWidth: 59
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

//                MenuButton {
//                    id: menuDisplay
//                    Layout.margins: 4
//                    text: "Dsp"

//                    onPressed: {
//                        itemChangeActive(menuDisplay)
//                    }
//                }

//                MenuButton {
//                    id: menuExport
//                    Layout.margins: 4
//                    text: "Exp"

//                    onPressed: {
//                        itemChangeActive(menuExport)
//                    }
//                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter

                MenuBlock {
                }

//                CCheck {
//                    id: chartEnable
//                    checked: true
//                    Layout.alignment: Qt.AlignHCenter
//                    Layout.fillWidth: true

//                    font.pixelSize: 14
//                    text: "Chr"

//                    Settings {
//                        property alias chartEnable: chartEnable.checked
//                    }

//                    onCheckedChanged: {
//                        core.setChartVis(checked);
//                    }
//                    opacity: 0.8
//                }

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

//                CCheck {
//                    id: distEnable
//                    checked: true
//                    Layout.alignment: Qt.AlignHCenter
//                    Layout.fillWidth: true
//                    text: "Dst"

//                    Settings {
//                        property alias distEnable: distEnable.checked
//                    }

//                    onCheckedChanged: {
//                        plot.setDistVis(checked);
//                    }
//                }

//                CCheck {
//                    id: distProcEnable
//                    checked: true
//                    Layout.alignment: Qt.AlignHCenter
//                    Layout.fillWidth: true
//                    text: "Prcs"

//                    Settings {
//                        property alias distProcEnable: distProcEnable.checked
//                    }

//                    onCheckedChanged: {
//                        plot.setDistProcVis(checked);
//                    }
//                }

//                CCheck {
//                    id: oscilloscopeEnable
//                    checked: false
//                    Layout.alignment: Qt.AlignHCenter
//                    Layout.fillWidth: true
//                    text: "Osc"

//                    Settings {
//                        property alias oscilloscopeEnable: oscilloscopeEnable.checked
//                    }

//                    onCheckedChanged: {
//                        core.setOscVis(checked);
//                    }
//                }
//            }

//            ColumnLayout {
//                Layout.alignment: Qt.AlignHCenter
//                Layout.margins: 4

//                MenuBlock {
//                }

//                CCheck {
//                    id: consoleEnable
//                    checked: false
//                    Layout.alignment: Qt.AlignHCenter
//                    Layout.fillWidth: true
//                    text: "Cns"

//                    Settings {
//                        property alias consoleEnable: consoleEnable.checked
//                    }
//                }
            }
        }

        DeviceSettingsViewer {
            id: devSettings
            Layout.alignment: Qt.AlignTop
            visible: menuSettings.active
            Layout.maximumHeight: menu.height
            width: 500
            y:0
        }

//        DiplaySettingsViewer {
//            id: menuDispVeawer
//            Layout.alignment: Qt.AlignTop
//            visible: menuDisplay.active
//            Layout.maximumHeight: menu.height
//            width: 580
//            y:0
//        }

//        ExportSettings{
//            id: menuExportVeawer
//            Layout.alignment: Qt.AlignTop
//            visible: menuExport.active
//            Layout.maximumHeight: menu.height
//            width: 500
//            y:0
//        }
    }
}

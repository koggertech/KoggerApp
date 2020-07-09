import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4

//import SerialList 1.0

Item {
    id: menu

    property var lastItem: null
    property real heightMenuViewer: 500

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
//        spacing: 10

        ColumnLayout {

            MenuButton {
                Layout.topMargin: 10
                id: menuConnection
                text: "C"

                onPressed: {
                    itemChangeActive(menuConnection)
                }

                ConnectionViewer {
                    height: heightMenuViewer
                    width: 580
                    x: menuConnection.width + 10
                    y: -menuConnection.y
                }
            }

            MenuButton {
                id: menuSettings
                Layout.bottomMargin: 10
                text: "S"

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

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 40
            height: 2
            color: "#404040"
        }

        ColumnLayout {

            Text {
                Layout.topMargin: 10
                Layout.alignment: Qt.AlignHCenter
                text: chartLevel.stopValue + " dB"
                font.pixelSize: 14
                color: "#909090"
            }

            ChartLevel {
                id: chartLevel

                onStartValueChanged: {
                    core.setPlotStartLevel(startValue);
                }

                onStopValueChanged: {
                    core.setPlotStopLevel(stopValue);
                }

            }

            Text {
                Layout.bottomMargin: 10
                Layout.alignment: Qt.AlignHCenter
                text: chartLevel.startValue + " dB"
                font.pixelSize: 14
                color: "#909090"
            }
        }
    }
}

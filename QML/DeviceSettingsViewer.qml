import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4

MenuViewer {
    id: viewerDeviceSettings
    width: grid.width
    height: grid.height

    GridLayout {
        id: grid
        columns: 5
        rowSpacing: 5

        RowLayout {
            Layout.margins: 10
            Layout.fillWidth: true
            Layout.columnSpan:  5
//            ComboBox  {
//                id: connectionTypeCombo
//                Layout.columnSpan:  5
//                Layout.fillWidth: true
//                model: ["Serial", "Log File", "Network"]
//            }

            Slider {
                from: 0
                to: 100
                value: 10
                stepSize: 10
            }
        }

//        RowLayout {
//            Layout.margins: 10
//            Layout.columnSpan:  5
//            ComboBox  {
//                id: portCombo
//                visible: connectionTypeCombo.currentText === "Serial"

//                onPressedChanged: {
//                    if(pressed) {
//                        model = core.availableSerialName()
//                    }
//                }

//                Component.onCompleted: {
//                    model = core.availableSerialName()
//                }
//            }

//            ComboBox  {
//                id: baudrateCombo
//                visible: connectionTypeCombo.currentText === "Serial"
//                model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]
//            }
//        }

//        RowLayout {
//            Layout.margins: 10
//            Layout.columnSpan:  5

//            Button {
//                property bool isConnection: false

//                id: connectionButton
//                Layout.fillWidth: true
//                text: "Connection"

//                onClicked: {
//                    if(isConnection) {
//                        core.closeConnection()
//                    } else {
//                        if(connectionTypeCombo.currentText === "Serial") {
//                            core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText))
//                        }
//                    }

//                    isConnection = core.isOpenConnection()
//                }

//                onIsConnectionChanged: {
//                    if(isConnection) {
//                        text = "Disconnection"
//                    } else {
//                        text = "Connection"
//                    }
//                }
//            }

//        }
    }
}

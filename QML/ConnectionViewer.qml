import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuViewer {
    id: viewerConnection
    height: 400

    Connections {
        target: core

        onConnectionChanged: {
            connectionButton.connection = core.isOpenConnection()
        }
    }

    MenuScroll {
        id: scrollBar

        ColumnLayout {
            width: parent.width
            spacing: 20

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: columnConnectionItem.height

                MenuBlock {
                }

                ColumnLayout {
                    id: columnConnectionItem
                    width: parent.width

                    TitleMenuBox {
                        titleText: "Connection #1"

//                        CCheck {
//                            id:checkLogging
//                            text: "Logging"
////                            checked: sonarDriver.datasetChart > 0
////                            onCheckedChanged: {
////                                if(checked == true && sonarDriver.datasetChart == 0) {
////                                    sonarDriver.datasetChart = switchDatasetChart.lastChannel
////                                } else if(checked == false && sonarDriver.datasetChart > 0) {
////                                    switchDatasetChart.lastChannel = sonarDriver.datasetChart
////                                    sonarDriver.datasetChart = 0
////                                }
////                            }
//                        }

//                        Text {
////                            text: "TX: 0 byte, 0 packet, 0 error;" + "     " + "RX: 0 byte, 0 packet, 0 error;"
//                            Layout.fillWidth: true
//                            horizontalAlignment: Text.AlignRight
//                            color: "#808080"
//                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 15

                        spacing: 10

                        CCombo  {
                            id: deviceTypeCombo
                            Layout.fillWidth: true

                            model: ["KOGGER"]


                            Settings {
                                property alias connectionType: deviceTypeCombo.currentIndex
                            }
                        }

                        CCombo  {
                            id: connectionTypeCombo
                            Layout.fillWidth: true
                            model: ["SERIAL"]

                            Settings {
                                property alias connectionType: connectionTypeCombo.currentIndex
                            }
                        }

                        CCombo  {
                            id: portCombo
                            Layout.fillWidth: true
                            visible: connectionTypeCombo.currentText === "SERIAL"
                            onPressedChanged: {
                                if(pressed) {
                                    model = core.availableSerialName()
                                }
                            }

                            Component.onCompleted: {
                                model = core.availableSerialName()

                            }

                            Settings {
                                property alias connectionType: portCombo.currentIndex
                            }
                        }

                        CCombo  {
                            id: baudrateCombo
                            Layout.fillWidth: true
                            visible: connectionTypeCombo.currentText === "SERIAL"
                            model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]
                            currentIndex: 4

                            Settings {
                                property alias serialBaudrate: baudrateCombo.currentIndex
                            }
                        }

                        CButton {
                            id: connectionButton
                            property bool connection: false
                            width: 30
                            implicitHeight: 30

                            text: ""

                            onClicked: {
                                if(connection) {
                                    core.closeConnection()
                                } else {
                                    if(connectionTypeCombo.currentText === "SERIAL") {
                                        core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText))
                                    } else if(connectionTypeCombo.currentText === "FILE") {
                                        fileDialog.open()
                                    }
                                }
                            }

                            onConnectionChanged: {
                                if(connection) {
                                    if(connectionTypeCombo.currentText === "SERIAL") {
                                    } else {
                                    }
                                } else {
                                }
                            }

                            indicator: Canvas {
                                id: canvas
                                x: connectionButton.width - width - connectionButton.rightPadding
                                y: connectionButton.topPadding + (connectionButton.availableHeight - height) / 2
                                width: connectionButton.availableWidth
                                height: connectionButton.availableHeight
                                contextType: "2d"

                                Connections {
                                    target: connectionButton
                                    onPressedChanged: canvas.requestPaint()
                                }

                                onPaint: {
                                    context.reset();

                                    if(connectionButton.connection) {
                                        context.moveTo(0, 0);
                                        context.lineTo(width, 0);
                                        context.lineTo(width, height);
                                        context.lineTo(0, height);
                                        context.closePath();
                                    } else {
                                        context.moveTo(0, 0);
                                        context.lineTo(width, height/2);
                                        context.lineTo(0, height);
                                        context.closePath();
                                    }

                                    context.fillStyle = connectionButton.connection ? "#E05040" : "#40E050"
                                    context.fill();
                                }
                            }
                        }
                    }

//                    RowLayout {
//                        Layout.fillWidth: true
//                        Layout.margins: 15
//                        visible: checkLogging.checked
//                        spacing: 10

//                        FileDialog {
//                            id: fileDialog
//                            title: "Please choose a folder"
//                            folder: shortcuts.home
//                            selectFolder: true
//                            onAccepted: {
//                                pathText.text = fileDialog.fileUrl.toString()
//                            }
//                            onRejected: {
//                            }
//                        }

//                        Settings {
//                            property alias loggingFolder: fileDialog.folder
//                        }

//                        TextField {
//                            id: pathText
//                            hoverEnabled: true
//                            Layout.fillWidth: true
//                            height: control.height
//                            padding: 4
//                            rightPadding: 40
//                            font.family: "Bahnschrift"; font.pointSize: 14;
//                            color: "#E07000"

//                            text: ""
//                            placeholderText: qsTr("Enter path")

//                            background: Rectangle {
//                                color: "#505050"
//                            }
//                        }

//                        CButton {
//                            text: "..."
//                            Layout.fillWidth: false
//                            implicitHeight: 30
//                            onClicked: {
//                                fileDialog.open()
//                            }
//                        }
//                    }
                }
            }
        }
    }
}

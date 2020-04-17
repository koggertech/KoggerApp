import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuViewer {
    id: viewerConnection
    width: grid.width
    height: grid.height

    Connections {
        target: core

        onConnectionChanged: {
            connectionButton.connection = core.isOpenConnection()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + fileDialog.fileUrls)
            core.openConnectionAsFile(fileDialog.fileUrl)
        }
        onRejected: {
            console.log("Canceled")
        }
    }

    ColumnLayout {
        id: grid

        RowLayout {
            Layout.margins: 20
            Layout.bottomMargin: 20
            Layout.fillWidth: true

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
                visible: connectionTypeCombo.currentText === "SERIAL"
                model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]

                Settings {
                    property alias serialBaudrate: baudrateCombo.currentIndex
                }
            }

            CButton {
                id: connectionButton
                property bool connection: false
                width: 30


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

        UpgradeBox {
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 5
            Layout.bottomMargin: 20
            Layout.fillWidth: true

            visible: connectionButton.connection
        }

//        AdjBox {
//            Layout.leftMargin: 20
//            Layout.rightMargin: 20
//            Layout.topMargin: 5
//            Layout.bottomMargin: 20
//            Layout.fillWidth: true

//            visible: connectionButton.connection

//            textValue: sonarDriver.chartSamples
//            textTitle: "SAMPLES"
//            sliderStepSize: sonarDriver.chartSamplSliderStep
//            onSliderValueChanged: {
//                sonarDriver.chartSamplSlider = sliderValue
//            }
//        }

    }
}

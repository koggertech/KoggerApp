import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    Layout.fillWidth: true
    Layout.preferredHeight: columnConnectionItem.height

    Connections {
        target: core

        onConnectionChanged: {
            connectionButton.connection = core.isOpenConnection()
        }
    }


    MenuBlock {
    }

    ColumnLayout {
        id: columnConnectionItem
        width: parent.width

        TitleMenuBox {
            id: titleConnection
            titleText: "Choose a connection..."
            visible: titleText != ""

            Connections {
                target: core
                onConnectionChanged: {
                    if(core.isOpenConnection()) {

                    } else {
                        titleConnection.titleText = "Choose a connection..."
                    }
                }
            }

            Connections {
                target: sonarDriver
                onDeviceVersionChanged: {
                    if(core.isOpenConnection()) {
                        titleConnection.titleText = core.deviceName() + ", SN: " + core.deviceSerialNumber()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            spacing: 10

            CCombo  {
                id: connectionTypeCombo
                model: ["SERIAL", "FILE"]

                Settings {
                    property alias connectionType: connectionTypeCombo.currentIndex
                }
            }

            CCombo  {
                id: deviceTypeCombo
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "SERIAL"
                model: ["KOGGER"]


                Settings {
                    property alias connectionTarget: deviceTypeCombo.currentIndex
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
                    property alias connectionPort: portCombo.currentIndex
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

            TextField {
                id: pathText
                hoverEnabled: true
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "FILE"
                padding: 4
                rightPadding: 40
                font.family: "Bahnschrift"; font.pointSize: 14;
                color: "#E07000"

                text: ""
                placeholderText: qsTr("Enter path")

                background: Rectangle {
                    color: "#505050"
                }
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                visible: connectionTypeCombo.currentText === "FILE"
                implicitHeight: 30
                onClicked: {
                    logFileDialog.open()
                }

                FileDialog {
                    id: logFileDialog
                    title: "Please choose a file"
                    folder: shortcuts.home
                    nameFilters: ["Kogger log files (*.klf)"]

                    onAccepted: {
                        pathText.text = logFileDialog.fileUrl.toString()
                    }
                    onRejected: {
                    }
                }

                Settings {
                    property alias logFolder: logFileDialog.folder
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
                            core.openConnectionAsFile(pathText.text);
                        }
                    }
                }

                onConnectionChanged: {
                    canvas.requestPaint()
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
    }
}

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    Layout.fillWidth: true
    Layout.preferredHeight: columnConnectionItem.height

    property var dev: null
    property var devList: devs.devs

    Connections {
        target: core

        onConnectionChanged: {
            connectionButton.connection = core.isOpenConnection()
            dev = null
        }
    }

    onDevListChanged: {
        devTab0.visible = devs.isCreatedId(0)
        if(devs.isCreatedId(0)) { dev = devList[0] }

        devTab1.visible = devs.isCreatedId(1)

        devTab2.visible = devs.isCreatedId(2)
    }

    MenuBlock {
    }

    ColumnLayout {
        id: columnConnectionItem
        width: parent.width
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10

            CCombo  {
                id: connectionTypeCombo
                model: ["Serial", "File"]

                Settings {
                    property alias connectionType: connectionTypeCombo.currentIndex
                }
            }

            CCombo  {
                id: portCombo
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "Serial"
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
                visible: connectionTypeCombo.currentText === "Serial"
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
                visible: connectionTypeCombo.currentText === "File"
                padding: 4
                rightPadding: 40
                font.family: "Bahnschrift"; font.pointSize: 14;
                color: "#E07000"

                text: ""
                placeholderText: qsTr("Enter path")

                Keys.onPressed: {
                    if (event.key === 16777220) {
                        core.openConnectionAsFile(pathText.text);
                    }
                }

                background: Rectangle {
                    color: "#505050"
                }
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                visible: connectionTypeCombo.currentText === "File"
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
                        core.openConnectionAsFile(pathText.text);
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
                visible: connectionTypeCombo.currentText !== "File"

                text: ""

                onClicked: {
                    if(connection) {
                        core.closeConnection()
                    } else {
                        if(connectionTypeCombo.currentText === "Serial") {
                            core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText), false)
                        } else if(connectionTypeCombo.currentText === "File") {
//                            core.openConnectionAsFile(pathText.text);
                        }
                    }
                }

                onConnectionChanged: {
                    canvas.requestPaint()
                    if(connection) {
                        if(connectionTypeCombo.currentText === "Serial") {
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

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            Layout.topMargin: -5
            spacing: 10
            visible: connectionTypeCombo.currentText === "Serial"

            CCheck {
                id: loggingCheck
                text: "logging"
                checked: false

                onCheckedChanged: core.logging = loggingCheck.checked
                Component.onCompleted: core.logging = loggingCheck.checked

                Settings {
                    property alias loggingCheck: loggingCheck.checked
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10
            visible: connectionButton.connection

            CButton {
                id: devTab0
                text: devList[0].devName + "\nSN:" + devList[0].devSN
                Layout.fillWidth: false
                implicitHeight: 50

                opacity: dev === devList[0] ? 1 : 0.5
                onClicked: {
                    dev = devList[0]
                }
            }

            CButton {
                id: devTab1
                text: devList[1].devName + "\nSN:" + devList[1].devSN
                Layout.fillWidth: false
                implicitHeight: 30

                opacity: dev === devList[1] ? 1 : 0.5

                onClicked: {
                    dev = devList[1]
                }
            }

            CButton {
                id: devTab2
                text: devList[2].devName + "\nSN:" + devList[2].devSN
                Layout.fillWidth: false
                implicitHeight: 30

                opacity: dev === devList[2] ? 1 : 0.5

                onClicked: {
                    dev = devList[2]
                }
            }
        }
    }
}

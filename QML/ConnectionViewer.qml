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
                Layout.fillWidth: true
                model: ["Serial", "File", "IP"]

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

//                onCurrentTextChanged: {
//                    if(connectionButton.connection) {
//                        dev.baudrate = Number(baudrateCombo.currentText)

//                        core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText), false)
//                    }
//                }

                Settings {
                    property alias serialBaudrate: baudrateCombo.currentIndex
                }
            }

            CTextField {
                id: pathText
                hoverEnabled: true
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "File"

                text: ""
                placeholderText: qsTr("Enter path")

                Keys.onPressed: {
                    if (event.key === 16777220) {
                        core.openConnectionAsFile(pathText.text);
                    }
                }

                Settings {
                    property alias pathText: pathText.text
                }
            }

            CTextField {
                id: ipAddressText
                hoverEnabled: true
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "IP"

                text: "192.168.4.1"
                placeholderText: ""

                Keys.onPressed: {
                    if (event.key === 16777220) {
                        console.info(ipAddressText.text)
                    }
                }

                Settings {
                    property alias ipAddressText: ipAddressText.text
                }
            }



            CTextField {
                id: ipPortText
                hoverEnabled: true
                Layout.fillWidth: false
                implicitWidth: 80
                visible: connectionTypeCombo.currentText === "IP"

                text: "14444"
                placeholderText: qsTr("Port")

                Settings {
                    property alias ipPortText: ipPortText.text
                }
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                visible: connectionTypeCombo.currentText === "File"
                implicitHeight: theme.controlHeight
                implicitWidth: implicitHeight*1.1
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
                implicitWidth: implicitHeight + 3
                visible: connectionTypeCombo.currentText !== "File"

                text: ""

//                ToolTip {
//                    id: control
//                    text: qsTr("A descriptive tool tip of what the button does")
//                    visible: connectionButton.hovered
//                    implicitWidth: 100
//                    implicitHeight: 100

//                    contentItem: Text {
//                        text: control.text
//                        font: control.font
//                        color: "#21be2b"
//                    }

//                    background: Rectangle {
//                        border.color: "#21be2b"
//                    }
//                }

                onClicked: {
                    if(connection) {
                        core.closeConnection()
                    } else {
                        if(connectionTypeCombo.currentText === "Serial") {
                            core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText), false)
                        } else if(connectionTypeCombo.currentText === "IP") {
                            core.openConnectionAsIP(ipAddressText.text, Number(ipPortText.text), true);
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
            visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"

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


//        RowLayout {
//            Layout.fillWidth: true
//            ListView {
//                height: 320
//                width: 100
//                Layout.fillWidth: true
//                model: Qt.fontFamilies()

//                delegate: Item {
//                    height: 40;
//                    width: ListView.view.width
//                    Text {
//                        anchors.centerIn: parent
//                        text: modelData;
//                        color: "#FFFFFF"
//                    }
//                }
//            }
//        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10
            visible: connectionButton.connection

            CButton {
                id: devTab0
                text: devList[0].devName + " [" + devList[0].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[0] ? 1 : 0.5

                onClicked: {
                    dev = devList[0]
                }
            }

            CButton {
                id: devTab1
                text: devList[1].devName + " [" + devList[1].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[1] ? 1 : 0.5

                onClicked: {
                    dev = devList[1]
                }
            }

            CButton {
                id: devTab2
                text: devList[2].devName + " [" + devList[2].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[2] ? 1 : 0.5

                onClicked: {
                    dev = devList[2]
                }
            }
        }
    }
}

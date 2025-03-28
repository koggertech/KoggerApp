import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

ColumnLayout {
    id: columnConnectionItem
    width: parent.width

    CTextField {
        id: portNames
        Layout.fillWidth: true
        text: "Motor Control"
        readOnly: true
        background: Rectangle {
            color: "transparent"
            border.width: 0
            border.color: theme.controlBorderColor
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            text: "connect device:"
        }

        ComboBox {
            id: motorsComboBox
            model: linkManagerWrapper.linkListModel
            visible: count > 0
            width: parent.width
            height: theme.controlHeight
            Layout.margins: 0
            Layout.topMargin: 0
            Layout.bottomMargin: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            delegate: motorItem

            background: Rectangle {
                color: "transparent"
                border.width: 0
                border.color: theme.controlBorderColor
            }

            Component.onCompleted: {
                currentIndex = 0
            }

            Component {
                id: motorItem
                Item {
                    id: itemWrapper
                    width: motorsComboBox.width
                    height: theme.controlHeight + 4

                    Rectangle {
                        id: backgroundRect
                        anchors.fill: parent
                        color: ConnectionStatus ? "#005000" : theme.controlSolidBackColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 2


                            CTextField {
                                id: portName
                                text: LinkType === 1 ? PortName : SourcePort
                                Layout.fillWidth: true
                                readOnly: true
                                background: Rectangle {
                                    color: "transparent"
                                    border.width: 0
                                    border.color: theme.controlBorderColor
                                }
                            }

                            CCombo {
                                id: baudrateCombo
                                implicitWidth: 150
                                model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000]
                                currentIndex: 7
                                displayText: Baudrate
                                visible: LinkType === 1

                                onCurrentTextChanged: {
                                    linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
                                }

                                background: Rectangle {
                                    color: "transparent"
                                    border.width: 0
                                    border.color: theme.controlBorderColor
                                }
                            }

                            CButton {
                                text: ConnectionStatus ? "Close" : "Open"
                                backColor: ConnectionStatus ? "#005000" : theme.controlSolidBackColor
                                // visible: LinkType === 1

                                onClicked: {
                                    motorsComboBox.currentIndex = 0

                                    if (ConnectionStatus) {
                                        linkManagerWrapper.closeLink(Uuid)
                                    }
                                    else {
                                        if(LinkType === 1) {
                                            linkManagerWrapper.openAsSerial(Uuid, 2)
                                        } else if(LinkType === 2) {
                                            linkManagerWrapper.openAsUdp(Uuid, Address, SourcePort, DestinationPort, 2)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "current angles: "
        }

        CText {
            Layout.fillWidth: true
            text: "f: " + deviceManagerWrapper.fAngle.toFixed(3) + "°"
        }

        CText {
            Layout.fillWidth: true
            text: "s: " + deviceManagerWrapper.sAngle.toFixed(3) + "°"
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "return to zero:"
        }

        ColumnLayout {
            Layout.fillWidth: true

            CButton {
                text: "first"
                implicitWidth: 80
                onClicked: {
                    deviceManagerWrapper.sendReturnToZero(0)
                }
            }

            CButton {
                text: "second"
                implicitWidth: 80
                onClicked: {
                    deviceManagerWrapper.sendReturnToZero(1)
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "set angles (°):"
        }

        ColumnLayout {
            RowLayout {
                spacing: 10

                CText {
                    text: "f:"
                }

                SpinBoxCustom { // small
                    id: spinBox1
                    from: -180
                    to: 180
                    stepSize: 1
                    value: 15
                }
                CButton {
                    text: "set"
                    implicitWidth: 80
                    onClicked: {
                        deviceManagerWrapper.sendRunSteps(0, 1, spinBox1.value)
                    }
                }
            }

            RowLayout {
                spacing: 10

                CText {
                    text: "s:"
                }

                SpinBoxCustom { // big
                    id: spinBox2
                    from: -180
                    to: 180
                    stepSize: 1
                    value: 15
                }
                CButton {
                    text: "set"
                    implicitWidth: 80
                    onClicked: {
                        deviceManagerWrapper.sendRunSteps(1, 1, spinBox2.value)
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "task file:"
        }

        CTextField {
            id: pathTextMotor
            hoverEnabled: true
            Layout.fillWidth: true

            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    deviceManagerWrapper.sendOpenCsvFile(pathTextMotor.text);
                }
            }

            Settings {
                property alias pathTextMotor: pathTextMotor.text
            }
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/file.svg"
            checkable: false
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight

            onClicked: {
                newFileDialog.open()
            }

            FileDialog {
                id: newFileDialog
                title: "Please choose a CSV file"
                folder: shortcuts.home

                nameFilters: ["CSV (*.csv)"]

                onAccepted: {
                    pathTextMotor.text = newFileDialog.fileUrl.toString().replace("file:///", "")
                    var name_parts = newFileDialog.fileUrl.toString().split('.')
                    deviceManagerWrapper.sendOpenCsvFile(pathTextMotor.text);
                }

                onRejected: {
                }
            }

            Settings {
                property alias logFolder: newFileDialog.folder
            }
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/file_off.svg"
            checkable: false
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight

            onClicked: {
                deviceManagerWrapper.sendClearTasks();
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CText {
                Layout.fillWidth: true
                text: "f:"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                CText {
                    Layout.fillWidth: true
                    text: "task: " + deviceManagerWrapper.taskFAngle.toFixed(3) + "°"
                }

                CText {
                    Layout.fillWidth: true
                    text: "fact: " + deviceManagerWrapper.currFAngle.toFixed(3) + "°"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CText {
                Layout.fillWidth: true
                text: "s:"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                CText {
                    Layout.fillWidth: true
                    text: "task: " + deviceManagerWrapper.taskSAngle.toFixed(3) + "°"
                }

                CText {
                    Layout.fillWidth: true
                    text: "fact: " + deviceManagerWrapper.currSAngle.toFixed(3) + "°"
                }
            }
        }
    }
}

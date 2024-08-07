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
        text: "available serial ports:"
        readOnly: true
        background: Rectangle {
            color: "transparent"
            border.width: 0
            border.color: theme.controlBorderColor
        }
    }

    ListView {
        id: motorsList
        model: linkManagerWrapper.linkListModel
        visible: count > 0
        height: count * theme.controlHeight
        delegate: motorItem
        focus: true

        Layout.margins: 0
        Layout.topMargin: 0
        Layout.bottomMargin: 0
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: count * (theme.controlHeight + 4)
        Layout.maximumHeight: 10 * (theme.controlHeight + 4)

        onCountChanged: {
            Qt.callLater(positionViewAtEnd)
        }

        Component {
            id: motorItem
            Item {
                id: itemWrapper
                width: motorsList.width
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
                            text: LinkType === 1 ? PortName : "some another port"
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
                            visible: LinkType === 1

                            onClicked: {
                                if (ConnectionStatus) {
                                    linkManagerWrapper.closeLink(Uuid)
                                } else {
                                    linkManagerWrapper.openAsSerial(Uuid, true)
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
            text: "return to zero:"
        }

        CButton {
            text: "first"
            onClicked: {
                deviceManagerWrapper.sendReturnToZero(0)
            }
        }

        CButton {
            text: "second"
            onClicked: {
                deviceManagerWrapper.sendReturnToZero(1)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "angle:"
        }

        RowLayout {
            CText {
                Layout.fillWidth: true

                text: "first:"
            }

            SpinBoxCustom { // small
                id: spinBox1
                from: -179
                to: 179
                stepSize: 1
                value: 25
                Layout.fillWidth: true
            }
            CButton {
                text: "set"
                onClicked: {
                    deviceManagerWrapper.sendRunSteps(0, 1, spinBox1.value)
                }
            }
        }
        RowLayout {
            CText {
                Layout.fillWidth: true
                text: "second:"
            }
            SpinBoxCustom { // big
                id: spinBox2
                from: -49
                to: 49
                stepSize: 1
                value: 25
                Layout.fillWidth: true
            }
            CButton {
                text: "set"
                onClicked: {
                    deviceManagerWrapper.sendRunSteps(1, 1, spinBox2.value)
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CText {
            Layout.fillWidth: true
            text: "current f angle: " + deviceManagerWrapper.fAngle.toFixed(3) + "°"
        }

        CText {
            Layout.fillWidth: true
            text: "current s angle: " + deviceManagerWrapper.sAngle.toFixed(3) + "°"
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        CTextField {
            id: pathText
            hoverEnabled: true
            Layout.fillWidth: true

            text: core.filePath
            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    deviceManagerWrapper.sendOpenCsvFile(pathText.text);
                }
            }

            Settings {
                property alias pathText: pathText.text
            }
        }

        CheckButton {
            icon.source: "./icons/file.svg"
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
                    pathText.text = newFileDialog.fileUrl.toString().replace("file:///", "")

                    var name_parts = newFileDialog.fileUrl.toString().split('.')

                    deviceManagerWrapper.sendOpenCsvFile(pathText.text);
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: newFileDialog.folder
            }
        }

        CheckButton {
            icon.source: "./icons/file-off.svg"
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

        CText {
            Layout.fillWidth: true
            text: "cfa: " + deviceManagerWrapper.currFAngle.toFixed(3) + "°"
        }

        CText {
            Layout.fillWidth: true
            text: "csa: " + deviceManagerWrapper.currSAngle.toFixed(3) + "°"
        }

        CText {
            Layout.fillWidth: true
            text: "tfa: " + deviceManagerWrapper.taskFAngle.toFixed(3) + "°"
        }

        CText {
            Layout.fillWidth: true
            text: "tsa: " + deviceManagerWrapper.taskSAngle.toFixed(3) + "°"
        }
    }

}

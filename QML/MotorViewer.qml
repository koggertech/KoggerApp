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
                    color: theme.controlBackColor

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
                            backColor: ConnectionStatus ? "green" : theme.controlSolidBackColor
                            visible: LinkType === 1

                            onClicked: {
                                if (ConnectionStatus) {
                                    linkManagerWrapper.closeLink(Uuid)
                                } else {
                                    linkManagerWrapper.openAsSerial(Uuid)
                                }

                                // Анимация изменения цвета фона
                                if (ConnectionStatus) {
                                    closeAnimation.start()
                                } else {
                                    openAnimation.start()
                                }
                            }
                        }
                    }
                }

                ColorAnimation {
                    id: openAnimation
                    target: backgroundRect
                    property: "color"
                    to: "green"
                    duration: 500
                }

                ColorAnimation {
                    id: closeAnimation
                    target: backgroundRect
                    property: "color"
                    to: theme.controlBackColor
                    duration: 500
                }
            }
        }
    }
}

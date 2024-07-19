import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isAddressSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            id:title
            titleText: qsTr("Bus settings")
            closeble: false
        }

        ColumnLayout {
            visible: title.isOpen

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                Text {
                    text: qsTr("Bus address:")
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    Layout.rightMargin: 20
                    width: 120
                    from: 0
                    to: 255
                    stepSize: 1
                    value: dev.busAddress
                    onValueChanged: {
                        dev.busAddress = value
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                SpinBoxCustom {
                    id: spinDev
                    width: 100
                    from: 0
                    to: 255
                    stepSize: 1
                    value: dev.devAddress
                    onValueChanged: {
                    }
                }

                CButton {
                    text: qsTr("Set Device's address")

                    onClicked: {
                        dev.devAddress = spinDev.value
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                CCombo  {
                    id: baudrateCombo
                    Layout.fillWidth: true
                    model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]
                    currentIndex: 4
                }

                CButton {
                    text: qsTr("Set baudrate")

                    onClicked: {
                        dev.baudrate = Number(baudrateCombo.currentText)
                    }
                }
            }


            RowLayout {
                visible: false
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                Text {
                    text: qsTr("Set default device's address:")
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    id: spinDevDef
                    width: 100
                    from: 0
                    to: 14
                    stepSize: 1
                    value: dev.devDefAddress
                    onValueChanged: {
                    }
                }

                CButton {
                    Layout.preferredWidth: 60
                    text: "Set"

                    onClicked: {
                        dev.devDefAddress = spinDevDef.value
                    }
                }
            }
        }
    }
}

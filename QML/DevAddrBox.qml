import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

Item {
    id: control
    Layout.preferredHeight: columnItem.height

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Bus address"
        }

        ColumnLayout {
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                Text {
                    text: "Target device's address:"
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    Layout.rightMargin: 30
                    width: 120
                    from: 0
                    to: 14
                    stepSize: 1
                    value: sonarDriver.busAddress
                    onValueChanged: {
                        sonarDriver.busAddress = value
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                Text {
                    text: "Set device's address:"
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    id: spinDev
                    width: 120
                    from: 0
                    to: 14
                    stepSize: 1
                    value: sonarDriver.devAddress
                    onValueChanged: {
                    }
                }

                CButton {
                    Layout.preferredWidth: 60
                    text: "Set"

                    onClicked: {
                        sonarDriver.devAddress = spinDev.value
                    }
                }
            }


            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10

                Text {
                    text: "Set default device's address:"
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    id: spinDevDef
                    width: 120
                    from: 0
                    to: 14
                    stepSize: 1
                    value: sonarDriver.devDefAddress
                    onValueChanged: {
                    }
                }

                CButton {
                    Layout.preferredWidth: 60
                    text: "Set"

                    onClicked: {
                        sonarDriver.devDefAddress = spinDevDef.value
                    }
                }
            }
        }
    }
}

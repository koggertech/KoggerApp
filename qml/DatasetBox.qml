import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isDatasetSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: qsTr("Dataset")
            Layout.fillWidth: true
        }

        GridLayout {
            Layout.margins: 15
            Layout.fillWidth: true
            rowSpacing: 0

            ColumnLayout {
                Layout.fillWidth: true

                RowLayout {
                    Text {
                        x: 35
                        y: height
                        text: qsTr("Channel #1, ms:")
                        padding: 10
                        color: "#808080"
                        font.pixelSize: 16
                    }

                    SpinBoxCustom {
                        x: 0
                        y: 0
                        width: 130
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: dev.ch1Period
                        onValueChanged: {
                            dev.ch1Period = value
                        }
                    }
                }

                DatasetCheckBox {
                    Layout.bottomMargin: 20
                    channelNumber: 1
                    dev: control.dev
                }

                RowLayout {
                    Text {
                        x: 35
                        y: height
                        text: qsTr("Channel #2, ms:")
                        padding: 10
                        color: "#808080"
                        font.pixelSize: 16
                    }

                    SpinBoxCustom {
                        x: 0
                        y: 0
                        width: 130
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: dev.ch2Period
                        onValueChanged: {
                            dev.ch2Period = value
                        }
                    }
                }

                DatasetCheckBox {
                    channelNumber: 2
                    dev: control.dev
                }
            }
        }
    }
}

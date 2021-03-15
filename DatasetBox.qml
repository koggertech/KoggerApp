import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


Item {
    id: control
    Layout.preferredHeight: columnItem.height

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Dataset"
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
                        text: "Channel #1, ms:"
                        padding: 10
                        color: "#808080"
                        font.pixelSize: 16
                    }

//                    CSlider {
//                        Layout.fillWidth: true
//                        x: 0
//                        y: 0
//                        horizontalPadding: 30
//                        lineStyle: 0

//                        stepSize: 1.0
//                        value: sonarDriver.ch1PeriodSlider
//                        to: sonarDriver.ch1PeriodSliderCount
//                        onValueChanged: {
//                            sonarDriver.ch1PeriodSlider = value
//                        }
//                    }

                    SpinBoxCustom {
                        x: 0
                        y: 0
                        width: 130
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: sonarDriver.ch1Period
                        onValueChanged: {
                            sonarDriver.ch1Period = value
                        }
                    }
                }

                DatasetCheckBox {
                    Layout.bottomMargin: 20
                    channelNumber: 1
                }


                RowLayout {

                    Text {
                        x: 35
                        y: height
                        text: "Channel #2, ms:"
                        padding: 10
                        color: "#808080"
                        font.pixelSize: 16
                    }

//                    CSlider {
//                        Layout.fillWidth: true
//                        x: 0
//                        y: 0
//                        width: 440
//                        horizontalPadding: 30
//                        lineStyle: 0

//                        stepSize: 1.0
//                        value: sonarDriver.ch2PeriodSlider
//                        to: sonarDriver.ch2PeriodSliderCount
//                        onValueChanged: {
//                            sonarDriver.ch2PeriodSlider = value
//                        }
//                    }

                    SpinBoxCustom {
                        x: 0
                        y: 0
                        width: 130
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: sonarDriver.ch2Period
                        onValueChanged: {
                            sonarDriver.ch2Period = value
                        }
                    }

                }

                DatasetCheckBox {
                    channelNumber: 2
                }
            }
        }
    }
}

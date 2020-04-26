import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuViewer {
    id: viewerConnection
    width: scrollBar.width
    height: scrollBar.height + 20

    ScrollBar {

    }

    Connections {
        target: core

        onConnectionChanged: {
            connectionButton.connection = core.isOpenConnection()
        }
    }



    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + fileDialog.fileUrls)
            core.openConnectionAsFile(fileDialog.fileUrl)
        }
        onRejected: {
            console.log("Canceled")
        }
    }

    ScrollView {
        id: scrollBar
        y:10
        width: grid.width + 40
        height: 500
        clip: true
        padding: 20

        ColumnLayout {
            id: grid

            RowLayout {
                Layout.margins: 6
                Layout.fillWidth: true

                CCombo  {
                    id: connectionTypeCombo
                    Layout.fillWidth: true
                    model: ["SERIAL"]

                    Settings {
                        property alias connectionType: connectionTypeCombo.currentIndex
                    }
                }

                CCombo  {
                    id: portCombo
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
                        property alias connectionType: portCombo.currentIndex
                    }
                }

                CCombo  {
                    id: baudrateCombo
                    visible: connectionTypeCombo.currentText === "SERIAL"
                    model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]

                    Settings {
                        property alias serialBaudrate: baudrateCombo.currentIndex
                    }
                }

                CButton {
                    id: connectionButton
                    property bool connection: false
                    width: 30


                    text: ""

                    onClicked: {
                        if(connection) {
                            core.closeConnection()
                        } else {
                            if(connectionTypeCombo.currentText === "SERIAL") {
                                core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText))
                            } else if(connectionTypeCombo.currentText === "FILE") {
                                fileDialog.open()
                            }
                        }
                    }

                    onConnectionChanged: {
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

            UpgradeBox {
                Layout.margins: 6
                Layout.fillWidth: true
//                visible: connectionButton.connection
            }

            RowLayout {

                CButton {
                    Layout.margins: 6
                    Layout.fillWidth: false
                    implicitWidth: 110
                    implicitHeight: 30
                    font.pointSize: 16;
                    text: "FLASH"

                    onClicked: {
                        sonarDriver.flashSettings()
                    }
                }

                CButton {
                    Layout.margins: 6
                    Layout.fillWidth: false
                    implicitWidth: 110
                    implicitHeight: 30
                    font.pointSize: 16;
                    text: "RESET"

                    onClicked: {
                        sonarDriver.resetSettings()
                    }
                }

                CButton {
                    Layout.margins: 6
                    Layout.fillWidth: false
                    implicitWidth: 110
                    implicitHeight: 30
                    font.pointSize: 16;
                    text: "REBOOT"

                    onClicked: {
                        sonarDriver.reboot()
                    }
                }
            }



            TitleMeasurement {
                id: titleDatasetChart
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "CHART"
                onPositionSwitchChanged: {
                    sonarDriver.datasetChart = positionSwitch
                }
            }

            AdjBox {
                id: sliderSample
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "SAMPLES"
                sliderStepCount: sonarDriver.chartSamplSliderCount
                sliderValue: sonarDriver.chartSamplSlider
                textValue: sonarDriver.chartSamples

                function onSliderValueChanged(value) {
                    sonarDriver.chartSamplSlider = value
                }
            }

            AdjBox {
                id: sliderResolution
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "RESOLUTION"
                sliderStepCount: sonarDriver.chartResolutionSliderCount
                sliderValue: sonarDriver.chartResolutionSlider
                textValue: sonarDriver.chartResolution

                function onSliderValueChanged(value) {
                    sonarDriver.chartResolutionSlider = value
                }
            }

            TitleMeasurement {
                id: titleDatasetDist
                //visible: connectionButton.connection
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "DIST"
                onPositionSwitchChanged: {
                    sonarDriver.datasetDist = positionSwitch
                }
            }

            TitleMeasurement {
                id: titleDatasetSDDBT
                //visible: connectionButton.connection
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "SDDBT"
                onPositionSwitchChanged: {
                    sonarDriver.datasetSDDBT = positionSwitch
                }
            }

            TitleMeasurement {
                id: titleDatasetTemp
//                visible: connectionButton.connection
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "TEMP"
                onPositionSwitchChanged: {
                    sonarDriver.datasetTemp = positionSwitch
                }
            }

            Connections {
                target: sonarDriver
                onDatasetChanged: {
                    titleDatasetChart.setPos(sonarDriver.datasetChart)
                    titleDatasetDist.setPos(sonarDriver.datasetDist)
                    titleDatasetSDDBT.setPos(sonarDriver.datasetSDDBT)
                    titleDatasetTemp.setPos(sonarDriver.datasetTemp)
                }
            }

            TitleSetup{
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "DATASET"
            }

            AdjBox {
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "PERIOD CH1"
                sliderStepCount: sonarDriver.ch1PeriodSliderCount
                sliderValue: sonarDriver.ch1PeriodSlider
                textValue: sonarDriver.ch1Period

                function onSliderValueChanged(value) {
                    sonarDriver.ch1PeriodSlider = value
                }
            }

            AdjBox {
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "PERIOD CH2"
                sliderStepCount: sonarDriver.ch2PeriodSliderCount
                sliderValue: sonarDriver.ch2PeriodSlider
                textValue: sonarDriver.ch2Period

                function onSliderValueChanged(value) {
                    sonarDriver.ch2PeriodSlider = value
                }
            }

            TitleSetup{
                Layout.fillWidth: true
                Layout.topMargin: 20
                textTitle: "TRANSDUCER"
            }

            AdjBox {
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "FREQ"
                sliderStepCount: sonarDriver.transFreqSliderCount
                sliderValue: sonarDriver.transFreqSlider
                textValue: sonarDriver.transFreq

                function onSliderValueChanged(value) {
                    sonarDriver.transFreqSlider = value
                }
            }

            AdjBox {
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "PULSE"
                sliderStepCount: sonarDriver.transPulseSliderCount
                sliderValue: sonarDriver.transPulseSlider
                textValue: sonarDriver.transPulse

                function onSliderValueChanged(value) {
                    sonarDriver.transPulseSlider = value
                }
            }

            AdjBox {
                Layout.margins: 6
                Layout.fillWidth: true
                //visible: connectionButton.connection
                textTitle: "BOOST"
                sliderStepCount: sonarDriver.transBoostSliderCount
                sliderValue: sonarDriver.transBoostSlider
                textValue: sonarDriver.transBoost

                function onSliderValueChanged(value) {
                    sonarDriver.transBoostSlider = value
                }
            }



        }

    }
}

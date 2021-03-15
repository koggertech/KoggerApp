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
            titleText: "Chart"
            Layout.fillWidth: true

            RowLayout {
                id: switchDatasetChart
                property int lastChannel: 1

                CCheck {
                    id:switch_ch1
                    text: "On"
                    checked: sonarDriver.datasetChart > 0
                    onCheckedChanged: {
                        if(checked == true && sonarDriver.datasetChart == 0) {
                            sonarDriver.datasetChart = switchDatasetChart.lastChannel
                        } else if(checked == false && sonarDriver.datasetChart > 0) {
                            switchDatasetChart.lastChannel = sonarDriver.datasetChart
                            sonarDriver.datasetChart = 0
                        }
                    }
                }

                CButton {
                    text: "Shot"
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 22
                    Layout.leftMargin: 10

                    onClicked: {
                        sonarDriver.requestChart();
                    }
                }


            }
        }

        GridLayout {
            Layout.margins: 15
            Layout.fillWidth: true
            Layout.preferredHeight: 250
            rowSpacing: 0

            Canvas {
                id: borderCanvas
                x: 0
                y: 0
                Layout.fillWidth: true
                height: 250
                contextType: "2d"
                opacity: 1
                property real offsetRight: 40
                property real tickness: 2
                property real heightSliderBox: 130
                property real heightChart: height - heightSliderBox
                property real heightSliderResol: 50
                property real posSliderResol: heightChart + heightSliderResol - 10
                property real heightSliderSamples: 90
                property real posSliderSamples: heightChart + heightSliderSamples - 10
                property real heightSliderOffset: 130
                property real posSliderOffset: heightChart + heightSliderOffset - 10

//                CSlider {
//                    id: sliderResol
//                    x: borderCanvas.offsetRight + 35 - 2
//                    y: borderCanvas.posSliderResol - height/2
//                    width: 440
//                    horizontalPadding: 30
//                    lineStyle: 1

//                    stepSize: 1.0
//                    value: sonarDriver.chartResolutionSlider
//                    to: sonarDriver.chartResolutionSliderCount
//                    onValueChanged: {
//                        sonarDriver.chartResolutionSlider = value
//                    }
//                }

                SpinBoxCustom {
                    x: borderCanvas.offsetRight + 35 - 2
                    y: borderCanvas.posSliderResol - height/2
                    width: 110
                    from: 10
                    to: 100
                    stepSize: 10
                    value: sonarDriver.chartResolution
                    onValueChanged: {
                        sonarDriver.chartResolution = value
                    }
                }

                Text {
                    x: borderCanvas.offsetRight + 25
                    y: borderCanvas.posSliderResol - height - 5
                    text: "Resolution, mm"
                    padding: 10
                    color: "#808080"
                    font.pixelSize: 14
                }

//                CSlider {
//                    x: borderCanvas.offsetRight + 25 - 2
//                    y: borderCanvas.posSliderSamples - height/2
//                    width: 440
//                    horizontalPadding: 30
//                    lineStyle: 0

//                    stepSize: 1.0
//                    value: sonarDriver.chartSamplSlider
//                    to: sonarDriver.chartSamplSliderCount
//                    onValueChanged: {
//                        sonarDriver.chartSamplSlider = value
//                    }
//                }

                SpinBoxCustom {
                    x: borderCanvas.width - 25 - width - 2
                    y: borderCanvas.posSliderSamples - height/2
                    width: 110
                    from: 100
                    to: 3000
                    stepSize: 100
                    value: sonarDriver.chartSamples
                    onValueChanged: {
                        sonarDriver.chartSamples = value
                    }
                }

                Text {
                    x: borderCanvas.offsetRight + 340
                    y: borderCanvas.posSliderSamples - height - 5
                    text: "Number of Samples"
                    padding: 10
                    color: "#808080"
                    font.pixelSize: 14
                }

//                CSlider {
//                    x: borderCanvas.offsetRight + 15 - 2
//                    y: borderCanvas.posSliderOffset - height/2
//                    width: 460
//                    horizontalPadding: 30
//                    lineStyle: 1

//                    stepSize: 1.0
//                    value: sonarDriver.chartOffsetSlider
//                    to: sonarDriver.chartOffsetSliderCount
//                    onValueChanged: {
//                        sonarDriver.chartOffsetSlider = value
//                    }
//                }

                SpinBoxCustom {
                    x: borderCanvas.offsetRight + 15 - 2
                    y: borderCanvas.posSliderOffset - height/2
                    width: 110
                    from: 0
                    to: 5000
                    stepSize: 100
                    value: sonarDriver.chartOffset
                    onValueChanged: {
                        sonarDriver.chartOffset = value
                    }
                }

                Text {
                    x: borderCanvas.offsetRight + 10
                    y: borderCanvas.posSliderOffset - height - 5
                    text: "Offset of Samples"
                    padding: 10
                    color: "#808080"
                    font.pixelSize: 14
                }

                onPaint: {
                    context.reset();

                    context.lineWidth = 2
                    context.strokeStyle = "#808080"

                    context.beginPath()
                    context.arc(4, heightChart + 1, 10, -1, 1, false)
                    context.stroke()

                    context.beginPath()
                    context.arc(4, heightChart + 1, 15, -1, 1, false)
                    context.stroke()

                    context.beginPath()
                    context.arc(4, heightChart + 1, 20, -1, 1, false)
                    context.stroke()

                    context.beginPath()
                    context.arc(5, heightChart + 1, 2, 0, Math.PI * 2, false)
                    context.stroke()

                    context.beginPath()

                    context.fillStyle = "#808080"

                    context.fillRect(offsetRight, 0, tickness, height)
                    context.fillRect(0, heightChart, width, tickness)

                    context.fillStyle = "#606060"
                    context.fillRect(offsetRight + 26, heightChart - 10, 1.5, heightSliderResol + 10)

                    context.fillStyle = "#808080"
                    context.fillRect(offsetRight, posSliderResol - 1, 35, 2)

                    context.moveTo(offsetRight + 2, posSliderResol)
                    context.lineTo(offsetRight + 2 + 10, posSliderResol - 4)
                    context.lineTo(offsetRight + 2 + 10, posSliderResol + 4)
                    context.closePath()

                    context.moveTo(offsetRight + 26, posSliderResol)
                    context.lineTo(offsetRight + 26 - 10, posSliderResol - 4)
                    context.lineTo(offsetRight + 26 - 10, posSliderResol + 4)
                    context.closePath()

                    context.fillStyle = "#606060"
                    context.fillRect(width - 5, heightChart - 10, 1.5, heightSliderSamples + 10)

                    context.fillStyle = "#808080"
                    context.fillRect(offsetRight + 2, posSliderSamples - 1, 352, 2)
                    context.fillRect(width - 5, posSliderSamples - 1, - 35, 2)

                    context.moveTo(offsetRight + 2, posSliderSamples)
                    context.lineTo(offsetRight + 2 + 10, posSliderSamples - 4)
                    context.lineTo(offsetRight + 2 + 10, posSliderSamples + 4)
                    context.closePath()

                    context.moveTo(width - 5, posSliderSamples)
                    context.lineTo(width - 5 - 10, posSliderSamples - 4)
                    context.lineTo(width  - 5 - 10, posSliderSamples + 4)
                    context.closePath()

                    context.fillStyle = "#606060"
                    context.fillRect(4, heightChart - 10, 1.5, heightSliderOffset + 10)

                    context.fillStyle = "#808080"
                    context.fillRect(5, posSliderOffset - 1, offsetRight - 5 + 35, 2)

                    context.moveTo(5, posSliderOffset)
                    context.lineTo(5 + 1 + 10, posSliderOffset - 4)
                    context.lineTo(5 + 1 + 10, posSliderOffset + 4)
                    context.closePath()

                    context.moveTo(offsetRight, posSliderOffset)
                    context.lineTo(offsetRight - 10, posSliderOffset - 4)
                    context.lineTo(offsetRight - 10, posSliderOffset + 4)
                    context.closePath()

                    context.fill()
                }
            }




        }
    }
}

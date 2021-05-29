import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isDistSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Distance"
            Layout.fillWidth: true

            RowLayout {
                id: switchDatasetDist
                property int lastDistChannel: 1

                CCheck {
                    id:switchBinnary
                    text: "Bin"
                    checked: dev.datasetDist > 0
                    onCheckedChanged: {
                        if(checked == true && dev.datasetDist === 0) {
                            dev.datasetDist = switchDatasetDist.lastDistChannel
                        } else if(checked == false && dev.datasetDist > 0) {
                            switchDatasetDist.lastDistChannel = dev.datasetDist
                            dev.datasetDist = 0
                        }
                    }
                }

                CCheck {
                    id:switchNMEA
                    text: "NMEA"
                    checked: dev.datasetSDDBT > 0
                    onCheckedChanged: {
                        if(checked == true && dev.datasetSDDBT === 0) {
                            dev.datasetSDDBT = switchDatasetDist.lastDistChannel
                        } else if(checked == false && dev.datasetSDDBT > 0) {
                            switchDatasetDist.lastDistChannel = dev.datasetSDDBT
                            dev.datasetSDDBT = 0
                        }
                    }
                }

                CCheck {
                    id:switchNMEA2
                    text: "NMEA #2"
                    checked: dev.datasetSDDBT_P2 > 0
                    onCheckedChanged: {
                        if(checked == true && dev.datasetSDDBT_P2 === 0) {
                            dev.datasetSDDBT_P2 = switchDatasetDist.lastDistChannel
                        } else if(checked == false && dev.datasetSDDBT_P2 > 0) {
                            switchDatasetDist.lastDistChannel = dev.datasetSDDBT_P2
                            dev.datasetSDDBT_P2 = 0
                        }
                    }
                }

                CButton {
                    text: "Shot"
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 22
                    Layout.leftMargin: 10

                    onClicked: {
                        dev.requestDist();
                    }
                }
            }
        }

        ColumnLayout {
            Layout.margins: 15
            Layout.fillWidth: true

            RowLayout {
                Canvas {
                    id: borderCanvas
                    x: 0
                    y: 0
                    Layout.fillWidth: true
                    height: 120
                    contextType: "2d"
                    opacity: 1
                    property real offsetRight: 5
                    property real tickness: 2
                    property real heightSliderBox: 100
                    property real heightChart: height - heightSliderBox
                    property real heightSliderSamples: 100
                    property real posSliderSamples: heightChart + heightSliderSamples - 10
                    property real heightSliderOffset: 60
                    property real posSliderOffset: heightChart + heightSliderOffset - 10

                    SpinBoxCustom {
                        x: borderCanvas.width - 25 - width - 2
                        y: borderCanvas.posSliderSamples - height/2
                        width: 130
                        from: 0
                        to: 50000
                        stepSize: 1000
                        value: dev.distMax
                        onValueChanged: {
                            dev.distMax = value
                        }
                    }

                    Text {
                        x: borderCanvas.offsetRight + 370
                        y: borderCanvas.posSliderSamples - height - 5
                        text: "Max distance, mm"
                        padding: 10
                        color: "#808080"
                        font.pixelSize: 14
                    }

                    SpinBoxCustom {
                        x: borderCanvas.offsetRight + 30 - 2
                        y: borderCanvas.posSliderOffset - height/2
                        width: 130
                        from: 0
                        to: 50000
                        stepSize: 100
                        value: dev.distDeadZone
                        onValueChanged: {
                            dev.distDeadZone = value
                        }
                    }

                    Text {
                        x: borderCanvas.offsetRight + 30
                        y: borderCanvas.posSliderOffset - height - 5
                        text: "Dead zone, mm"
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
                        context.fillRect(width - 5, heightChart - 10, 1.5, heightSliderSamples + 10)

                        context.fillStyle = "#808080"
                        context.fillRect(offsetRight + 2, posSliderSamples - 1, 370, 2)
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
                        context.fillRect(30, heightChart - 10, 1.5, heightSliderOffset + 10)

                        context.fillStyle = "#808080"
                        context.fillRect(5, posSliderOffset - 1, offsetRight - 5 + 35, 2)

                        context.moveTo(5, posSliderOffset)
                        context.lineTo(5 + 1 + 10, posSliderOffset - 4)
                        context.lineTo(5 + 1 + 10, posSliderOffset + 4)
                        context.closePath()

                        context.moveTo(30, posSliderOffset)
                        context.lineTo(30 - 10, posSliderOffset - 4)
                        context.lineTo(30 - 10, posSliderOffset + 4)
                        context.closePath()

                        context.fill()
                    }
                }
            }

            RowLayout {
                Layout.topMargin: 10
                Text {
                    text: "Confidence threshold, %"
                    color: "#808080"
                    font.pixelSize: 16
                }

                SpinBoxCustom {
                    width: 130
                    from: 0
                    to: 100
                    stepSize: 1
                    value: dev.distConfidence
                    onValueChanged: {
                        dev.distConfidence = value
                    }
                }
            }

        }
    }
}

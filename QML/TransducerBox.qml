import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isTransducerSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Transducer"
            Layout.fillWidth: true
        }

        GridLayout {
            Layout.margins: 15
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            rowSpacing: 0

            Canvas {
                id: borderCanvas
                x: 0
                y: 0
                Layout.fillWidth: true
                height: 150
                contextType: "2d"
                opacity: 1
                property real offsetRight: 15
                property real tickness: 2
                property real heightSliderBox: 80
                property real heightChart: height - heightSliderBox
                property real heightSliderResol: 80
                property real posSliderResol: heightChart + heightSliderResol - 10

                Connections {/*
                    target: control
                    onActiveChanged: borderCanvas.requestPaint()*/
                }

                Connections {
                    target: dev
                    onTransPulseChanged: borderCanvas.requestPaint()
                }

                CSlider {
                    id: sliderPulse
                    x: borderCanvas.offsetRight - 10
                    y: borderCanvas.heightChart - height/2
                    width: 490
                    height: 50
                    horizontalPadding: 20
                    lineStyle: 2

                    stepSize: 1.0
                    value: dev.transPulseSlider
                    to: dev.transPulseSliderCount
                    onValueChanged: {
                        dev.transPulseSlider = value
                    }
                }

                RowLayout {
                    Text {
                        text: "Pulse count:"
                        color: "#808080"
                        font.pixelSize: 16
                    }

                    SpinBoxCustom {
                        width: 100
                        from: 0
                        to: 30
                        stepSize: 1
                        value: dev.transPulse
                        onValueChanged: {
                            dev.transPulse = value
                        }
                    }

                    Text {
                        Layout.leftMargin: 20
                        text: "Boost:"
                        color: "#808080"
                        font.pixelSize: 16
                    }

                    SpinBoxCustom {
                        width: 100
                        from: 0
                        to: 1
                        stepSize: 1
                        value: dev.transBoost
                        onValueChanged: {
                            dev.transBoost = value
                        }

                        property var items: ["Off", "On"]

                        validator: RegExpValidator {
                            regExp: new RegExp("(Off|On)", "i")
                        }

                        textFromValue: function(value) {
                            return items[value];
                        }

                        valueFromText: function(text) {
                            for (var i = 0; i < items.length; ++i) {
                                if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                                    return i
                            }
                            return sb.value
                        }
                    }
                }

                SpinBoxCustom {
                    x: borderCanvas.offsetRight + 35 - 2
                    y: borderCanvas.posSliderResol - height/2
                    width: 130
                    from: 100
                    to: 6000
                    stepSize: 5
                    value: dev.transFreq
                    onValueChanged: {
                        dev.transFreq = value
                    }
                }

                Text {
                    x: borderCanvas.offsetRight + 35
                    y: borderCanvas.posSliderResol - height - 5
                    text: "Frequency, kHz"
                    padding: 10
                    color: "#808080"
                    font.pixelSize: 16
                }

                onPaint: {
                    context.reset();

                    context.lineWidth = 3
                    context.strokeStyle = "#808080"

                    context.beginPath()
                    context.fillStyle = "#606060"

                    context.fillStyle = "#606060"
                    context.fillRect(offsetRight, heightChart, 1.5, heightSliderResol + 10)
                    context.fillRect(offsetRight + 16, heightChart, 1.5, heightSliderResol + 10)

                    context.fillStyle = "#808080"
                    context.fillRect(offsetRight, posSliderResol - 1, 35, 2)

                    context.moveTo(offsetRight + 1, posSliderResol)
                    context.lineTo(offsetRight + 1 - 10, posSliderResol - 4)
                    context.lineTo(offsetRight + 1 - 10, posSliderResol + 4)
                    context.closePath()

                    context.moveTo(offsetRight + 1 + 15, posSliderResol)
                    context.lineTo(offsetRight + 1 + 15 + 10, posSliderResol - 4)
                    context.lineTo(offsetRight + 1 + 15 + 10, posSliderResol + 4)
                    context.closePath()

                    context.fill()

//                    if (sonarDriver.transPulse > 0) {
////                        context.fillRect(offsetRight + dev.transPulse*15 + 1, 30, tickness, 50)
//                        context.beginPath()
//                        context.arc(offsetRight + dev.transPulse*15 + 2, heightChart, 2, 0, Math.PI * 2, false)
//                        context.stroke()
//                    }

                    context.beginPath()
                    context.arc(offsetRight + 2, heightChart, 4, 0, Math.PI * 2, false)
                    context.fill()

                    context.beginPath()
                    context.arc(offsetRight + 2 + 15, heightChart, 4, 0, Math.PI * 2, false)
                    context.fill()

                    context.fillRect(0, heightChart - 1, offsetRight, tickness)

//                    context.beginPath()
//                    context.arc(20 + 15, 54 - sonarDriver.transBoost*20, 4, 0, Math.PI * 2, false)
//                    context.fill()

                    context.beginPath()
                    context.arc(offsetRight + dev.transPulse*15 + 2, heightChart, 4, 0, Math.PI * 2, false)
                    context.fill()

//                    context.fillRect(offsetRight + sonarDriver.transPulse*15, heightChart, 4, tickness)

                    var height_sin = dev.transBoost ? 120 : 50

                    for (var i = 0; i < dev.transPulse; i++)  {
                        var x_offset = i*15;

                        context.beginPath();
                        context.moveTo(offsetRight + 2 + x_offset, heightChart);
                        context.bezierCurveTo(offsetRight + x_offset + 15/3 + 2, heightChart - height_sin, offsetRight + x_offset + 15*2/3 + 2, heightChart + height_sin, offsetRight + x_offset + 15 + 2, heightChart);
                        context.stroke();
                    }
                }
            }
        }
    }
}

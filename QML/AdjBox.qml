import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: control
    Layout.fillWidth: true
    height: 50
    property string textTitle: "VALUE NAME:"
    property string textValue: "Value"
    property real sliderStepSize: 0.0
    property real sliderValue: 0.0

    Rectangle {
        id: backRect
        width: control.width
        height: control.height

        color: "#104060"
        opacity: 0.9
    }

    RowLayout {
        Layout.fillWidth: true
        width: control.width

        Text {
            width: 100
            padding: 10
            text: textTitle
            font.pixelSize: 16
            color: "#F07000"
        }

        ColumnLayout {
            Text {
                id: text
                padding: 2
                Layout.fillWidth: true
                text: textValue
                font.pixelSize: 20
                color: "#F07000"
                horizontalAlignment: Text.AlignHCenter
            }

            CSlider{
                id: slider
                Layout.fillWidth: true
                horizontalPadding: 40

                stepSize: sliderStepSize

                onValueChanged: {
                    sliderValue = slider.value
                }
            }
        }
    }
}

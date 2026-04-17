import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: control
    Layout.fillWidth: true
    height: 64
    property string textTitle: qsTr("VALUE NAME:")
    property string textValue: ""
    property real sliderStepCount: 0
    property real sliderValue: 0

    function onSliderValueChanged(value) {
    }

    AdjBoxBack {
        id: backRect
        width: control.width
        height: control.height
    }

    ColumnLayout {
        Layout.fillWidth: true
        width: control.width
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: control.width

        RowLayout {
            Text {
                id: textT
                padding: 8
                text: textTitle
                font.pixelSize: 16
                color: "#F07000"
            }

            Text {
                id: text
                Layout.fillWidth: true
                padding: 8
                text: textValue
                font.pixelSize: 20
                color: "#F07000"
            }
        }

        CSlider{
            id: slider
            value: sliderValue
            Layout.fillWidth: true
            horizontalPadding: 40
            stepSize: 1.0
            to: sliderStepCount
            onValueChanged: {
                onSliderValueChanged(value)
            }
        }
    }
}

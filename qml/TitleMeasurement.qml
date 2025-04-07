import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: control
    Layout.fillWidth: true
    height: 32
    property int setPositionSwitch: 0
    property int positionSwitch: 0
    property string textTitle: qsTr("TITLE")

    function setPos(pos) {
        if(pos === 0) {
            switch_off.checked = true
        } else if(pos === 1) {
            switch_ch1.checked = true
        } else if(pos === 2) {
            switch_ch2.checked = true
        }
        positionSwitch = pos
    }

    onSetPositionSwitchChanged: {
    }

    Canvas {
        id: borderCanvas
        x: 0
        y: 0
        width: control.width
        height: control.height
        contextType: "2d"
        opacity: 1

        onPaint: {
            context.reset();

            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width, 2);
            context.lineTo(text.width, 2);
            context.lineTo(text.width, height);
            context.lineTo(0, height);
            context.closePath();

            context.fillStyle = "#70C0F0"
            context.fill();
        }
    }

    RowLayout {
        Text {
            id: text
            Layout.alignment: Qt.AlignTop
            text: textTitle
            padding: 6
            font.pixelSize: 18
            color: "black"
        }

        ButtonGroup {
            buttons: column.children
            onClicked: {
                if(button.text === qsTr("OFF")) {
                    positionSwitch = 0
                } else if(button.text === qsTr("CH1")) {
                    positionSwitch = 1
                } else if(button.text === qsTr("CH2")) {
                    positionSwitch = 2
                }
            }
        }

        Row {
            id: column
            CRButton {
                id:switch_off
                checked: setPositionSwitch == 0
                text: qsTr("OFF")
            }

            CRButton {
                id:switch_ch1
                checked: setPositionSwitch == 1
                text: qsTr("CH1")
            }

            CRButton {
                id:switch_ch2
                checked: setPositionSwitch == 2
                text: qsTr("CH2")
            }
        }
    }
}

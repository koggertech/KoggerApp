import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


SpinBox {
    id: control
    value: 50
    editable: true
    from: 20
    to: 30000
    font.pixelSize: 16
    padding: 2

    implicitHeight: styleSet.controllHeight - 6
    implicitWidth: 110

    StyleSet {
        id: styleSet
    }

    valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text); }

    contentItem: TextInput {
        id:textInput
        text: control.textFromValue(control.value, control.locale)
        width: control.width
        font: control.font
        color: styleSet.colorControllText
        selectionColor: styleSet.colorControllTextActive
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        selectByMouse: true

        onTextEdited: {
            control.value = control.valueFromText(textInput.text, control.locale)
        }

        autoScroll: false

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.onPressedChanged: upCanvas.requestPaint()

    up.indicator: Canvas {
        id: upCanvas
        x: parent.width - width
        y: 0
        opacity: 1
        height: parent.height
        width: parent.height
        contextType: "2d"

        property bool pressed: control.up.pressed

//        Connections {
//            target: control.up
//            onPressedChanged: upCanvas.requestPaint()
//        }

        onEnabledChanged: {
            upCanvas.requestPaint()
        }

        onPaint: {
            context.reset();

            var mid_height = height/2
            var width_button = height/3

            context.moveTo(width, mid_height);
            context.lineTo(width - width_button, 0);
            context.lineTo(0, 0);
            context.lineTo(0, height);
            context.lineTo(width - width_button, height);

            context.closePath();
            context.fillStyle = enabled ? (pressed ? "#909090" : styleSet.colorControllBackActive) : "#505050"
            context.fill();

            context.lineWidth = 1
            context.strokeStyle = enabled ? (pressed ? "#909090" : "#808080") : "#606060"
            context.stroke()


            var mid_icon_x = width/2 - 1
            var radius_icon = 6
            var tickness_icon = 1

            context.beginPath();
            context.moveTo(mid_icon_x - radius_icon, mid_height - tickness_icon);
            context.lineTo(mid_icon_x - tickness_icon, mid_height - tickness_icon);
            context.lineTo(mid_icon_x - tickness_icon, mid_height - radius_icon);
            context.lineTo(mid_icon_x + tickness_icon, mid_height - radius_icon);
            context.lineTo(mid_icon_x + tickness_icon, mid_height - tickness_icon);
            context.lineTo(mid_icon_x + radius_icon, mid_height - tickness_icon);

            context.lineTo(mid_icon_x + radius_icon, mid_height + tickness_icon);
            context.lineTo(mid_icon_x + tickness_icon, mid_height + tickness_icon);
            context.lineTo(mid_icon_x + tickness_icon, mid_height + radius_icon);
            context.lineTo(mid_icon_x - tickness_icon, mid_height + radius_icon);
            context.lineTo(mid_icon_x - tickness_icon, mid_height + tickness_icon);
            context.lineTo(mid_icon_x - radius_icon, mid_height + tickness_icon);

            context.closePath();
            context.fillStyle = enabled ? styleSet.colorControllBack : "#505050"
            context.fill();
        }
    }

//    down.indicator: Rectangle {
//        id: downCanvas
//        x: control.mirrored ? parent.width - width : 0
//        opacity: 1
//        height: parent.height
//        width: parent.height
//        color: control.down.pressed ? styleSet.colorControllBackActive : styleSet.colorControllBack
//        border.color: control.down.pressed ? styleSet.colorControllBorderActive: styleSet.colorControllBorder
//    }

    down.onPressedChanged: downCanvas.requestPaint()

    down.indicator: Canvas {
        id: downCanvas
        x: 0
        y: 0
        opacity: 1
        height: parent.height
        width: parent.height
        contextType: "2d"
        property bool pressed: control.down.pressed

//        Connections {
//            target: control.down
//            onPressedChanged: downCanvas.requestPaint()
//        }

        onEnabledChanged: {
            downCanvas.requestPaint()
        }

        onPaint: {
            context.reset();

            var mid_height = height/2
            var width_button = height/3

            context.moveTo(0, mid_height);
            context.lineTo(width_button, 0);
            context.lineTo(width, 0);
            context.lineTo(width, height);
            context.lineTo(width_button, height);

            context.closePath();
            context.fillStyle = enabled ? (pressed ? "#909090" : styleSet.colorControllBackActive) : "#505050"
            context.fill();

            context.lineWidth = 1
            context.strokeStyle = enabled ? (pressed ? "#909090" : "#808080") : "#606060"
            context.stroke()


            var mid_icon_x = width/2 + 1
            var radius_icon = 6
            var tickness_icon = 1

            context.beginPath();
            context.moveTo(mid_icon_x - radius_icon, mid_height + tickness_icon);
            context.lineTo(mid_icon_x + radius_icon, mid_height + tickness_icon);
            context.lineTo(mid_icon_x + radius_icon, mid_height - tickness_icon);
            context.lineTo(mid_icon_x - radius_icon, mid_height - tickness_icon);
            context.closePath();
            context.fillStyle =  enabled ? styleSet.colorControllBack : "#505050"
            context.fill();

        }
    }

    background: Rectangle {
        x: down.indicator.width + 2
        y: 0
        width: control.width - downCanvas.width - upCanvas.width - 4
        height: control.height
        opacity: styleSet.controllBackOpacity*0.7

        color: styleSet.colorControllBack
        border.color: styleSet.colorControllBorder
    }

//    background:  Canvas {
//        id: borderCanvas
//        x: down.indicator.width
//        y: 0
//        width: control.width - downCanvas.width - upCanvas.width
//        height: control.height
//        contextType: "2d"

//        Connections {
//            target: control
//            onActiveChanged: borderCanvas.requestPaint()
//        }

//        onPaint: {
//            context.reset();

//            var mid_height = height/2
//            var width_button = height/3

//            context.moveTo(0, mid_height);
//            context.lineTo(width_button, 0);
//            context.lineTo(width - width_button, 0);
//            context.lineTo(width, mid_height);
//            context.lineTo(width - width_button, height);
//            context.lineTo(width_button, height);

//            context.closePath();
//            context.fillStyle = styleSet.colorControllBorder
//            context.fill();
//        }
//    }
}

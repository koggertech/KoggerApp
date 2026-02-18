import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


SpinBox {
    id: control

    property bool spinner: true
    property bool isValid: true

    property int devValue: Number.MIN_VALUE
    property bool isDriverChanged: true

    value: 50
    from: 20
    to: 30000
    editable: true
    font.pixelSize: 16
    padding: 2

    implicitHeight: theme.controlHeight
    implicitWidth: implicitHeight*6

    valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text); }

    contentItem: TextInput {
        id:textInput
        text: control.textFromValue(control.value, control.locale)
        width: control.width
        font: theme.textFont
        color: theme.textColor
//        selectionColor: styleSet.colorControllTextActive
        selectedTextColor: theme.textColor
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
        visible: control.spinner
        x: parent.width - width
        y: 0
        height: parent.height
        width: control.spinner ? parent.height + 6 : 0
        contextType: "2d"
        renderTarget: Canvas.FramebufferObject
        opacity: 1

        property bool pressed: control.up.pressed

        Connections {
            target: theme

            function onThemeIDChanged() {
                upCanvas.requestPaint()
            }
        }

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
            context.fillStyle = enabled ? (pressed ? theme.controlSolidBackColor : theme.controlBackColor) : theme.menuBackColor
            context.fill();

            context.lineWidth = 1
            context.strokeStyle = enabled ? (pressed ? theme.controlSolidBorderColor : theme.controlBorderColor) : theme.menuBackColor
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
            context.fillStyle = enabled ? theme.textColor : theme.menuBackColor
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
        visible: control.spinner
        x: 0
        y: 0

        height: parent.height
        width: control.spinner ? parent.height + 6 : 0
        contextType: "2d"
        renderTarget: Canvas.FramebufferObject
        property bool pressed: control.down.pressed

        Connections {
            target: theme

            function onThemeIDChanged() {
                downCanvas.requestPaint()
            }
        }

        onEnabledChanged: {
            downCanvas.requestPaint()
        }

        onPaint: {
            if (context === null) {
                return;
            }

            context.reset();

            var mid_height = height/2
            var width_button = height/3

            context.moveTo(0, mid_height);
            context.lineTo(width_button, 0);
            context.lineTo(width, 0);
            context.lineTo(width, height);
            context.lineTo(width_button, height);

            context.closePath();
            context.fillStyle = enabled ? (pressed ? theme.controlSolidBackColor : theme.controlBackColor) : theme.menuBackColor
            context.fill();

            context.lineWidth = 1
            context.strokeStyle = enabled ? (pressed ? theme.controlSolidBorderColor : theme.controlBorderColor) : theme.menuBackColor
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
            context.fillStyle =  enabled ? theme.textColor : theme.menuBackColor
            context.fill();

        }
    }

    background: Rectangle {
        x: control.spinner ? down.indicator.width : 0
        y: 0
        width: control.spinner ? control.width - downCanvas.width - upCanvas.width : control.width
        height: control.height

        color: isValid ? theme.controlBackColor : "red"
        border.color: theme.controlBorderColor
    }

    onDevValueChanged: {
        if (value !== devValue) {
            isDriverChanged = true
            value = devValue
        }
        else {
            isDriverChanged = false
        }
    }

    Component.onCompleted: {
        isDriverChanged = false
    }
}

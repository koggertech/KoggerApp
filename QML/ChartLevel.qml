import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: control
    width: 87
    height: theme.controlHeight*8

    property int widthSlider: theme.controlHeight
    property int heightSlider: theme.controlHeight/2
    property int mouseRange: height - heightSlider*2

    property int from: 0
    property int to: 120

    property int startValue: 10
    property int stopValue: 100

    property int startPointY: valueToPosition(startValue)
    property int stopPointY: valueToPosition(stopValue)
    property int activeSlider: 1

    property color borderColor: theme.textColor
    property color backColor: theme.controlBackColor

    function valueToPosition(val) {
        return Math.round(mouseRange - val / (to - from) * mouseRange + heightSlider)
    }

    function mouseToVal(mauseCoord) {
        var val = (mouseRange - mauseCoord) * (to - from) / mouseRange
        return Math.max(Math.min(val, to), from)
    }

    function updateValue(mouseX, mouseY, pressed) {
        var centerMouse = mouseY - heightSlider
        var startCoord = centerMouse - heightSlider/2
        var stopCoord = centerMouse + heightSlider/2

        if(pressed) {
            if(startPointY == stopPointY) {
                if(mouseY > startPointY) {
                    activeSlider = 1
                } else {
                    activeSlider = 2
                }
            } else if(Math.abs(startPointY - mouseY) < Math.abs(stopPointY - mouseY)) {
                activeSlider = 1
            } else {
                activeSlider = 2
            }
        }

        if(activeSlider == 1) {
            startValue = mouseToVal(startCoord)
            if(startValue > stopValue) {
                stopValue = startValue
            }
        } else if(activeSlider == 2){
            stopValue = mouseToVal(stopCoord)
            if(stopValue < startValue) {
                startValue = stopValue;
            }
        }

        startPointY = valueToPosition(startValue)
        stopPointY = valueToPosition(stopValue)

        canvas.requestPaint()
    }

    function update() {
        canvas.requestPaint()
    }

    MouseArea {
        id:mouseArea
        anchors.fill: parent

        // hoverEnabled: true
        // onEntered: parent.color = 'yellow'
        // onExited: parent.color = 'black'

        onPressed: {
            updateValue(mouseX, mouseY, true)
        }

        onPositionChanged:  {
            updateValue(mouseX, mouseY, false)
        }
    }

    Connections {
        target: theme

        function onThemeIDChanged() {
            canvas.requestPaint()
        }
    }

    Canvas {
        id: canvas
        contextType: "2d"
        anchors.fill: parent

        onPaint: {
            context.reset();
            context.fillStyle = parent.borderColor
            context.lineWidth = 1
            context.strokeStyle = parent.borderColor

            var startPointX = width/2
            var stopPointX = width/2
            var startY = Math.round(startPointY)
            var stopY = Math.round(stopPointY)

            context.beginPath()
            context.fillStyle =  parent.backColor
            context.moveTo(startPointX - widthSlider/2, startY);
            context.lineTo(stopPointX - widthSlider/2, stopY);
            context.moveTo(startPointX + widthSlider/2, startY);
            context.lineTo(stopPointX + widthSlider/2, stopY);
            // context.fillRect(startPointX - widthSlider/2, startY,  widthSlider, stopY - startY);
            context.stroke()


            context.fillStyle =  parent.borderColor

            context.beginPath()
            context.moveTo(startPointX - widthSlider/2, startY);
            context.lineTo(startPointX + widthSlider/2, startY);
            context.lineTo(startPointX + widthSlider/2, startY + heightSlider/2);
            context.lineTo(startPointX, startY + heightSlider - 2);
            context.lineTo(startPointX - widthSlider/2, startY + heightSlider/2);
            context.closePath();
            context.fill();
            context.stroke()

            context.beginPath()
            context.moveTo(stopPointX - widthSlider/2, stopY);
            context.lineTo(stopPointX + widthSlider/2, stopY);
            context.lineTo(stopPointX + widthSlider/2, stopY - heightSlider/2);
            context.lineTo(stopPointX, stopY - heightSlider + 2);
            context.lineTo(stopPointX - widthSlider/2, stopY - heightSlider/2);
            context.closePath();
            context.fill();
            context.stroke()


            context.beginPath()
            context.moveTo(startPointX, height);
            context.lineTo(startPointX, startY + heightSlider - 2);

            context.moveTo(stopPointX, 0);
            context.lineTo(stopPointX, stopY - heightSlider + 2);

            context.stroke()
        }
    }
}

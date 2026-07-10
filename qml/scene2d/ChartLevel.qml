import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import controls
import menus

Item {
    id: control
    implicitWidth: 87
    implicitHeight: theme.controlHeight * heightCoeff

    property int orientation: Qt.Vertical
    readonly property bool _horiz: orientation === Qt.Horizontal

    property int heightCoeff: 5
    property int widthSlider: theme.controlHeight
    property int heightSlider: theme.controlHeight/2
    property int mouseRange: (_horiz ? width : height) - heightSlider*2

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
        var span = val / (to - from) * mouseRange
        return Math.round(_horiz ? (span + heightSlider)
                                 : (mouseRange - span + heightSlider))
    }

    function mouseToVal(mauseCoord) {
        var val = (_horiz ? mauseCoord : (mouseRange - mauseCoord)) * (to - from) / mouseRange
        return Math.max(Math.min(val, to), from)
    }

    function updateValue(mouseX, mouseY, pressed) {
        var mainMouse = _horiz ? mouseX : mouseY
        var centerMouse = mainMouse - heightSlider
        var startCoord = centerMouse - heightSlider/2
        var stopCoord = centerMouse + heightSlider/2

        if(pressed) {
            if(startPointY === stopPointY) {
                var grabStart = _horiz ? (mainMouse < startPointY) : (mainMouse > startPointY)
                activeSlider = grabStart ? 1 : 2
            } else if(Math.abs(startPointY - mainMouse) < Math.abs(stopPointY - mainMouse)) {
                activeSlider = 1
            } else {
                activeSlider = 2
            }
        }

        if(activeSlider === 1) {
            startValue = mouseToVal(_horiz ? centerMouse : startCoord)
            if(startValue > stopValue) {
                stopValue = startValue
            }
        } else if(activeSlider === 2){
            stopValue = mouseToVal(_horiz ? centerMouse : stopCoord)
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

    property int wheelStep: 1

    onMouseRangeChanged: {
        startPointY = valueToPosition(startValue)
        stopPointY  = valueToPosition(stopValue)
        if (canvas) canvas.requestPaint()
    }

    onStartValueChanged: {
        startPointY = valueToPosition(startValue)
        if (canvas) canvas.requestPaint()
    }
    onStopValueChanged: {
        stopPointY = valueToPosition(stopValue)
        if (canvas) canvas.requestPaint()
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        preventStealing: true

        onWheel: function(wheel) {
            var delta = wheel.angleDelta.y > 0 ? control.wheelStep : -control.wheelStep
            var mainW = control._horiz ? wheel.x : wheel.y
            var closeToStart = Math.abs(startPointY - mainW) < Math.abs(stopPointY - mainW)
            if (startPointY === stopPointY) {
                closeToStart = control._horiz ? (mainW < startPointY) : (mainW > startPointY)
            }
            if (closeToStart) {
                startValue = Math.max(from, Math.min(to, startValue + delta))
                if (startValue > stopValue) stopValue = startValue
            } else {
                stopValue = Math.max(from, Math.min(to, stopValue + delta))
                if (stopValue < startValue) startValue = stopValue
            }
            startPointY = valueToPosition(startValue)
            stopPointY  = valueToPosition(stopValue)
            canvas.requestPaint()
            wheel.accepted = true
        }

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
            if (!context)
                return;
            context.reset();
            context.fillStyle = parent.borderColor
            context.lineWidth = 1
            context.strokeStyle = parent.borderColor

            if (control._horiz) {
                var cy = height/2
                var startX = Math.round(startPointY)
                var stopX = Math.round(stopPointY)

                context.beginPath()
                context.fillStyle = parent.backColor
                context.moveTo(startX, cy - widthSlider/2);
                context.lineTo(stopX, cy - widthSlider/2);
                context.moveTo(startX, cy + widthSlider/2);
                context.lineTo(stopX, cy + widthSlider/2);
                context.stroke()

                context.fillStyle = parent.borderColor

                context.beginPath()
                context.moveTo(startX, cy - widthSlider/2);
                context.lineTo(startX, cy + widthSlider/2);
                context.lineTo(startX - heightSlider/2, cy + widthSlider/2);
                context.lineTo(startX - heightSlider + 2, cy);
                context.lineTo(startX - heightSlider/2, cy - widthSlider/2);
                context.closePath();
                context.fill();
                context.stroke()

                context.beginPath()
                context.moveTo(stopX, cy - widthSlider/2);
                context.lineTo(stopX, cy + widthSlider/2);
                context.lineTo(stopX + heightSlider/2, cy + widthSlider/2);
                context.lineTo(stopX + heightSlider - 2, cy);
                context.lineTo(stopX + heightSlider/2, cy - widthSlider/2);
                context.closePath();
                context.fill();
                context.stroke()

                context.beginPath()
                context.moveTo(0, cy);
                context.lineTo(startX - heightSlider + 2, cy);
                context.moveTo(width, cy);
                context.lineTo(stopX + heightSlider - 2, cy);
                context.stroke()
                return
            }

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

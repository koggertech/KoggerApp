import QtQuick 2.15

BackStyle {
    id: control

    Rectangle {
        id: backRect
        width: control.width
        height: control.active ? control.height - ticknessActive - 1 : control.height
        color: backColor
        opacity: backOpacity
    }

    Canvas {
        id: borderCanvas
        x: 0
        y: 0
        width: backRect.width
        height: backRect.height
        contextType: "2d"
        opacity: borderOpacity

        Connections {
            target: control
            onActiveChanged: borderCanvas.requestPaint()
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width, ticknessCorner);
            context.lineTo(width*0.7, ticknessCorner);
            context.lineTo(width*0.7 - ticknessCorner, ticknessBase);
            context.lineTo(0, ticknessBase);
            context.closePath();

            context.moveTo(width, height);
            context.lineTo(0, height);
            context.lineTo(0, height - ticknessCorner);
            context.lineTo(width*0.3, height - ticknessCorner);
            context.lineTo(width*0.3 + ticknessCorner, height - ticknessBase);
            context.lineTo(width, height - ticknessBase);
            context.closePath();

            context.fillStyle = borderColor
            context.fill();
        }
    }

    Rectangle {
        id: activeRect
        visible: control.active
        y: backRect.height
        width: control.width
        height: ticknessActive
        color: borderColor
        opacity: borderOpacity
    }

}

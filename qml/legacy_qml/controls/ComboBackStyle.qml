import QtQuick 2.15

Item {
    id: control

    StyleSet {
        id: styleSet
    }

    Rectangle {
        id: backRect
        radius: 1
        width: control.width
        height: control.height
        color: styleSet.colorControllBack
        opacity: styleSet.controllBackOpacity
        border.color: styleSet.colorControllBorder
        border.width: 1
    }

//    Canvas {
//        id: borderCanvas
//        x: 0
//        y: 0
//        width: backRect.width
//        height: backRect.height
//        contextType: "2d"
//        opacity: borderOpacity

//        Connections {
//            target: control
//            onActiveChanged: borderCanvas.requestPaint()
//        }

//        onPaint: {
//            context.reset();

//            context.moveTo(0, 0);
//            context.lineTo(width, 0);
//            context.lineTo(width, ticknessCorner);
//            context.lineTo(width*0.7, ticknessCorner);
//            context.lineTo(width*0.7 - ticknessCorner, ticknessBase);
//            context.lineTo(0, ticknessBase);
//            context.closePath();

////            context.moveTo(0, height);
////            context.lineTo(width, height - 0);
////            context.lineTo(width, height - ticknessCorner);
////            context.lineTo(width*0.7, height - ticknessCorner);
////            context.lineTo(width*0.7 - ticknessCorner, height - ticknessBase);
////            context.lineTo(0, height - ticknessBase);
////            context.closePath();

//            context.fillStyle = borderColor
//            context.fill();
//        }
//    }

//    Rectangle {
//        id: activeRect
//        visible: control.active
//        y: backRect.height
//        width: control.width
//        height: ticknessActive
//        color: borderColor
//        opacity: borderOpacity
//    }

}

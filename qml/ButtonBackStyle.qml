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
//        width: control.width
//        height: control.height
//        contextType: "2d"
//        opacity: borderOpacity

//        Connections {
//            target: control
//            onActiveChanged: borderCanvas.requestPaint()
//        }

//        onPaint: {
//            context.reset();

//            context.moveTo(0, 0);
//            context.lineTo(width*0.3, 0);
//            context.lineTo(width*0.3, ticknessBase);
//            context.lineTo(ticknessBase, ticknessBase);
//            context.lineTo(ticknessBase, height*0.3);
//            context.lineTo(0, height*0.3);
//            context.closePath();

//            context.moveTo(width, 0);
//            context.lineTo(width - width*0.3, 0);
//            context.lineTo(width - width*0.3, ticknessBase);
//            context.lineTo(width - ticknessBase, ticknessBase);
//            context.lineTo(width - ticknessBase, height*0.3);
//            context.lineTo(width - 0, height*0.3);
//            context.closePath();

//            context.moveTo(width, height);
//            context.lineTo(width - width*0.3, height);
//            context.lineTo(width - width*0.3, height - ticknessBase);
//            context.lineTo(width - ticknessBase, height - ticknessBase);
//            context.lineTo(width - ticknessBase, height - height*0.3);
//            context.lineTo(width, height - height*0.3);
//            context.closePath();

//            context.moveTo(0, height);
//            context.lineTo(width*0.3, height);
//            context.lineTo(width*0.3, height - ticknessBase);
//            context.lineTo(ticknessBase, height - ticknessBase);
//            context.lineTo(ticknessBase, height - height*0.3);
//            context.lineTo(0, height - height*0.3);
//            context.closePath();

////            context.moveTo(width, height);
////            context.lineTo(0, height);
////            context.lineTo(0, height - ticknessCorner);
////            context.lineTo(width*0.3, height - ticknessCorner);
////            context.lineTo(width*0.3 + ticknessCorner, height - ticknessBase);
////            context.lineTo(width, height - ticknessBase);
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

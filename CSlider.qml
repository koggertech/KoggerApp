import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Slider {
    id: slider
    from: 0
    value: 0
    to: 0
    horizontalPadding: 0
    snapMode: Slider.SnapAlways
    property int lineStyle: 0

    onEnabledChanged: {
        upCanvas.requestPaint()
    }

    onValueChanged: {
        upCanvas.requestPaint()
    }

    StyleSet {
        id: styleSet
    }


    handle: Canvas {
        id: upCanvas
        x: backHandleX
        y: backHandleY
        opacity: 1
        width: slider.horizontal ? 10 : 28
        height: slider.horizontal ? 28 : 10
        contextType: "2d"

        property real backHandleX: slider.horizontal ? slider.leftPadding + slider.visualPosition * (slider.availableWidth) - width/2 : slider.leftPadding
        property real backHandleY: slider.horizontal ? slider.topPadding + slider.availableHeight / 2 - height / 2 : -slider.topPadding / 2 + slider.visualPosition * (slider.height) - height / 2


        property bool pressed: false

        onPaint: {
            context.reset();

            var mid_height = slider.horizontal ? height/2 : height/2
            var width_button = slider.horizontal ? height/6 : width/2

            context.moveTo(0, mid_height);
            context.lineTo(width_button, 0);
            context.lineTo(width - width_button, 0);
            context.lineTo(width, mid_height);
            context.lineTo(width - width_button, height);
            context.lineTo(width_button, height);

            context.closePath();
            context.fillStyle = enabled ? (pressed ? "#909090" : styleSet.colorControllBackActive) : "#505050"
            context.fill();

            context.lineWidth = 1
            context.strokeStyle = enabled ? (pressed ? "#909090" : "#808080") : "#606060"
            context.stroke()
        }
    }

    background: Item {
        Rectangle {
            visible: lineStyle == 0 || lineStyle == 1
            x: slider.horizontal ? slider.leftPadding - upCanvas.width : slider.leftPadding + upCanvas.width/2
            y: slider.horizontal ? slider.topPadding + slider.availableHeight / 2 - height / 2 : slider.topPadding
            width: slider.horizontal ? upCanvas.x - x : 2
            height: slider.horizontal ? 2 : upCanvas.y
            color: "#808080"
        }

        Rectangle {
            visible: lineStyle == 0 || lineStyle == 2
            x: slider.horizontal ? upCanvas.x : slider.leftPadding + upCanvas.width/2
            y: slider.horizontal ? slider.topPadding + slider.availableHeight / 2 - height / 2 : upCanvas.y
            width: slider.horizontal ? slider.width - slider.leftPadding - x + upCanvas.width : 2
            height: slider.horizontal ? 2 : slider.height - slider.bottomPadding - y
            color: "#808080"
        }
    }
}

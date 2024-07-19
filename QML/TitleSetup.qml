import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: control
    Layout.fillWidth: true
    height: 32
    property string textTitle: qsTr("TITLE")


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
            context.lineTo(text.x + text.width, 2);
            context.lineTo(text.x + text.width, height);
            context.lineTo(text.x, height);
            context.lineTo(text.x, 2);
            context.lineTo(0, 2);
            context.closePath();

            context.fillStyle = "#70C0F0"
            context.fill();
        }
    }

    Text {
        id: text
        x: control.width/2 - width/2
        text: textTitle
        padding: 6
        font.pixelSize: 18
        color: "black"
    }

}

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

ComboBox {
    id: control

    font.pointSize: 12

    StyleSet {
        id:styleSet
    }

    delegate: ItemDelegate {
        id: itemDelegate
        width: control.width
        implicitHeight: 30
        contentItem: Text {
            text: modelData
            antialiasing: false
            color: styleSet.colorControllText

            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle{
            color: itemDelegate.highlighted ? styleSet.colorControllBackActive : "transparent"
            border.color: styleSet.colorControllBorder
            border.width: itemDelegate.highlighted ? 1 : 0
        }

        highlighted: control.highlightedIndex === index
    }

    onPressedChanged: canvas.requestPaint()

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

//        Connections {
//            target: control
//            onPressedChanged: canvas.requestPaint()
//        }

//        Connections {
//            target: control.popup
//            onVisibleChanged: canvas.requestPaint()
//        }

        onPaint: {
            context.reset();

            if(control.popup.visible) {
                context.moveTo(width / 2, 0);
                context.lineTo(width, height);
                context.lineTo(0, height);
                context.closePath();
            } else {
                context.moveTo(0, 0);
                context.lineTo(width, 0);
                context.lineTo(width / 2, height);
                context.closePath();
            }

            context.fillStyle = control.down ? styleSet.colorControllTextActive : styleSet.colorControllText
            context.fill();
        }
    }

    contentItem: Text {
        antialiasing: false
        leftPadding: 15
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: control.down ? styleSet.colorControllTextActive : styleSet.colorControllText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background:  Rectangle {
        id: backRect
        implicitWidth: 100
        implicitHeight: styleSet.controllHeight
        radius: 1
        color: control.down ? styleSet.colorControllBackActive : styleSet.colorControllBack
        opacity: styleSet.controllBackOpacity
        border.color: control.down ? styleSet.colorControllBorderActive : styleSet.colorControllBorder
        border.width: 1
    }

    popup: Popup {
        y: control.height + 2
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        onVisibleChanged: canvas.requestPaint()

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            highlightFollowsCurrentItem: false
            focus: true
            ScrollIndicator.vertical: ScrollIndicator { }
        }



        background: Rectangle {
            id: popupRect
            implicitWidth: 100
            implicitHeight: styleSet.controllHeight
            radius: 1
            color: control.down ? styleSet.colorControllBack : styleSet.colorControllBack
            opacity: styleSet.controllBackOpacity
            border.color: control.down ? styleSet.colorControllBorderActive : styleSet.colorControllBorder
            border.width: 1
        }
    }
}

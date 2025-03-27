import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ComboBox {
    id: control

    delegate: ItemDelegate {
        id: itemDelegate
        width: control.width
        implicitHeight: theme.controlHeight
        contentItem: CText {
            text: modelData
            small: false
//            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            color: itemDelegate.highlighted ? theme.textSolidColor :  theme.textColor
        }

        background: Rectangle{
            color: itemDelegate.highlighted ? theme.controlSolidBackColor :  theme.controlBackColor
            border.color: itemDelegate.highlighted ? theme.controlSolidBorderColor : theme.controlBorderColor
            border.width: itemDelegate.highlighted ? 1 : 0
        }

        highlighted: control.highlightedIndex === index
    }

    onPressedChanged: canvas.requestPaint()

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: theme.controlHeight/2
        height: theme.controlHeight/3
        contextType: "2d"

        Connections {
            target: theme

            function onThemeIDChanged() {
                canvas.requestPaint()
            }
        }

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

            context.fillStyle = control.down ? theme.textColor : theme.textColor
            context.fill();
        }
    }

    contentItem: CText {
        id: contentText
        leftPadding: 15
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        verticalAlignment: Text.AlignVCenter
        small: false

//        elide: Text.ElideRight

        color: contentText.highlighted ? theme.textSolidColor :  theme.textColor

    }

    background:  Rectangle {
        id: backRect
        implicitWidth: 100
        implicitHeight: theme.controlHeight
        radius: 1
        color: contentText.highlighted ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: contentText.highlighted ? theme.controlSolidBorderColor : theme.controlBorderColor
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
            implicitHeight: theme.controlHeight
            radius: 1
            color: theme.controlBackColor
            border.color: theme.controlBorderColor
            border.width: 1
        }
    }
}

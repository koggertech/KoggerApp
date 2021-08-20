import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

ComboBox {
    id: control

    delegate: ItemDelegate {
        id: itemDelegate
        width: control.width
        implicitHeight: 26
        contentItem: CText {
            text: modelData
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle{
            color: itemDelegate.highlighted ? theme.controlSolidBackColor : "transparent"
            border.color: theme.controlBorderColor
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
        antialiasing: false
        leftPadding: 15
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background:  Rectangle {
        id: backRect
        implicitWidth: 100
        implicitHeight: 26
        radius: 1
        color: control.down ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: control.down ? theme.controlSolidBorderColor : theme.controlBorderColor
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
            implicitHeight: 26
            radius: 1
            color: theme.controlBackColor
            border.color: theme.controlBorderColor
            border.width: 1
        }
    }
}

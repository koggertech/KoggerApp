import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: control
    text: "Ok"
    highlighted: true
    implicitHeight: 26

    StyleSet {
        id: styleSet
    }

    contentItem: Text {
        width: control.width
        text: control.text
        font.pointSize: 12
        color: control.down ? styleSet.colorControllTextActive : styleSet.colorControllText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        id: backRect
        implicitWidth: 35
        implicitHeight: styleSet.controllHeight
        radius: 1
        color: control.down ? styleSet.colorControllBackActive : styleSet.colorControllBack
        opacity: styleSet.controllBackOpacity
        border.color: control.down ? styleSet.colorControllBorderActive : styleSet.colorControllBorder
        border.width: styleSet.controlBorderSize
    }
}

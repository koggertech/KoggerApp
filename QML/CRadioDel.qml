import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RadioDelegate {
    id: radioDel
    Layout.fillWidth: true
    implicitHeight: theme.controlHeight
    implicitWidth: 200

    contentItem: Text {
        rightPadding: radioDel.indicator.width + radioDel.spacing
        text: radioDel.text
        font: theme.textFont
        color: theme.textColor
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Rectangle {
        implicitWidth: 22
        implicitHeight: 22
        radius: 11
        x: radioDel.width - width - radioDel.rightPadding
        y: radioDel.height / 2 - height / 2
        color: "transparent"
        border.color: theme.textColor

        Rectangle {
            width: 14
            height: 14
            x: 4
            y: 4
            radius: 7
            color: theme.textColor
            visible: radioDel.checked
        }
    }

//    background: Rectangle {
//        color: theme.menuBackColor
////        border.color: theme.controlBorderColor
//    }
}

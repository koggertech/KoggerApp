import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

RadioButton {
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
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        x: radioDel.width - width - radioDel.rightPadding
        y: radioDel.height / 2 - height / 2
        color: "transparent"
        border.color: theme.textColor

        Rectangle {
            width: 10
            height: 10
            x: 4
            y: 4
            radius: 7
            color: theme.textColor
            visible: radioDel.checked
        }
    }
 }

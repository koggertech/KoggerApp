import QtQuick 2.12
import QtQuick.Controls 2.12

CheckBox {
     id: control
     text: "Check"

     indicator: Rectangle {
         implicitWidth: 19
         implicitHeight: 19
         radius: 1
         x: control.leftPadding
         y: parent.height / 2 - height / 2
         color: theme.controlBackColor
         border.color: control.checked ? "#F0F0F0" : "#C0C0C0"
         border.width: 1

         Rectangle {
             width: 11
             height: 11
             x: 4
             y: 4
             radius: 1
             color: "#D0D0D0"
             visible: control.checked
         }
     }

     contentItem: CText {
         text: control.text
         verticalAlignment: Text.AlignVCenter
         leftPadding: control.indicator.width + control.spacing
     }
 }

import QtQuick 2.12
import QtQuick.Controls 2.12

RadioButton {
     id: control
     text: "RButton"

     StyleSet {
         id: styleSet
     }

     indicator: Rectangle {
         implicitWidth: 18
         implicitHeight: 18
         radius: 9
         x: control.leftPadding
         y: parent.height / 2 - height / 2
         color: styleSet.colorControllBack
         border.color: styleSet.colorControllBorder

         Rectangle {
             width: 12
             height: 12
             radius: 6
             x: 3
             y: 3
             color: "#7090b0"
             visible: control.checked
         }
     }

     contentItem: Text {
         text: control.text
         font: control.font
         opacity: 0.9
         color: styleSet.colorControllText
         verticalAlignment: Text.AlignVCenter
         leftPadding: control.indicator.width + control.spacing
     }
 }

import QtQuick 2.12
import QtQuick.Controls 2.12

RadioButton {
     id: control
     text: "RButton"

     indicator: Rectangle {
         implicitWidth: 20
         implicitHeight: 20
         x: control.leftPadding
         y: parent.height / 2 - height / 2
         border.color: "#7090b0"

         Rectangle {
             width: 14
             height: 14
             x: 3
             y: 3
             color: "#7090b0"
             visible: control.checked
         }
     }

     contentItem: Text {
         text: control.text
         font: control.font
         opacity: enabled ? 1.0 : 0.3
         color: "#7090b0"
         verticalAlignment: Text.AlignVCenter
         leftPadding: control.indicator.width + control.spacing
     }
 }

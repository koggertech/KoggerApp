import QtQuick 2.12
import QtQuick.Controls 2.12

CheckBox {
     id: control
     text: qsTr("Check")
     implicitHeight: theme.controlHeight

     indicator: Rectangle {
         id: backRect
         implicitHeight: control.implicitHeight*0.7
         implicitWidth: control.implicitHeight*0.7
         radius: 1
         x: control.leftPadding
         y: control.height / 2 - height / 2
         color: theme.controlBackColor
         border.color: theme.textColor
         border.width: 1

         Rectangle {
             width: backRect.width/2
             height: backRect.height/2
             x: backRect.width/4
             y: backRect.height/4
             radius: 1
             color: theme.textColor
             visible: control.checked
         }
     }

     contentItem: KText {
         text: control.text
         verticalAlignment: Text.AlignVCenter
         leftPadding: control.indicator.width + control.spacing
     }
 }

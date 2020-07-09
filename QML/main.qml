import QtQuick 2.6
import QtQuick.Window 2.12

import QtQuick.Layouts 1.12

import QtQuick.Controls 2.12

import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


import WaterFall 1.0

Window  {
    id: mainview
    visible: true
    width: 1024
    minimumWidth: 512
    height: 512
    minimumHeight: 256
    color: "black"
    title: qsTr("KoggerApp, KOGGER")

    Settings {
        property alias x: mainview.x
        property alias y: mainview.y
        property alias width: mainview.width
        property alias height: mainview.height
    }


    WaterFall {
        id: waterView
        visible: true
        anchors.top: parent.top
        width: mainview.width
        height: mainview.height

    }

//    TextArea {
//        width: mainview.width
//        anchors.bottom: parent.bottom
//        height: 500
//        text: "text" // core.consoleTextOut
//        textFormat: TextEdit.RichText
//        font.family: "Helvetica"
//        font.pointSize: 10
//        color: "blue"
//        focus: false
//        readOnly: true
//        selectByMouse: true

//        onPressAndHold: {
//            core.consoleTextUpdate = true
//        }

//        onPressed: {
//            core.consoleTextUpdate = false
//        }

//        onReleased: {
//            core.consoleTextUpdate = true
//        }

//    }


//    Console {
//        visible: true
//    }

    MenuBar {
        x: 0
        y: 5
        heightMenuViewer: mainview.height
    }



}

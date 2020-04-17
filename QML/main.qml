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
    title: qsTr("Sonar Viewer, KOGGER")

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

    MenuBar {
        x: 5
        y: 5
    }

    Console {
        visible: false
    }

}

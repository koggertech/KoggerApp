import QtQuick 2.6
import QtQuick.Window 2.12

import QtQuick.Layouts 1.12

import QtQuick.Controls 2.12

import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.15

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

    SplitView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.fill: parent

        orientation: Qt.Vertical
        visible: true

        handle: Rectangle {
            implicitWidth: 5
            implicitHeight: 5
            color: SplitHandle.pressed ? "#A0A0A0" : "#707070"

            Rectangle {
                width: parent.width
                height: 1
                color: "#A0A0A0"
            }

            Rectangle {
                y: parent.height
                width: parent.width
                height: 1
                color: "#A0A0A0"
            }
        }

        ColumnLayout {
            SplitView.fillHeight: true
            SplitView.fillWidth: true
            spacing: 0


            WaterFall {
                id: waterView
                visible: true
                width: mainview.width
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#707070"
            }

            CSlider {
                id: historyScroll
                width: mainview.width
                height: 30
                implicitHeight: 30
                horizontalPadding: 15
                lineStyle: 3
                opacity: 1

                Layout.fillWidth: true

                stepSize: 0.0001
                from: 1
                to: 0

                onValueChanged: core.setTimelinePosition(value);
            }
        }

        Console {
            id: console_vis
            visible: menuBar.isConsoleVisible
            SplitView.minimumHeight: 100
        }
    }

    MenuBar {
        id: menuBar
        Layout.fillHeight: true
        height: waterView.height
    }
}

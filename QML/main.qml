import QtQuick 2.12
import SceneGraphRendering 1.0
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

//    Settings {
//        property alias x: mainview.x
//        property alias y: mainview.y
//        property alias width: mainview.width
//        property alias height: mainview.height
//    }

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
            id: visualisationLayout
            SplitView.fillHeight: true
            SplitView.fillWidth: true
            spacing: 0

            Renderer {
                id: renderer
                visible: Scene3DModel.sceneVisibility()
                width: mainview.width
                Layout.fillHeight: true
                Layout.fillWidth: true
                focus: true

                onVisibleChanged: {
                    if(visible) { core.movePoints() }
                }


                Rectangle{

                    id: surfaceCalculatingRectangle
                    visible: Scene3DModel.triangulationAvailable()
                    width: 300
                    height: 100
                    color: "transparent"


                    ColumnLayout{

                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        width: 100
                        height: 50
                        spacing: 10


                        Text{
                            text: "Calculating surface.\nPlease wait..."
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ProgressBar{
                            id: surfaceProcessingProgressBar
                            value: 0.0
                            indeterminate: true
                            Layout.fillWidth: true
                        }

                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                PinchArea {
                    id: pinch3D
                    anchors.fill: parent

                    enabled: true
                    onPinchUpdated: {
                        renderer.scaleDelta((pinch.previousScale - pinch.scale)*500.0)
                        renderer.mouse(pinch.center.x, pinch.center.y, false)
                    }

                    onPinchStarted: {
                        mousearea3D.enabled = false
                        renderer.mouse(-1, -1, false)
                    }

                    onPinchFinished: {
                        mousearea3D.enabled = true
                        renderer.mouse(-1, -1, false)
                    }

                    MouseArea {
                        id: mousearea3D
                        enabled: true
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

                        onWheel: {
                            renderer.scaleDelta(wheel.angleDelta.y)
                        }

                        onPositionChanged: {
                            if((mousearea3D.pressedButtons & Qt.MiddleButton) || ((mousearea3D.pressedButtons & Qt.LeftButton) && (mouse.modifiers & Qt.ControlModifier))) {
                                renderer.mouse(mouse.x, mouse.y, false)
                            } else  if(mousearea3D.pressedButtons & Qt.LeftButton) {
                                renderer.mouse(mouse.x, mouse.y, true)

                            }
                        }

                        onPressed: {
                            renderer.mouse(mouse.x, mouse.y, false)
                        }

                        onReleased: {
                            renderer.mouse(-1, -1, false)
                        }

                    }
                }
            }

            WaterFall {
                id: waterView
                visible: menuBar.is2DVisible
                width: mainview.width
                Layout.fillHeight: true
                Layout.fillWidth: true
                focus: true

                horizontal: menuBar.is2DHorizontal

                PinchArea {
                    id: pinch2D
                    anchors.fill: parent

                    enabled: true
                    onPinchUpdated: {
                        waterView.verZoomEvent((pinch.previousScale - pinch.scale)*500.0)
                        waterView.horScrollEvent(-(pinch.previousCenter.x - pinch.center.x))
                        waterView.verScrollEvent(pinch.previousCenter.y - pinch.center.y)
                    }

                    onPinchStarted: {
                        mousearea.enabled = false
                        waterView.setMouse(-1, -1)
                    }

                    onPinchFinished: {
                        mousearea.enabled = true
                        waterView.setMouse(-1, -1)
                    }

                    MouseArea {
                        id: mousearea

                        enabled: true
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onWheel: {
                            if (wheel.modifiers & Qt.ControlModifier) {
                                waterView.verZoomEvent(-wheel.angleDelta.y)
                            } else if (wheel.modifiers & Qt.ShiftModifier) {
                                waterView.verScrollEvent(-wheel.angleDelta.y)
                            } else {
                                waterView.horScrollEvent(wheel.angleDelta.y)
                            }
                        }

                        onClicked: {
                            waterView.focus = true
                            if (mouse.button === Qt.RightButton) {
                            }
                        }

                        onReleased: {
                            if (mouse.button === Qt.LeftButton) {
                                waterView.setMouse(-1, -1)
                            }
                        }

                        onPressed: {
                            if (mouse.button === Qt.LeftButton) {
                                waterView.setMouse(mouse.x, mouse.y)
                            }
                        }

                        onPositionChanged: {
                            if(mousearea.pressedButtons & Qt.LeftButton) {
                                waterView.setMouse(mouse.x, mouse.y)
                            }
                        }

                    }

                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.controlBorderColor
            }

            CSlider {
                id: historyScroll
                Layout.fillWidth: true
                width: mainview.width
                implicitHeight: theme.controlHeight

                stepSize: 0.0001
                from: 1
                to: 0

                onValueChanged: core.setTimelinePosition(value);
            }
        }

        Console {
            id: console_vis
            visible: theme.consoleVisible
            SplitView.minimumHeight: 100
        }
    }

    MenuBar {
        id: menuBar
        Layout.fillHeight: true
        height: visualisationLayout.height
        //settingsWidth: theme.controlHeight*20 < 800 ? theme.controlHeight*20 : 800
    }

    Connections {
        target: Scene3DModel
        onStateChanged: {
            renderer.visible = Scene3DModel.sceneVisibility()
            surfaceCalculatingRectangle.visible = !Scene3DModel.triangulationAvailable()
        }
    }

}

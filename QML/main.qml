import QtQuick 2.12
import SceneGraphRendering 1.0
import QtQuick.Window 2.12

import QtQuick.Layouts 1.12

import QtQuick.Controls 2.12

import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.15

import WaterFall 1.0

import KoggerCommon 1.0

Window  {
    id:            mainview
    visible:       true
    width:         1024
    minimumWidth:  512
    height:        512
    minimumHeight: 256
    color:         "black"
    title:         qsTr("KoggerApp, KOGGER")

    readonly property int _rightBarWidth:                360
    readonly property int _activeObjectParamsMenuHeight: 500
    readonly property int _sceneObjectsListHeight:       300

    //    Settings {
    //        property alias x: mainview.x
    //        property alias y: mainview.y
    //        property alias width: mainview.width
    //        property alias height: mainview.height
    //    }

    SplitView {
        Layout.fillHeight: true
        Layout.fillWidth:  true
        anchors.fill:      parent
        orientation:       Qt.Vertical
        visible:           true

        handle: Rectangle {
            implicitWidth:  5
            implicitHeight: 5
            color:          SplitHandle.pressed ? "#A0A0A0" : "#707070"

            Rectangle {
                width:  parent.width
                height: 1
                color:  "#A0A0A0"
            }

            Rectangle {
                y:      parent.height
                width:  parent.width
                height: 1
                color:  "#A0A0A0"
            }
        }

        ColumnLayout {
            id:                   visualisationLayout
            SplitView.fillHeight: true
            SplitView.fillWidth:  true
            spacing:              0

            GraphicsScene3dView {
                id:                renderer
                objectName: "GraphicsScene3dView"
                width:             mainview.width
                Layout.fillHeight: true
                Layout.fillWidth:  true
                focus:             true

                KWaitProgressBar{
                    id:        surfaceProcessingProgressBar
                    objectName: "surfaceProcessingProgressBar"
                    text:      "Calculating surface.\nPlease wait..."
                    textColor: "black"
                    visible:   false
                }

                PinchArea {
                    id:           pinch3D
                    anchors.fill: parent
                    enabled:      true

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
                        enabled:              true
                        anchors.fill:         parent
                        acceptedButtons:      Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        focus:                true
                        hoverEnabled:         true
                        Keys.enabled:         true
                        Keys.onDeletePressed: renderer.keyPressTrigger(event.key)

                        onWheel: {
                            renderer.mouseWheelTrigger(wheel.buttons,
                                                       wheel.x,
                                                       wheel.y,
                                                       wheel.angleDelta)
                        }

                        onPositionChanged: {
                            renderer.mouseMoveTrigger(mouse.buttons, mouse.x, mouse.y)
                        }

                        onPressed: {
                            renderer.mousePressTrigger(mouse.buttons, mouse.x, mouse.y)
                        }

                        onReleased: {
                            renderer.mouseReleaseTrigger(mouse.buttons, mouse.x, mouse.y)
                        }

                    }
                }
            }

            WaterFall {
                id:                waterView
                visible:           menuBar.is2DVisible
                width:             mainview.width
                Layout.fillHeight: true
                Layout.fillWidth:  true
                focus:             true
                horizontal:        menuBar.is2DHorizontal

                PinchArea {
                    id:           pinch2D
                    anchors.fill: parent
                    enabled:      true

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
                        id:              mousearea
                        enabled:         true
                        anchors.fill:    parent
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
                visible:          menuBar.is2DVisible
                Layout.fillWidth: true
                height:           1
                color:            theme.controlBorderColor
            }

            CSlider {
                visible:          menuBar.is2DVisible
                id:               historyScroll
                Layout.fillWidth: true
                width:            mainview.width
                implicitHeight:   theme.controlHeight

                stepSize:       0.0001
                from:           1
                to:             0
                onValueChanged: core.setTimelinePosition(value);
            }
        }

        Console {
            id:                      console_vis
            visible:                 theme.consoleVisible
            SplitView.minimumHeight: 100
        }
    }

    MenuBar {
        id:                menuBar
        objectName:        "menuBar"
        Layout.fillHeight: true
        height:            visualisationLayout.height
        Keys.forwardTo:    [mousearea3D]
    }

    Scene3DToolbar{
        //visible: menuBar.is3DVisible
        id:                       scene3DToolbar
        anchors.top:              parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.rightMargin:      20
        Keys.forwardTo:           [mousearea3D]
    }

    Connections{
        target: SurfaceControlMenuController
        onSurfaceProcessorTaskStarted: surfaceProcessingProgressBar.visible = true
        onSurfaceProcessorTaskFinished: surfaceProcessingProgressBar.visible = false
    }
}

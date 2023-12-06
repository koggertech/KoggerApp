import QtQuick 2.15
import SceneGraphRendering 1.0
import QtQuick.Window 2.15

import QtQuick.Layouts 1.15

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


            GridLayout {
                visible: menuBar.is2DVisible
                rows    : 10
                columns : 10
                Plot2D {
                    id: waterView

                    width: mainview.width
                    Layout.fillHeight: true
//                    Layout.fillWidth: true
                    Layout.preferredWidth: mainview.width
                    Layout.rowSpan   : 10
                    Layout.columnSpan: 10
                    focus: true
                    horizontal: menuBar.is2DHorizontal
                }

//                Plot2D {
//                    id: waterView2
//                    visible: true
//                    width: mainview.width/2
//                    Layout.fillHeight: true
////                    Layout.fillWidth: true
//                    Layout.preferredWidth: mainview.width/2
//                    Layout.rowSpan   : 5
//                    Layout.columnSpan: 5
//                    focus: true

//                    horizontal: menuBar.is2DHorizontal
//                }

//                Plot2D {
//                    id: waterView3
//                    visible: true
//                    width: mainview.width
//                    Layout.fillHeight: true
////                    Layout.fillWidth: true
//                    Layout.preferredWidth: mainview.width
//                    Layout.rowSpan   : 5
//                    Layout.columnSpan: 10
//                    focus: true

//                    horizontal: menuBar.is2DHorizontal
//                }
            }



//            Plot2D {
//                id: waterView2
//                visible: menuBar.is2DVisible
//                width: mainview.width
//                Layout.fillHeight: true
//                Layout.fillWidth: true
//                focus: true

//                horizontal: menuBar.is2DHorizontal
//            }

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
                width: mainview.width
                implicitHeight: theme.controlHeight
                value: waterView.timelinePosition
                stepSize: 0.0001
                from: 0
                to: 1
                onValueChanged: core.setTimelinePosition(value);
            }
        }

        Console {
            id:                      console_vis
            visible:                 theme.consoleVisible
            SplitView.minimumHeight: 100
        }
    }




    ColumnLayout {
        anchors.top: parent
        anchors.horizontalCenter: parent.horizontalCenter

        RowLayout {
            MenuBlock {
            }
            CCombo  {
                id: pilotArmedState
                Layout.margins: 4
                visible: devs.pilotArmState >= 0
                Layout.fillWidth: true
                model: ["Disarmed", "Armed"]
                currentIndex: devs.pilotArmState

                onCurrentIndexChanged: {
                    if(currentIndex != devs.pilotArmState) {
                        currentIndex = devs.pilotArmState
                    }
                }
            }

            CCombo  {
                id: pilotModeState
                Layout.margins: 4
                visible: devs.pilotModeState >= 0
                Layout.fillWidth: true
                model: [
                    "Manual",
                    "Acro",
                    "Steering",
                    "Hold",
                    "Loiter",
                    "Follow",
                    "Simple",
                    "Dock",
                    "Circle",
                    "Auto",
                    "RTL",
                    "SmartRTL",
                    "Guided",
                    "Mode16",
                    "Mode17"
                ]
                currentIndex: devs.pilotModeState

                onCurrentIndexChanged: {
                    if(currentIndex != devs.pilotModeState) {
                        currentIndex = devs.pilotModeState
                    }
                }
            }
        }

        RowLayout {
            MenuBlock {

            }
            CText {
                id: fcTextBatt
                Layout.margins: 4
                visible: isFinite(devs.vruVoltage)
                rightPadding: 20
                leftPadding: 20
                text: devs.vruVoltage.toFixed(1) + " V   " + devs.vruCurrent.toFixed(1) + " A   " + devs.vruVelocityH.toFixed(2) + " m/s"
            }
        }



//        CText {
//            id: fcTextMode
//            rightPadding: 20
//            leftPadding: 20
//            color: devs.pilotArmed ? theme.textColor : theme.textErrorColor
//            text: devs.pilotArmed ? "Armed" : "Disarmed"
//        }


    }


    MenuBar {
        id:                menuBar
        objectName:        "menuBar"
        Layout.fillHeight: true
        Keys.forwardTo:    [mousearea3D]
        height: visualisationLayout.height
        targetPlot: waterView

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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import WaterFall 1.0

WaterFall {
    id: plot

    PinchArea {
        id: pinch2D
        anchors.fill: parent
        enabled: true

        property int thresholdXAxis: 15
        property int thresholdYAxis: 15
        property double zoomThreshold: 0.1

        property bool movementX: false
        property bool movementY: false
        property bool zoomY: false
        property point pinchStartPos: Qt.point(-1, -1)

        function clearPinchMovementState() {
            movementX = false
            movementY = false
            zoomY = false
        }

        onPinchStarted: {
            menuBlock.visible = false

            mousearea.enabled = false
            plot.plotMousePosition(-1, -1)

            clearPinchMovementState()
            pinchStartPos = Qt.point(pinch.center.x, pinch.center.y)
        }

        onPinchUpdated: {
            console.info("onPinchUpdated")

            if (movementX) {
                plot.horScrollEvent(-(pinch.previousCenter.x - pinch.center.x))
            }
            else if (movementY) {
                plot.verScrollEvent(pinch.previousCenter.y - pinch.center.y)
            }
            else if (zoomY) {
                plot.verZoomEvent((pinch.previousScale - pinch.scale)*500.0)
            }
            else {
                if (Math.abs(pinchStartPos.x - pinch.center.x) > thresholdXAxis) {
                    movementX = true
                }
                else if (Math.abs(pinchStartPos.y - pinch.center.y) > thresholdYAxis) {
                    movementY = true
                }
                else if (pinch.scale > (1.0 + zoomThreshold) || pinch.scale < (1.0 - zoomThreshold)) {
                    zoomY = true
                }
            }
        }       

        onPinchFinished: {
            mousearea.enabled = true
            plot.plotMousePosition(-1, -1)

            clearPinchMovementState()
            pinchStartPos = Qt.point(-1, -1)
        }

        MouseArea {
            id: mousearea
            enabled: true
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            property int lastMouseX: -1

            onClicked: {
                lastMouseX = mouse.x

                plot.focus = true

                if (mouse.button === Qt.RightButton && theme.instrumentsGrade !== 0) {
                    menuBlock.position(mouse.x, mouse.y)
                }
            }

            onPressed: {
                lastMouseX = mouse.x

                if (mouse.button === Qt.LeftButton) {
                    menuBlock.visible = false
                }

                if (mouse.button === Qt.LeftButton) {
                    plot.plotMousePosition(mouse.x, mouse.y)
                }
            }

            onReleased: {
                lastMouseX = -1

                if (mouse.button === Qt.LeftButton) {
                    plot.plotMousePosition(-1, -1)
                }

                if (Qt.platform.os === "android" && theme.instrumentsGrade !== 0) {
                    menuBlock.position(mouse.x, mouse.y)
                }
            }

            onCanceled: {
                lastMouseX = -1
            }

            onPositionChanged: {
                var delta = mouse.x - lastMouseX
                lastMouseX = mouse.x

                if (mousearea.pressedButtons & Qt.LeftButton) {
                    plot.plotMousePosition(mouse.x, mouse.y)

                    if (theme.instrumentsGrade === 0) {
                        plot.horScrollEvent(delta)
                    }
                }
            }

            onWheel: {
                if (wheel.modifiers & Qt.ControlModifier) {
                    plot.verZoomEvent(-wheel.angleDelta.y)
                }
                else if (wheel.modifiers & Qt.ShiftModifier) {
                    plot.verScrollEvent(-wheel.angleDelta.y)
                }
                else {
                    plot.horScrollEvent(wheel.angleDelta.y)
                }
            }
        }
    }

    onHeightChanged: {
        if(menuBlock.visible) {
            menuBlock.position(menuBlock.x, menuBlock.y)
        }
    }

    onWidthChanged: {
        if(menuBlock.visible) {
            menuBlock.position(menuBlock.x, menuBlock.y)
        }
    }

    MenuFrame {
        Layout.alignment: Qt.AlignHCenter
        //visible: visible2dButton.checked
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 4
        margins: 0

        // isDraggable: true
        isOpacityControlled: true

        ColumnLayout {

            CText {
                Layout.fillWidth: true
                Layout.topMargin: 0
                Layout.preferredWidth: theme.controlHeight*1.2
                // visible: chartEnable.checked // TODO
                horizontalAlignment: Text.AlignHCenter
                text: echogramLevelsSlider.stopValue
                small: true
            }

            ChartLevel {
                // opacity: 0.8
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight*1.2
                id: echogramLevelsSlider
                // visible: chartEnable.checked // TODO
                Layout.alignment: Qt.AlignHCenter

                onStartValueChanged: {
                    plot.plotEchogramSetLevels(startValue, stopValue);
                }

                onStopValueChanged: {
                    plot.plotEchogramSetLevels(startValue, stopValue);
                }

                Component.onCompleted: {
                    plot.plotEchogramSetLevels(startValue, stopValue);
                }

                Settings {
                    property alias echogramLevelsStart: echogramLevelsSlider.startValue
                    property alias echogramLevelsStop: echogramLevelsSlider.stopValue
                }
            }

            CText {
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight*1.2
                Layout.bottomMargin: 0
                // visible: chartEnable.checked // TODO
                horizontalAlignment: Text.AlignHCenter

                text: echogramLevelsSlider.startValue
                small: true
            }
        }
    }

    RowLayout {
        id: menuBlock
        Layout.alignment: Qt.AlignHCenter
        spacing: 1
        visible: false
        Layout.margins: 0

        function position(mx, my) {
            var oy = plot.height - (my + implicitHeight)
            if(oy < 0) {
                my = my + oy
            }

            if(my < 0) {
                my = 0
            }

            var ox = plot.width - (mx - implicitWidth)
            if(ox < 0) {
                mx = mx + ox
            }

            x = mx
            y = my
            visible = true
//            backgrn.focus = true
        }

        ButtonGroup { id: pencilbuttonGroup }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/direction-arrows.svg"
            checked: true
            backColor: theme.controlBackColor

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(1)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/arrow-bar-to-down.svg"
            backColor: theme.controlBackColor

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(2)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/pencil.svg"
            backColor: theme.controlBackColor

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(3)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/arrow-bar-to-up.svg"
            backColor: theme.controlBackColor

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(4)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/eraser.svg"
            backColor: theme.controlBackColor

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(5)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            Layout.fillWidth: true
            icon.source: "./icons/x.svg"
            backColor: theme.controlBackColor
            checkable: false

            onClicked: {
                menuBlock.visible = false
            }

            ButtonGroup.group: pencilbuttonGroup
        }
    }
}

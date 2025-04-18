import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import WaterFall 1.0

WaterFall {
    id: plot

    function setLevels(low, high) {
        echogramLevelsSlider.startValue = low
        echogramLevelsSlider.stopValue = high
        echogramLevelsSlider.startPointY = echogramLevelsSlider.valueToPosition(low);
        echogramLevelsSlider.stopPointY = echogramLevelsSlider.valueToPosition(high);
        echogramLevelsSlider.update()
    }

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
            property bool wasMoved: false
            property point startMousePos: Qt.point(-1, -1)
            property real mouseThreshold: 15
            property int contactMouseX: -1
            property int contactMouseY: -1

            hoverEnabled: true

            Timer {
                id: longPressTimer
                interval: 500
                repeat: false
                onTriggered: {
                    if (Qt.platform.os === "android" && theme.instrumentsGrade !== 0 && !mousearea.wasMoved) {
                        plot.onCursorMoved(mousearea.mouseX, mousearea.mouseY)
                        mousearea.contactMouseX = mousearea.mouseX
                        mousearea.contactMouseY = mousearea.mouseY
                        plot.simplePlotMousePosition(mousearea.mouseX, mousearea.mouseY)

                        menuBlock.position(mousearea.mouseX, mousearea.mouseY)
                    }
                }
            }

            onClicked: {
                lastMouseX = mouse.x
                plot.focus = true

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)

                    if (theme.instrumentsGrade !== 0) {
                        menuBlock.position(mouse.x, mouse.y)
                    }
                }

                wasMoved = false
            }

            onPressed: {
                lastMouseX = mouse.x

                if (Qt.platform.os === "android") {
                    startMousePos = Qt.point(mouse.x, mouse.y)
                    longPressTimer.start()
                }

                if (mouse.button === Qt.LeftButton) {
                    menuBlock.visible = false
                    plot.plotMousePosition(mouse.x, mouse.y)
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
                }

                wasMoved = false
            }

            onReleased: {
                lastMouseX = -1

                if (Qt.platform.os === "android") {
                    longPressTimer.stop()
                }

                if (mouse.button === Qt.LeftButton) {
                    plot.plotMousePosition(-1, -1)
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
                }

                wasMoved = false
                startMousePos = Qt.point(-1, -1)
            }

            onCanceled: {
                lastMouseX = -1

                if (Qt.platform.os === "android") {
                    longPressTimer.stop()
                }

                wasMoved = false
                startMousePos = Qt.point(-1, -1)
            }

            onPositionChanged: {
                plot.onCursorMoved(mouse.x, mouse.y)

                if (Qt.platform.os === "android") {
                    if (!wasMoved) {
                        var currDelta = Math.sqrt(Math.pow((mouse.x - startMousePos.x), 2) + Math.pow((mouse.y - startMousePos.y), 2));
                        if (currDelta > mouseThreshold) {
                            wasMoved = true;
                        }
                    }
                }

                var delta = mouse.x - lastMouseX
                lastMouseX = mouse.x

                if (mousearea.pressedButtons & Qt.LeftButton) {
                    plot.plotMousePosition(mouse.x, mouse.y)

                    if (theme.instrumentsGrade === 0) {
                        plot.horScrollEvent(delta)
                    }
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
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

    CContact {
        id: contactDialog

        onVisibleChanged: {
            if (!visible) {
                parent.focus = true

                if (accepted) {
                    plot.setContact(contactDialog.indx, contactDialog.inputFieldText)
                    accepted = false
                }
                contactDialog.info = ""
                contactDialog.inputFieldText = ""
            }
        }

        onDeleteButtonClicked: {
            plot.deleteContact(contactDialog.indx)
        }

        onCopyButtonClicked: {
            plot.updateContact()
        }

        onInputAccepted: {
            contactDialog.visible = false
            plot.updateContact()
        }

        onSetButtonClicked: {
            contactDialog.visible = false
            plot.updateContact()
        }
    }

    onContactVisibleChanged: {
        contactDialog.visible = plot.contactVisible;

        if (contactDialog.visible) {
            contactDialog.info = plot.contactInfo
            contactDialog.inputFieldText =  plot.contactInfo
        }
        else {
            contactDialog.info = ""
            contactDialog.inputFieldText = ""
        }

        contactDialog.x = plot.contactPositionX
        contactDialog.y = plot.contactPositionY
        contactDialog.indx = plot.contactIndx
        contactDialog.lat = plot.contactLat
        contactDialog.lon = plot.contactLon
        contactDialog.depth = plot.contactDepth
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
            icon.source: "qrc:/icons/ui/direction_arrows.svg"
            checked: true
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(1)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/arrow_bar_to_down.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(2)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/pencil.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(3)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/arrow_bar_to_up.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(4)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/eraser.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(5)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/anchor.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight
            checkable: false

            onClicked: {
                contactDialog.x = mousearea.contactMouseX
                contactDialog.y = mousearea.contactMouseY
                contactDialog.visible = true;

                contactDialog.indx = -1

                menuBlock.visible = false
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/x.svg"
            backColor: theme.controlBackColor
            checkable: false
            implicitWidth: theme.controlHeight

            onClicked: {
                menuBlock.visible = false
            }

            ButtonGroup.group: pencilbuttonGroup
        }
    }
}

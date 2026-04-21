import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    id: root

    required property var workspaceRoot
    property int leafId: -1
    property string paneKind: "2D"
    property bool active: true

    signal scene3dRightReleased(real x, real y)
    property bool focusOnPointer: true
    property int lastKeyPressed: Qt.Key_unknown
    property int lastMouseButtons: Qt.NoButton
    property bool suppressNextClickAfterDoubleClick: false

    readonly property var plotTarget: (root.workspaceRoot
                                       && root.leafId >= 0
                                       && root.paneKind === "2D"
                                       && typeof root.workspaceRoot.plotItemForLeaf === "function")
                                      ? root.workspaceRoot.plotItemForLeaf(root.leafId)
                                      : null

    function markMouseKeyboardInput() {
        if (root.workspaceRoot
                && root.workspaceRoot.inputState
                && typeof root.workspaceRoot.inputState.markMouseKeyboardInput === "function") {
            root.workspaceRoot.inputState.markMouseKeyboardInput()
        }
    }

    function markKeyboardInput() {
        if (root.workspaceRoot
                && root.workspaceRoot.inputState
                && typeof root.workspaceRoot.inputState.markKeyboardInput === "function") {
            root.workspaceRoot.inputState.markKeyboardInput()
        }
    }

    function markTouchInput() {
        if (root.workspaceRoot
                && root.workspaceRoot.inputState
                && typeof root.workspaceRoot.inputState.markTouchInput === "function") {
            root.workspaceRoot.inputState.markTouchInput()
        }
    }

    function routeScene3DPress(mouseButton, buttons, x, y) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DMousePress !== "function")
            return
        root.workspaceRoot.forwardScene3DMousePress(buttons, x, y, lastKeyPressed)
    }

    function routeScene3DMove(buttons, x, y) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DMouseMove !== "function")
            return
        root.workspaceRoot.forwardScene3DMouseMove(buttons, x, y, lastKeyPressed)
    }

    function routeScene3DRelease(mouseButton, buttons, x, y) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DMouseRelease !== "function")
            return
        root.workspaceRoot.forwardScene3DMouseRelease(buttons, x, y, lastKeyPressed)
    }

    function routeScene3DWheel(buttons, x, y, angleDelta) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DMouseWheel !== "function")
            return
        root.workspaceRoot.forwardScene3DMouseWheel(buttons, x, y, angleDelta, lastKeyPressed)
    }

    function routeScene3DKey(key) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DKeyPress !== "function")
            return
        root.workspaceRoot.forwardScene3DKeyPress(key)
    }

    function routeScene3DPinch(prevCenterX, prevCenterY, currCenterX, currCenterY, scaleDelta, angleDelta) {
        if (!root.workspaceRoot || typeof root.workspaceRoot.forwardScene3DPinch !== "function")
            return
        root.workspaceRoot.forwardScene3DPinch(Qt.point(prevCenterX, prevCenterY), Qt.point(currCenterX, currCenterY), scaleDelta, angleDelta)
    }

    function routePlotPress(mouseButton, buttons, x, y) {
        if (!root.plotTarget || typeof root.plotTarget.handlePointerPress !== "function")
            return
        root.plotTarget.handlePointerPress(mouseButton, buttons, x, y, lastKeyPressed)
    }

    function routePlotClick(mouseButton, buttons, x, y) {
        if (!root.plotTarget || typeof root.plotTarget.handlePointerClick !== "function")
            return
        root.plotTarget.handlePointerClick(mouseButton, buttons, x, y, lastKeyPressed)
    }

    function routePlotMove(buttons, x, y) {
        if (!root.plotTarget || typeof root.plotTarget.handlePointerMove !== "function")
            return
        root.plotTarget.handlePointerMove(buttons, x, y, lastKeyPressed)
    }

    function routePlotRelease(mouseButton, buttons, x, y) {
        if (!root.plotTarget || typeof root.plotTarget.handlePointerRelease !== "function")
            return
        root.plotTarget.handlePointerRelease(mouseButton, buttons, x, y, lastKeyPressed)
    }

    function routePlotWheel(buttons, x, y, angleDelta, modifiers) {
        if (!root.plotTarget || typeof root.plotTarget.handlePointerWheel !== "function")
            return
        root.plotTarget.handlePointerWheel(buttons, x, y, angleDelta, modifiers, lastKeyPressed)
    }

    function routePlotPinchStarted(centerX, centerY) {
        if (!root.plotTarget || typeof root.plotTarget.handlePinchStarted !== "function")
            return
        root.plotTarget.handlePinchStarted(centerX, centerY)
    }

    function routePlotPinchUpdated(prevCenterX, prevCenterY, currCenterX, currCenterY, prevScale, scale, prevAngle, angle) {
        if (!root.plotTarget || typeof root.plotTarget.handlePinchUpdated !== "function")
            return
        root.plotTarget.handlePinchUpdated(prevCenterX, prevCenterY, currCenterX, currCenterY, prevScale, scale, prevAngle, angle)
    }

    function routePlotPinchFinished() {
        if (!root.plotTarget || typeof root.plotTarget.handlePinchFinished !== "function")
            return
        root.plotTarget.handlePinchFinished()
    }

    function toggleLeafFullscreen() {
        if (!root.workspaceRoot || !root.workspaceRoot.store || root.leafId < 0)
            return

        if (root.leafId === root.workspaceRoot.store.globalPopupLeafId) {
            if (typeof root.workspaceRoot.toggleGlobalPopupFullscreen === "function")
                root.workspaceRoot.toggleGlobalPopupFullscreen()
            return
        }

        if (typeof root.workspaceRoot.store.toggleLeafMaximize !== "function")
            return
        root.workspaceRoot.store.activeLeafId = root.leafId
        root.workspaceRoot.store.toggleLeafMaximize(root.leafId)
    }

    Item {
        id: overlay
        anchors.fill: parent
        visible: root.active
        enabled: root.active
        focus: true

        property bool pinchActive: false
        property bool pointerStarted: false

        MouseArea {
            id: pointerArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            hoverEnabled: true
            preventStealing: true
            enabled: root.active && !overlay.pinchActive
            cursorShape: Qt.ArrowCursor

            onEntered: if (root.focusOnPointer) overlay.forceActiveFocus()

            onPressed: function(mouse) {
                if (root.focusOnPointer)
                    overlay.forceActiveFocus()
                root.markMouseKeyboardInput()
                root.lastMouseButtons = mouse.buttons
                overlay.pointerStarted = true
                if (root.paneKind === "3D")
                    root.routeScene3DPress(mouse.button, mouse.buttons, mouse.x, mouse.y)
                else
                    root.routePlotPress(mouse.button, mouse.buttons, mouse.x, mouse.y)
            }

            onPositionChanged: function(mouse) {
                root.markMouseKeyboardInput()
                if (root.paneKind === "3D")
                    root.routeScene3DMove(mouse.buttons, mouse.x, mouse.y)
                else
                    root.routePlotMove(mouse.buttons, mouse.x, mouse.y)
            }

            onReleased: function(mouse) {
                root.markMouseKeyboardInput()
                if (root.paneKind === "3D") {
                    root.routeScene3DRelease(mouse.button, root.lastMouseButtons, mouse.x, mouse.y)
                    if (mouse.button === Qt.RightButton)
                        root.scene3dRightReleased(mouse.x, mouse.y)
                } else {
                    root.routePlotRelease(mouse.button, root.lastMouseButtons, mouse.x, mouse.y)
                }
                root.lastMouseButtons = Qt.NoButton
                overlay.pointerStarted = false
            }

            onCanceled: {
                root.markMouseKeyboardInput()
                if (root.paneKind === "3D" && root.workspaceRoot && typeof root.workspaceRoot.cancelScene3DPointerInteraction === "function")
                    root.workspaceRoot.cancelScene3DPointerInteraction()
                else if (root.paneKind === "2D" && root.plotTarget && typeof root.plotTarget.handlePointerCancel === "function")
                    root.plotTarget.handlePointerCancel()
                root.lastMouseButtons = Qt.NoButton
                overlay.pointerStarted = false
            }

            onClicked: function(mouse) {
                root.markMouseKeyboardInput()
                if (root.suppressNextClickAfterDoubleClick) {
                    mouse.accepted = true
                    return
                }
                if (root.paneKind === "2D")
                    root.routePlotClick(mouse.button, mouse.buttons, mouse.x, mouse.y)
            }

            onDoubleClicked: function(mouse) {
                root.markMouseKeyboardInput()
                if (root.focusOnPointer)
                    overlay.forceActiveFocus()

                mouse.accepted = true
                root.suppressNextClickAfterDoubleClick = true
                doubleClickBlocker.restart()
                root.toggleLeafFullscreen()
            }

            onWheel: function(wheel) {
                root.markMouseKeyboardInput()
                if (root.paneKind === "3D")
                    root.routeScene3DWheel(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta)
                else
                    root.routePlotWheel(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta, wheel.modifiers)
            }
        }

        PinchArea {
            id: pinchArea
            anchors.fill: parent
            enabled: root.active

            onPinchStarted: {
                root.markTouchInput()
                overlay.pinchActive = true
                pointerArea.enabled = false
                if (root.paneKind === "2D")
                    root.routePlotPinchStarted(pinch.center.x, pinch.center.y)
            }

            onPinchUpdated: function(pinch) {
                root.markTouchInput()
                if (root.paneKind === "3D") {
                    root.routeScene3DPinch(pinch.previousCenter.x, pinch.previousCenter.y,
                                           pinch.center.x, pinch.center.y,
                                           pinch.scale - pinch.previousScale,
                                           pinch.angle - pinch.previousAngle)
                } else {
                    root.routePlotPinchUpdated(pinch.previousCenter.x, pinch.previousCenter.y,
                                                pinch.center.x, pinch.center.y,
                                                pinch.previousScale, pinch.scale,
                                                pinch.previousAngle, pinch.angle)
                }
            }

            onPinchFinished: {
                root.markTouchInput()
                if (root.paneKind === "2D")
                    root.routePlotPinchFinished()
                overlay.pinchActive = false
                pointerArea.enabled = true
            }
        }

        Timer {
            id: doubleClickBlocker
            interval: 350
            repeat: false
            onTriggered: root.suppressNextClickAfterDoubleClick = false
        }

        Keys.enabled: true
        Keys.onPressed: function(event) {
            root.markKeyboardInput()
            lastKeyPressed = event.key
            if (root.paneKind === "3D") {
                root.routeScene3DKey(event.key)
                event.accepted = true
            }
        }

        Keys.onReleased: function(event) {
            if (event.key === lastKeyPressed)
                lastKeyPressed = Qt.Key_unknown
        }
    }
}

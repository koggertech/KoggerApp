import QtQuick 2.15
import QtQuick.Window 2.15
import kqml_types 1.0

Item {
    id: root

    required property var workspaceRoot
    property int leafId: -1
    property string paneKind: "2D"
    property bool active: true

    signal scene3dRightReleased(real x, real y, bool wasDrag)
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

    // Mark pane active on user interaction.
    function markActiveLeaf() {
        if (root.workspaceRoot && root.workspaceRoot.store && root.leafId >= 0)
            root.workspaceRoot.store.activeLeafId = root.leafId
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

    function modifierKey(modifiers) {
        if (modifiers & Qt.ControlModifier) return Qt.Key_Control
        if (modifiers & Qt.ShiftModifier)   return Qt.Key_Shift
        return Qt.Key_unknown
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

        // Mouse + single-finger touch (synthesis). preventStealing intentionally
        // false — lets PinchHandler grab both touches for 2-finger pinch.
        MouseArea {
            id: pointerArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            hoverEnabled: true
            preventStealing: false
            enabled: root.active && !overlay.pinchActive
            cursorShape: Qt.ArrowCursor

            onEntered: if (root.focusOnPointer) overlay.forceActiveFocus()

            // Manual double-tap detection — Qt's MouseArea.onDoubleClicked
            // uses tight thresholds (~5 px, ~400ms) that synthesised touch on
            // Android often misses. Track press timestamps here ourselves.
            property real _lastPressMs: 0
            property point _lastPressPos: Qt.point(-10000, -10000)

            // Set when a press was swallowed by the manual double-tap
            // recognizer. The matching release must NOT be routed to the
            // plot/3D scene, otherwise it sees a Release without a Press
            // and can leave hover/drag/selection state inconsistent.
            property bool _skipNextRelease: false

            // RMB press position — used to tell a click (open editor) from a
            // drag (box-select → apply active bottom-track tool) on release.
            property point _rmbPressPos: Qt.point(-10000, -10000)

            onPressed: function(mouse) {
                if (root.focusOnPointer)
                    overlay.forceActiveFocus()
                root.markMouseKeyboardInput()
                root.markActiveLeaf()
                root.lastMouseButtons = mouse.buttons
                overlay.pointerStarted = true
                root.lastKeyPressed = root.modifierKey(mouse.modifiers)

                if (mouse.button === Qt.RightButton)
                    pointerArea._rmbPressPos = Qt.point(mouse.x, mouse.y)

                // Manual double-tap recognizer (touch-friendly distance via
                // AppPalette.doubleTapDistancePx; interval = canonical 320ms).
                if (mouse.button === Qt.LeftButton) {
                    var now = Date.now()
                    var dx = mouse.x - pointerArea._lastPressPos.x
                    var dy = mouse.y - pointerArea._lastPressPos.y
                    var distSq = dx * dx + dy * dy
                    var maxDist = AppPalette.doubleTapDistancePx
                    var dt = now - pointerArea._lastPressMs

                    if (dt <= 320 && distSq <= maxDist * maxDist) {
                        pointerArea._lastPressMs = 0
                        pointerArea._lastPressPos = Qt.point(-10000, -10000)
                        root.markMouseKeyboardInput()
                        if (root.focusOnPointer)
                            overlay.forceActiveFocus()
                        root.suppressNextClickAfterDoubleClick = true
                        doubleClickBlocker.restart()
                        pointerArea._skipNextRelease = true
                        root.toggleLeafFullscreen()
                        mouse.accepted = true
                        return
                    }
                    pointerArea._lastPressMs = now
                    pointerArea._lastPressPos = Qt.point(mouse.x, mouse.y)
                }

                pointerArea._skipNextRelease = false
                if (root.paneKind === "3D")
                    root.routeScene3DPress(mouse.button, mouse.buttons, mouse.x, mouse.y)
                else
                    root.routePlotPress(mouse.button, mouse.buttons, mouse.x, mouse.y)
            }

            onPositionChanged: function(mouse) {
                root.markMouseKeyboardInput()
                root.lastKeyPressed = root.modifierKey(mouse.modifiers)
                if (root.paneKind === "3D")
                    root.routeScene3DMove(mouse.buttons, mouse.x, mouse.y)
                else
                    root.routePlotMove(mouse.buttons, mouse.x, mouse.y)
            }

            onReleased: function(mouse) {
                root.markMouseKeyboardInput()
                root.lastKeyPressed = root.modifierKey(mouse.modifiers)
                // If the matching press was swallowed by the double-tap
                // recognizer, drop this release too — routing it would
                // give the plot/3D scene an unbalanced Release.
                if (pointerArea._skipNextRelease) {
                    pointerArea._skipNextRelease = false
                    root.lastMouseButtons = Qt.NoButton
                    overlay.pointerStarted = false
                    mouse.accepted = true
                    return
                }
                if (root.paneKind === "3D") {
                    root.routeScene3DRelease(mouse.button, root.lastMouseButtons, mouse.x, mouse.y)
                    if (mouse.button === Qt.RightButton) {
                        var rdx = mouse.x - pointerArea._rmbPressPos.x
                        var rdy = mouse.y - pointerArea._rmbPressPos.y
                        root.scene3dRightReleased(mouse.x, mouse.y, (rdx * rdx + rdy * rdy) > 36)
                    }
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
                // Skip if manual onPressed recognizer already fired — otherwise
                // both paths toggle and cancel each other (stationary cursor
                // hits both Qt's tight 5 px / 400 ms threshold AND our wider
                // doubleTapDistancePx / 500 ms threshold).
                if (root.suppressNextClickAfterDoubleClick) {
                    mouse.accepted = true
                    return
                }
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
                root.lastKeyPressed = root.modifierKey(wheel.modifiers)
                if (root.paneKind === "3D")
                    root.routeScene3DWheel(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta)
                else
                    root.routePlotWheel(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta, wheel.modifiers)
            }
        }

        // 2-finger pinch — Qt 6 pointer handler. Pinch deltas are derived from
        // active{Scale,Rotation,Centroid} to keep the existing route* delta API.
        PinchHandler {
            id: pinchHandler
            target: null
            minimumPointCount: 2
            maximumPointCount: 2

            property real _prevScale: 1.0
            property real _prevRotation: 0
            property point _prevCenter: Qt.point(0, 0)

            onActiveChanged: {
                if (active) {
                    root.markTouchInput()
                    root.markActiveLeaf()
                    overlay.pinchActive = true
                    pointerArea.enabled = false
                    pinchHandler._prevScale = activeScale
                    pinchHandler._prevRotation = activeRotation
                    pinchHandler._prevCenter = centroid.position
                    if (root.paneKind === "2D")
                        root.routePlotPinchStarted(centroid.position.x, centroid.position.y)
                } else {
                    root.markTouchInput()
                    if (root.paneKind === "2D")
                        root.routePlotPinchFinished()
                    overlay.pinchActive = false
                    pointerArea.enabled = true
                }
            }

            onActiveScaleChanged: pinchHandler._emitUpdate()
            onActiveRotationChanged: pinchHandler._emitUpdate()
            onCentroidChanged: pinchHandler._emitUpdate()

            function _emitUpdate() {
                if (!active) return
                var currCenter = centroid.position
                if (root.paneKind === "3D") {
                    root.routeScene3DPinch(_prevCenter.x, _prevCenter.y,
                                           currCenter.x, currCenter.y,
                                           activeScale - _prevScale,
                                           activeRotation - _prevRotation)
                } else {
                    root.routePlotPinchUpdated(_prevCenter.x, _prevCenter.y,
                                                currCenter.x, currCenter.y,
                                                _prevScale, activeScale,
                                                _prevRotation, activeRotation)
                }
                _prevScale = activeScale
                _prevRotation = activeRotation
                _prevCenter = currCenter
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

import QtQuick 2.15
import SceneGraphRendering 1.0
import "qrc:/qml/scene2d/"
import kqml_types 1.0

Item {
    id: workspace

    required property var store

    property alias inputState: inputStateObject
    property bool sizeReportPending: false
    property var paneHostStacksByLeafId: ({})
    property var plotItemsByLeafId: ({})
    property Item active3DHostItem: null
    property int active3DLeafId: -1
    readonly property alias scene3dViewItem: scene3dView

    property var primaryPlotItem: null
    readonly property var leafRects: workspace.store ? workspace.store.leafRects : []
    readonly property string inputDeviceLabel: inputStateObject.displayLabel
    readonly property color inputDeviceColor: inputStateObject.displayColor

    function reportWorkspaceSize() {
        if (!workspace.store || typeof workspace.store.setWorkspaceSize !== "function")
            return
        workspace.store.setWorkspaceSize(width, height)
    }

    function scheduleWorkspaceSizeReport() {
        if (sizeReportPending)
            return

        sizeReportPending = true
        Qt.callLater(function() {
            sizeReportPending = false
            reportWorkspaceSize()
        })
    }

    function forwardScene3DMousePress(mouseButtons, x, y, keyboardKey) {
        if (scene3dView && typeof scene3dView.mousePressTrigger === "function")
            scene3dView.mousePressTrigger(mouseButtons, x, y, keyboardKey)
    }

    function forwardScene3DMouseMove(mouseButtons, x, y, keyboardKey) {
        if (scene3dView && typeof scene3dView.mouseMoveTrigger === "function")
            scene3dView.mouseMoveTrigger(mouseButtons, x, y, keyboardKey)
    }

    function forwardScene3DMouseRelease(mouseButtons, x, y, keyboardKey) {
        if (scene3dView && typeof scene3dView.mouseReleaseTrigger === "function")
            scene3dView.mouseReleaseTrigger(mouseButtons, x, y, keyboardKey)
    }

    function forwardScene3DMouseWheel(mouseButtons, x, y, angleDelta, keyboardKey) {
        if (scene3dView && typeof scene3dView.mouseWheelTrigger === "function")
            scene3dView.mouseWheelTrigger(mouseButtons, x, y, angleDelta, keyboardKey)
    }

    function forwardScene3DKeyPress(key) {
        if (scene3dView && typeof scene3dView.keyPressTrigger === "function")
            scene3dView.keyPressTrigger(key)
    }

    function forwardScene3DPinch(prevCenter, currCenter, scaleDelta, angleDelta) {
        if (scene3dView && typeof scene3dView.pinchTrigger === "function")
            scene3dView.pinchTrigger(prevCenter, currCenter, scaleDelta, angleDelta)
    }

    function cancelScene3DPointerInteraction() {
        if (scene3dView && typeof scene3dView.cancelPointerInteraction === "function")
            scene3dView.cancelPointerInteraction()
    }

    function toggleGlobalPopupFullscreen() {
        if (workspace.store)
            workspace.store.globalPopupFullscreen = !workspace.store.globalPopupFullscreen
    }

    InputDeviceState {
        id: inputStateObject
    }

    function normalizedPaneMode(value) {
        return value === "3D" ? "3D" : "2D"
    }

    function copyArray(values) {
        return values.slice(0)
    }

    function plotItemForLeaf(leafId) {
        if (leafId === undefined || leafId === null || leafId < 0)
            return null

        var item = plotItemsByLeafId[String(leafId)]
        return item !== undefined && item !== null ? item : null
    }

    function updateBottomTrackForAllPlots() {
        forEachRegisteredPlotItem(function(item) {
            if (typeof item.updateBottomTrackProcessing === "function")
                item.updateBottomTrackProcessing()
        })
    }

    function forEachRegisteredPlotItem(callback) {
        if (typeof callback !== "function")
            return

        for (var key in plotItemsByLeafId) {
            if (!Object.prototype.hasOwnProperty.call(plotItemsByLeafId, key))
                continue

            var item = plotItemsByLeafId[key]
            if (!item || item.visible === false || item.enabled === false)
                continue

            callback(item, key)
        }
    }

    function applyLegacy2DHotkey(functionName, parameter) {
        var fn = typeof functionName === "string" ? functionName : ""
        if (fn === "")
            return false

        var delta = Number(parameter)
        if (!isFinite(delta) || delta === 0)
            delta = 5

        var handled = false
        function applyToVisiblePlots(action) {
            forEachRegisteredPlotItem(function(item) {
                handled = true
                action(item)
            })
        }

        switch (fn) {
        case "horScrollLeft":
            applyToVisiblePlots(function(item) { item.horScrollEvent(-delta) })
            break
        case "horScrollRight":
            applyToVisiblePlots(function(item) { item.horScrollEvent(delta) })
            break
        case "verScrollUp":
            applyToVisiblePlots(function(item) { item.verScrollEvent(-delta) })
            break
        case "verScrollDown":
            applyToVisiblePlots(function(item) { item.verScrollEvent(delta) })
            break
        case "verZoomOut":
            applyToVisiblePlots(function(item) { item.verZoomEvent(-delta) })
            break
        case "verZoomIn":
            applyToVisiblePlots(function(item) { item.verZoomEvent(delta) })
            break
        case "increaseLowLevel":
            applyToVisiblePlots(function(item) {
                var newLow = Math.min(120, item.getLowEchogramLevel() + delta)
                var newHigh = item.getHighEchogramLevel()
                if (newLow > newHigh)
                    newHigh = newLow
                item.plotEchogramSetLevels(newLow, newHigh)
                item.setLevels(newLow, newHigh)
            })
            break
        case "decreaseLowLevel":
            applyToVisiblePlots(function(item) {
                var low = Math.max(0, item.getLowEchogramLevel() - delta)
                var high = item.getHighEchogramLevel()
                item.plotEchogramSetLevels(low, high)
                item.setLevels(low, high)
            })
            break
        case "increaseHighLevel":
            applyToVisiblePlots(function(item) {
                var high = Math.min(120, item.getHighEchogramLevel() + delta)
                var low = item.getLowEchogramLevel()
                item.plotEchogramSetLevels(low, high)
                item.setLevels(low, high)
            })
            break
        case "decreaseHighLevel":
            applyToVisiblePlots(function(item) {
                var high = Math.max(0, item.getHighEchogramLevel() - delta)
                var low = item.getLowEchogramLevel()
                if (high < low)
                    low = high
                item.plotEchogramSetLevels(low, high)
                item.setLevels(low, high)
            })
            break
        case "prevTheme":
            applyToVisiblePlots(function(item) {
                var themeId = item.getThemeId()
                if (themeId > 0)
                    item.plotEchogramTheme(themeId - 1)
            })
            break
        case "nextTheme":
            applyToVisiblePlots(function(item) {
                var themeId = item.getThemeId()
                if (themeId < 9)
                    item.plotEchogramTheme(themeId + 1)
            })
            break
        default:
            return false
        }

        return handled
    }

    function leafRectForId(leafId) {
        if (!leafRects)
            return null

        for (var i = 0; i < leafRects.length; ++i) {
            if (leafRects[i].leafId === leafId)
                return leafRects[i]
        }

        return null
    }

    function firstVisiblePlotItem() {
        if (workspace.store && workspace.store.leafRects) {
            for (var i = 0; i < workspace.store.leafRects.length; ++i) {
                var rect = workspace.store.leafRects[i]
                if (!rect)
                    continue

                var item = plotItemForLeaf(rect.leafId)
                if (item)
                    return item
            }
        }

        for (var key in plotItemsByLeafId) {
            if (Object.prototype.hasOwnProperty.call(plotItemsByLeafId, key) && plotItemsByLeafId[key])
                return plotItemsByLeafId[key]
        }

        return null
    }

    function paneHostItemForLeaf(leafId, mode) {
        if (leafId === undefined || leafId === null || leafId < 0)
            return null

        var stack = paneHostStacksByLeafId[String(leafId)]
        if (!stack || stack.length === 0)
            return null

        var targetMode = normalizedPaneMode(mode)
        for (var i = stack.length - 1; i >= 0; --i) {
            var entry = stack[i]
            if (!entry)
                continue
            if (targetMode !== "" && normalizedPaneMode(entry.mode) !== targetMode)
                continue
            return entry.hostItem
        }

        return null
    }

    function copyObject(values) {
        var next = {}
        if (!values)
            return next

        for (var key in values) {
            if (Object.prototype.hasOwnProperty.call(values, key))
                next[key] = values[key]
        }
        return next
    }

    function registerPlotItem(leafId, item) {
        if (leafId < 0 || !item)
            return

        var key = String(leafId)
        if (plotItemsByLeafId[key] === item)
            return

        var nextItems = copyObject(plotItemsByLeafId)
        nextItems[key] = item
        plotItemsByLeafId = nextItems
        refreshPrimaryPlotItem()
    }

    function unregisterPlotItem(leafId, item) {
        if (leafId < 0 || !item)
            return

        var key = String(leafId)
        if (plotItemsByLeafId[key] !== item)
            return

        var nextItems = copyObject(plotItemsByLeafId)
        delete nextItems[key]
        plotItemsByLeafId = nextItems
        refreshPrimaryPlotItem()
    }

    function refreshPrimaryPlotItem() {
        var activeLeafPlot = workspace.store ? plotItemForLeaf(workspace.store.activeLeafId) : null
        if (activeLeafPlot) {
            primaryPlotItem = activeLeafPlot
            return
        }

        var visiblePlot = firstVisiblePlotItem()
        if (visiblePlot) {
            primaryPlotItem = visiblePlot
            return
        }

        for (var key in plotItemsByLeafId) {
            if (Object.prototype.hasOwnProperty.call(plotItemsByLeafId, key) && plotItemsByLeafId[key]) {
                primaryPlotItem = plotItemsByLeafId[key]
                return
            }
        }
        primaryPlotItem = null
    }

    function refreshHostAssignmentsForLeaf(leafId) {
        var stack = paneHostStacksByLeafId[leafId]
        var topEntry = stack && stack.length > 0 ? stack[stack.length - 1] : null

        if (!topEntry) {
            if (active3DLeafId === leafId) {
                active3DLeafId = -1
                active3DHostItem = null
            }
            return
        }

        var mode = normalizedPaneMode(topEntry.mode)
        if (mode === "3D") {
            active3DLeafId = leafId
            active3DHostItem = topEntry.hostItem
            return
        }

        if (active3DLeafId === leafId) {
            active3DLeafId = -1
            if (active3DHostItem === topEntry.hostItem)
                active3DHostItem = null
        }
    }

    function registerPaneHost(leafId, hostItem, mode) {
        if (leafId < 0 || !hostItem)
            return

        var normalizedMode = normalizedPaneMode(mode)
        var stack = paneHostStacksByLeafId[leafId]
        var nextStack = []
        if (stack && stack.length > 0) {
            for (var i = 0; i < stack.length; ++i) {
                if (stack[i].hostItem !== hostItem)
                    nextStack.push(stack[i])
            }
        }

        nextStack.push({ hostItem: hostItem, mode: normalizedMode })
        var nextStacks = copyObject(paneHostStacksByLeafId)
        nextStacks[String(leafId)] = nextStack
        paneHostStacksByLeafId = nextStacks
        refreshHostAssignmentsForLeaf(leafId)
    }

    function unregisterPaneHost(leafId, hostItem) {
        var stack = paneHostStacksByLeafId[leafId]
        if (!stack || stack.length === 0)
            return

        var nextStack = []
        var removed = false
        for (var i = 0; i < stack.length; ++i) {
            if (stack[i].hostItem === hostItem) {
                removed = true
                continue
            }
            nextStack.push(stack[i])
        }

        if (!removed)
            return

        var nextStacks = copyObject(paneHostStacksByLeafId)
        nextStacks[String(leafId)] = nextStack
        paneHostStacksByLeafId = nextStacks
        refreshHostAssignmentsForLeaf(leafId)
    }

    onWidthChanged: scheduleWorkspaceSizeReport()
    onHeightChanged: scheduleWorkspaceSizeReport()

    Component.onCompleted: {
        scheduleWorkspaceSizeReport()
    }

    Connections {
        target: workspace.store
        ignoreUnknownSignals: true

        function onActiveLeafIdChanged() {
            refreshPrimaryPlotItem()
        }
    }

    Item {
        id: renderRoot
        anchors.fill: parent
        z: -10

        GraphicsScene3dView {
            id: scene3dView
            objectName: "GraphicsScene3dView"
            parent: workspace.active3DHostItem ? workspace.active3DHostItem : renderRoot
            anchors.fill: parent
            visible: workspace.active3DHostItem !== null
            focus: true
        }

        Repeater {
            model: 4

            delegate: Plot2D {
                id: slotPlot

                required property int index
                readonly property int slotIndex: index
                readonly property int slotLeafId: workspace.store
                                                && workspace.store.leafRects
                                                && slotIndex < workspace.store.leafRects.length
                                                && workspace.store.leafRects[slotIndex]
                                                ? workspace.store.leafRects[slotIndex].leafId
                                                : -1
                readonly property var slotLeafRect: workspace.leafRectForId(slotLeafId)
                readonly property var slotHostItem: workspace.paneHostItemForLeaf(slotLeafId, "2D")
                property int registeredLeafId: -1

                inputState: workspace.inputState
                externalInputRouting: true
                parent: slotHostItem ? slotHostItem : renderRoot
                anchors.fill: parent
                visible: slotLeafRect !== null && slotHostItem !== null
                enabled: visible
                focus: false
                indx: slotIndex + 1
                is3dVisible: workspace.active3DHostItem !== null

                Component.onCompleted: {
                    setIndx(indx)
                    if (typeof core !== "undefined" && core && typeof core.registerPlot2D === "function")
                        core.registerPlot2D(slotPlot)
                    syncPlotRegistration()
                }

                onSlotLeafIdChanged: {
                    syncPlotRegistration()
                }

                Component.onDestruction: {
                    if (registeredLeafId > 0 && workspace && typeof workspace.unregisterPlotItem === "function")
                        workspace.unregisterPlotItem(registeredLeafId, slotPlot)
                }

                function syncPlotRegistration() {
                    if (registeredLeafId > 0 && registeredLeafId !== slotLeafId && workspace && typeof workspace.unregisterPlotItem === "function") {
                        workspace.unregisterPlotItem(registeredLeafId, slotPlot)
                        registeredLeafId = -1
                    }

                    if (slotLeafId > 0 && workspace && typeof workspace.registerPlotItem === "function") {
                        workspace.registerPlotItem(slotLeafId, slotPlot)
                        registeredLeafId = slotLeafId
                    }
                }
            }
        }

        Plot2D {
            id: globalPopupPlot

            readonly property var slotHostItem: workspace.paneHostItemForLeaf(
                workspace.store ? workspace.store.globalPopupLeafId : -1, "2D")

            inputState: workspace.inputState
            externalInputRouting: true
            parent: slotHostItem ? slotHostItem : renderRoot
            anchors.fill: parent
            visible: slotHostItem !== null
            enabled: visible
            focus: false
            indx: 5
            is3dVisible: workspace.active3DHostItem !== null

            Component.onCompleted: {
                setIndx(5)
                if (typeof core !== "undefined" && core && typeof core.registerPlot2D === "function")
                    core.registerPlot2D(globalPopupPlot)
                if (workspace.store && typeof workspace.registerPlotItem === "function")
                    workspace.registerPlotItem(workspace.store.globalPopupLeafId, globalPopupPlot)
            }

            Component.onDestruction: {
                if (workspace && workspace.store && typeof workspace.unregisterPlotItem === "function")
                    workspace.unregisterPlotItem(workspace.store.globalPopupLeafId, globalPopupPlot)
            }
        }
    }

    Repeater {
        model: workspace.store.leafRectModel

        delegate: PaneFrame {
            required property var rectData
            paneRect: rectData
            store: workspace.store
            workspaceItem: workspace
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: workspace.store.modePickerLeafId !== -1
        z: ZOrder.dragOverlay
        color: "#02061799"

        MouseArea {
            anchors.fill: parent
        }
    }

    Repeater {
        model: workspace.store.modePickerLeafId === -1 && workspace.store.maximizedLeafId === -1
               ? workspace.store.splitHandleModel : []

        delegate: Item {
            id: splitDragZone

            required property var handleData

            readonly property bool vertical: handleData.orientation === "vertical"
            readonly property real hitSize: 20
            readonly property bool isActiveResizeSplit: workspace.store.edgeResizeMovingSplitId === handleData.splitId
            readonly property bool showResizeHandle: splitDragMouse.resizing || isActiveResizeSplit

            x: vertical ? handleData.x - hitSize / 2 : handleData.x
            y: vertical ? handleData.y : handleData.y - hitSize / 2
            width: vertical ? hitSize : handleData.width
            height: vertical ? handleData.height : hitSize
            z: ZOrder.dropZoneHighlight

            Rectangle {
                visible: splitDragZone.showResizeHandle
                color: "#60A5FA"
                width: splitDragZone.vertical ? 2 : splitDragZone.width
                height: splitDragZone.vertical ? splitDragZone.height : 2
                x: splitDragZone.vertical ? (splitDragZone.width - width) / 2 : 0
                y: splitDragZone.vertical ? 0 : (splitDragZone.height - height) / 2
                opacity: 0.9
            }

            Rectangle {
                id: splitResizeThumb
                visible: splitDragZone.showResizeHandle
                width: splitDragZone.vertical ? 12 : 38
                height: splitDragZone.vertical ? 38 : 12
                x: (splitDragZone.width - width) / 2
                y: (splitDragZone.height - height) / 2
                radius: 6
                color: "#1D4ED8"
                border.width: 1
                border.color: "#BFDBFE"
                opacity: 0.95

                Column {
                    anchors.centerIn: parent
                    visible: splitDragZone.vertical
                    spacing: 3

                    Repeater {
                        model: 3
                        delegate: Rectangle {
                            width: 4
                            height: 2
                            radius: 1
                            color: "#E2E8F0"
                        }
                    }
                }

                Row {
                    anchors.centerIn: parent
                    visible: !splitDragZone.vertical
                    spacing: 3

                    Repeater {
                        model: 3
                        delegate: Rectangle {
                            width: 2
                            height: 4
                            radius: 1
                            color: "#E2E8F0"
                        }
                    }
                }
            }

            MouseArea {
                id: splitDragMouse

                property bool resizing: false

                anchors.fill: parent
                hoverEnabled: workspace.store.editableMode
                acceptedButtons: Qt.LeftButton
                pressAndHoldInterval: 280
                cursorShape: resizing
                             ? Qt.ClosedHandCursor
                             : (workspace.store.editableMode && workspace.store.modePickerLeafId === -1
                                ? (splitDragZone.vertical ? Qt.SplitHCursor : Qt.SplitVCursor)
                                : Qt.ArrowCursor)
                preventStealing: true

                function pointerInWorkspace(mouse) {
                    return splitDragMouse.mapToItem(workspace, mouse.x, mouse.y)
                }

                function startResize(mouse) {
                    var p = pointerInWorkspace(mouse)
                    var nearest = workspace.store.nearestSplitHandleAtPoint(p.x, p.y, handleData.orientation, 18)
                    var splitId = nearest ? nearest.splitId : handleData.splitId
                    var orientation = nearest ? nearest.orientation : handleData.orientation
                    resizing = workspace.store.beginResizeForSplitHandle(splitId, orientation, p.x, p.y)
                    if (resizing)
                        workspace.store.updateEdgeResize(p.x, p.y)
                }

                onPressed: function(mouse) {
                    if (!workspace.store.editableMode) {
                        resizing = false
                        return
                    }
                    startResize(mouse)
                }

                onPressAndHold: function(mouse) {
                    if (workspace.store.editableMode)
                        return
                    startResize(mouse)
                }

                onPositionChanged: function(mouse) {
                    if (!pressed || !resizing)
                        return
                    var p = pointerInWorkspace(mouse)
                    workspace.store.updateEdgeResize(p.x, p.y)
                }

                onReleased: {
                    if (resizing)
                        workspace.store.finishEdgeResize()
                    resizing = false
                }

                onCanceled: {
                    if (resizing)
                        workspace.store.finishEdgeResize()
                    resizing = false
                }
            }
        }
    }

    Rectangle {
        visible: workspace.store.dragActive
        width: 22
        height: 24
        radius: 6
        x: workspace.store.dragCursor.x - width / 2
        y: workspace.store.dragCursor.y - height / 2
        color: "#1E40AF"
        border.color: "#BFDBFE"
        z: ZOrder.dragOverlay
        opacity: 0.9

        Text {
            anchors.centerIn: parent
            text: "::"
            color: "#E2E8F0"
            font.pixelSize: 11
            font.bold: true
        }
    }
}

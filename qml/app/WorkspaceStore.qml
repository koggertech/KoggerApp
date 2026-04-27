import QtQuick 2.15
import QtCore
import "LayoutRules.js" as Rules
import "LayoutTree.js" as Tree
import "LayoutResize.js" as Resize

QtObject {
    id: store

    property real windowWidth: 0
    property real windowHeight: 0
    property real workspaceWidth: 0
    property real workspaceHeight: 0
    property real pendingWorkspaceWidth: 0
    property real pendingWorkspaceHeight: 0

    signal windowGeometryRestoreRequested(int width, int height)

    property var layoutTree: null
    property var leafRects: []
    property var splitHandles: []
    property ListModel leafRectModel: ListModel { }
    property ListModel splitHandleModel: ListModel { }

property int nextLeafSerial: 0
property int nextSplitSerial: 0

property int draggedLeafId: -1
property int dropTargetLeafId: -1
property point dragCursor: Qt.point(0, 0)
property bool dragActive: false
property var slotContentIds: []

property int activeLeafId: -1
property int edgeResizeMovingSplitId: -1
property string edgeResizeMovingSide: ""
property int edgeResizeFixedSplitId: -1
property string edgeResizeFixedSide: ""
property string edgeResizeAxis: ""
property int edgeResizeHighlightLeafId: -1
property string edgeResizeHighlightEdge: ""
property real edgeResizePointerStart: 0
property real edgeResizeMovingCoordStart: 0
property real edgeResizeFixedCoord: 0
property string edgeResizeFavoriteSignatureBefore: ""
property bool editableMode: false
property int maximizedLeafId: -1
property int lastTappedLeafId: -1
property real lastTapTimestamp: 0
property bool settingsPanelOpen: false
property bool modeSettingsPanelOpen: false
property bool settingsPushContent: false
property bool resizeActive: false
property bool layoutTransitionSuspended: false
property string settingsSide: "left"
property string selectedConnectionFilePath: ""
property bool quickActionFavoritesEnabled: true
property bool quickActionMarkerEnabled: true
property bool quickActionConnectionStatusEnabled: true
property string hotkeysRevealKey: ""
property int hotkeysRevealNonce: 0
property int modeSettingsLeafId: -1
property string modeSettingsMode: "2D"
readonly property int settingsPanelSizePx: 380
property int modePickerLeafId: -1
property var modePickerLeafIds: []
property int hoveredPopupCandidateLeafId: -1
property int flashingLeafId: -1
readonly property int globalPopupLeafId: 9999
property bool globalPopupFullscreen: false
property var favoriteLayouts: []
property bool currentLayoutIsFavorite: false
property string currentLayoutFavoriteSignature: ""
property var settingsGroupExpandedMap: ({})
property var fullscreenPopupSourceByHost: ({})
property var fullscreenPopupStateByHost: ({})
property bool globalPopupEnabled: false
property string globalPopupMode: ""
property bool globalPopupModePickerOpen: false
property bool globalPopupPreferencesLoading: false
property var globalPopupState: ({
    x: -1,
    y: -1,
    collapsed: false,
    expandedWidth: -1,
    expandedHeight: -1
})

readonly property real splitterThickness: 0
readonly property real minPaneSize: 120
readonly property real splitSnapThresholdPx: 18
readonly property int maxPaneCount: 4
readonly property int doubleTapIntervalMs: 320
readonly property int popupMarginPx: 16
readonly property int workspaceResizeDebounceMs: 120

signal workspaceSizeCommitRequested()

property Settings layoutStore: Settings {
    category: "workspace"

    property string layoutJson: ""
    property int nextLeafSerialStored: 0
    property int nextSplitSerialStored: 0
    property int activeLeafIdStored: -1
    property int windowWidthStored: 0
    property int windowHeightStored: 0
    property bool settingsPushContentStored: false
    property string settingsSideStored: "left"
    property bool quickActionFavoritesEnabledStored: true
    property bool quickActionMarkerEnabledStored: true
    property bool quickActionConnectionStatusEnabledStored: true
    property string selectedConnectionFilePathStored: ""
    property string favoriteLayoutsJson: "[]"
    property string settingsGroupExpandedJson: "{}"
    property string fullscreenPopupSourceJson: "{}"
    property string fullscreenPopupStateJson: "{}"
    property bool globalPopupEnabledStored: false
    property string globalPopupModeStored: ""
    property string globalPopupStateJson: "{\"x\":-1,\"y\":-1,\"collapsed\":false,\"expandedWidth\":-1,\"expandedHeight\":-1}"
}

onEditableModeChanged: {
    if (editableMode && maximizedLeafId !== -1)
        maximizedLeafId = -1
}

onSettingsPanelOpenChanged: {
    if (settingsPanelOpen)
        closeModeSettingsPanel()
    else if (editableMode)
        editableMode = false
}

onSettingsSideChanged: {
    var normalizedSide = normalizedSettingsSide(settingsSide)
    if (settingsSide !== normalizedSide)
        settingsSide = normalizedSide
}

onModeSettingsPanelOpenChanged: {
    if (modeSettingsPanelOpen)
        settingsPanelOpen = false
}

onLayoutTreeChanged: {
    sanitizeFullscreenPopupConfig()
    updateCurrentLayoutFavoriteState()
}

onGlobalPopupEnabledChanged: {
    if (globalPopupPreferencesLoading)
        return

    if (globalPopupEnabled) {
        if (normalizedGlobalPopupMode(globalPopupMode) === "")
            globalPopupModePickerOpen = true
    } else {
        globalPopupMode = ""
        globalPopupModePickerOpen = false
        globalPopupState = defaultGlobalPopupState()
    }
    saveGlobalPopupPreferences()
}
onGlobalPopupModeChanged: {
    if (globalPopupPreferencesLoading)
        return
    saveGlobalPopupPreferences()
}

function closeModeSettingsPanel() {
    modeSettingsPanelOpen = false
    modeSettingsLeafId = -1
}

function effectiveWorkspaceWidth() {
    return workspaceWidth > 0 ? workspaceWidth : windowWidth
}

function effectiveWorkspaceHeight() {
    return workspaceHeight > 0 ? workspaceHeight : windowHeight
}

function setWorkspaceSize(width, height) {
    var nextWidth = Math.max(0, Math.round(width))
    var nextHeight = Math.max(0, Math.round(height))
    var changed = nextWidth !== workspaceWidth || nextHeight !== workspaceHeight

    pendingWorkspaceWidth = nextWidth
    pendingWorkspaceHeight = nextHeight
    workspaceWidth = nextWidth
    workspaceHeight = nextHeight

    if (changed)
        rebuildLayoutCaches()

    if (!changed && !resizeActive)
        return

    resizeActive = true
    layoutTransitionSuspended = true
    workspaceSizeCommitRequested()
}

function commitWorkspaceSize() {
    if (workspaceWidth !== pendingWorkspaceWidth)
        workspaceWidth = pendingWorkspaceWidth
    if (workspaceHeight !== pendingWorkspaceHeight)
        workspaceHeight = pendingWorkspaceHeight

    resizeActive = false
    layoutTransitionSuspended = false
}

function syncRectModel(model, roleName, items, keyName) {
    if (!model)
        return

    var sameStructure = model.count === items.length
    if (sameStructure) {
        for (var i = 0; i < items.length; ++i) {
            var current = model.get(i)
            var currentItem = current ? current[roleName] : null
            if (!currentItem || currentItem[keyName] !== items[i][keyName]) {
                sameStructure = false
                break
            }
        }
    }

    if (!sameStructure) {
        model.clear()
        for (var j = 0; j < items.length; ++j) {
            var entry = {}
            entry[roleName] = items[j]
            model.append(entry)
        }
        return
    }

    for (var k = 0; k < items.length; ++k) {
        var nextEntry = {}
        nextEntry[roleName] = items[k]
        model.set(k, nextEntry)
    }
}

function openAppLayoutSettings() {
    closeModeSettingsPanel()
    settingsPanelOpen = true
    setSettingsGroupExpanded("app.layoutPlacement", true)
}

function openConnectionsSettings() {
    closeModeSettingsPanel()
    settingsPanelOpen = true
    setSettingsGroupExpanded("app.connections", true)
}

function toggleAppLayoutSettings() {
    if (settingsPanelOpen) {
        settingsPanelOpen = false
        return
    }

    closeModeSettingsPanel()
    settingsPanelOpen = true
}

function toggleConnectionsSettings() {
    if (settingsPanelOpen && isSettingsGroupExpanded("app.connections")) {
        settingsPanelOpen = false
        return
    }

    openConnectionsSettings()
}

function requestHotkeysReveal(key) {
    hotkeysRevealKey = typeof key === "string" ? key : ""
    hotkeysRevealNonce += 1
}

function syncModePickerLeafId() {
    modePickerLeafId = modePickerLeafIds.length > 0 ? modePickerLeafIds[0] : -1
}

function clearModePickerSelection() {
    modePickerLeafIds = []
    modePickerLeafId = -1
}

function setModePickerLeafIds(ids) {
    if (!ids) {
        clearModePickerSelection()
        return
    }

    var unique = []
    for (var i = 0; i < ids.length; ++i) {
        var leafId = ids[i]
        if (typeof leafId !== "number" || leafId <= 0)
            continue
        if (!hasLeafIdInTree(layoutTree, leafId))
            continue
        if (unique.indexOf(leafId) !== -1)
            continue
        unique.push(leafId)
    }

    modePickerLeafIds = unique
    syncModePickerLeafId()
}

function removeModePickerLeafId(leafId) {
    if (modePickerLeafIds.length === 0) {
        modePickerLeafId = -1
        return
    }

    var next = []
    for (var i = 0; i < modePickerLeafIds.length; ++i) {
        if (modePickerLeafIds[i] !== leafId)
            next.push(modePickerLeafIds[i])
    }
    modePickerLeafIds = next
    syncModePickerLeafId()
}

function refreshModePickerLeafIds() {
    setModePickerLeafIds(modePickerLeafIds)
}

function isLeafModeSelecting(leafId) {
    return modePickerLeafIds.indexOf(leafId) !== -1
}

function parseLeafId(value) {
    var id = typeof value === "number" ? Math.round(value) : parseInt(value)
    if (isNaN(id) || id <= 0)
        return -1
    return id
}

function defaultGlobalPopupState() {
    return {
        x: -1,
        y: -1,
        collapsed: false,
        expandedWidth: -1,
        expandedHeight: -1
    }
}

function normalizedGlobalPopupState(rawState) {
    var x = -1
    var y = -1
    var collapsed = false
    var expandedWidth = -1
    var expandedHeight = -1

    if (rawState && typeof rawState === "object" && !Array.isArray(rawState)) {
        if (typeof rawState.x === "number" && isFinite(rawState.x))
            x = rawState.x
        if (typeof rawState.y === "number" && isFinite(rawState.y))
            y = rawState.y
        if (typeof rawState.expandedWidth === "number" && isFinite(rawState.expandedWidth) && rawState.expandedWidth > 0)
            expandedWidth = rawState.expandedWidth
        if (typeof rawState.expandedHeight === "number" && isFinite(rawState.expandedHeight) && rawState.expandedHeight > 0)
            expandedHeight = rawState.expandedHeight
        collapsed = rawState.collapsed === true
    }

    return {
        x: x,
        y: y,
        collapsed: collapsed,
        expandedWidth: expandedWidth,
        expandedHeight: expandedHeight
    }
}

function normalizedGlobalPopupMode(value) {
    return value === "3D" ? "3D"
                         : value === "2D" ? "2D"
                                          : ""
}

function saveGlobalPopupPreferences() {
    layoutStore.globalPopupEnabledStored = globalPopupEnabled === true
    layoutStore.globalPopupModeStored = normalizedGlobalPopupMode(globalPopupMode)
    layoutStore.globalPopupStateJson = JSON.stringify(normalizedGlobalPopupState(globalPopupState))
}

function loadGlobalPopupPreferences() {
    globalPopupPreferencesLoading = true

    var nextMode = normalizedGlobalPopupMode(layoutStore.globalPopupModeStored)
    var nextEnabled = layoutStore.globalPopupEnabledStored
    var parsed = defaultGlobalPopupState()
    if (layoutStore.globalPopupStateJson && layoutStore.globalPopupStateJson !== "") {
        try {
            parsed = JSON.parse(layoutStore.globalPopupStateJson)
        } catch (error) {
            parsed = defaultGlobalPopupState()
        }
    }

    globalPopupMode = nextEnabled ? nextMode : ""
    globalPopupEnabled = nextEnabled
    globalPopupModePickerOpen = globalPopupEnabled && normalizedGlobalPopupMode(globalPopupMode) === ""
    globalPopupState = nextEnabled ? normalizedGlobalPopupState(parsed) : defaultGlobalPopupState()

    globalPopupPreferencesLoading = false
    saveGlobalPopupPreferences()
}

function globalPopupCanChoose3D() {
    return firstLeafIdByMode(layoutTree, "3D") === -1
}

function setGlobalPopupMode(mode) {
    var nextMode = normalizedGlobalPopupMode(mode)
    if (nextMode === "")
        return false
    if (nextMode === "3D" && !globalPopupCanChoose3D())
        return false

    globalPopupMode = nextMode
    globalPopupModePickerOpen = false
    saveGlobalPopupPreferences()
    return true
}

function globalPopupExpandedSize(defaultWidth, defaultHeight) {
    var state = normalizedGlobalPopupState(globalPopupState)
    var width = state.expandedWidth > 0 ? state.expandedWidth : defaultWidth
    var height = state.expandedHeight > 0 ? state.expandedHeight : defaultHeight
    return Qt.size(Math.max(80, width), Math.max(80, height))
}

function setGlobalPopupExpandedSize(expandedWidth, expandedHeight) {
    var nextState = normalizedGlobalPopupState(globalPopupState)
    nextState.expandedWidth = Math.max(80, expandedWidth)
    nextState.expandedHeight = Math.max(80, expandedHeight)
    globalPopupState = nextState
    saveGlobalPopupPreferences()
}

function globalPopupPosition(popupWidth, popupHeight) {
    var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
    var areaHeight = windowHeight > 0 ? windowHeight : workspaceHeight

    var spacing = popupMarginPx
    var minX = spacing
    var minY = spacing
    var maxX = areaWidth - popupWidth - spacing
    var maxY = areaHeight - popupHeight - spacing

    if (maxX < minX) {
        minX = 0
        maxX = Math.max(0, areaWidth - popupWidth)
    }
    if (maxY < minY) {
        minY = 0
        maxY = Math.max(0, areaHeight - popupHeight)
    }

    var state = normalizedGlobalPopupState(globalPopupState)
    var x = state.x >= 0 ? state.x : maxX
    var y = state.y >= 0 ? state.y : maxY
    return Qt.point(clamp(x, minX, maxX), clamp(y, minY, maxY))
}

function setGlobalPopupPosition(x, y, popupWidth, popupHeight) {
    var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
    var areaHeight = windowHeight > 0 ? windowHeight : workspaceHeight

    var spacing = popupMarginPx
    var minX = spacing
    var minY = spacing
    var maxX = areaWidth - popupWidth - spacing
    var maxY = areaHeight - popupHeight - spacing

    if (maxX < minX) {
        minX = 0
        maxX = Math.max(0, areaWidth - popupWidth)
    }
    if (maxY < minY) {
        minY = 0
        maxY = Math.max(0, areaHeight - popupHeight)
    }

    var nextState = normalizedGlobalPopupState(globalPopupState)
    nextState.x = clamp(x, minX, maxX)
    nextState.y = clamp(y, minY, maxY)
    globalPopupState = nextState
    saveGlobalPopupPreferences()
}

function globalPopupCollapsed() {
    return normalizedGlobalPopupState(globalPopupState).collapsed === true
}

function setGlobalPopupCollapsed(collapsed) {
    var nextState = normalizedGlobalPopupState(globalPopupState)
    nextState.collapsed = collapsed === true
    globalPopupState = nextState
    saveGlobalPopupPreferences()
}

function cloneObjectMap(source) {
    var next = {}
    if (!source || typeof source !== "object" || Array.isArray(source))
        return next

    for (var key in source) {
        if (!Object.prototype.hasOwnProperty.call(source, key))
            continue
        next[key] = source[key]
    }
    return next
}

function saveFullscreenPopupState() {
    layoutStore.fullscreenPopupSourceJson = JSON.stringify(fullscreenPopupSourceByHost)
    layoutStore.fullscreenPopupStateJson = JSON.stringify(fullscreenPopupStateByHost)
}

function loadFullscreenPopupState() {
    var parsedSource = {}
    if (layoutStore.fullscreenPopupSourceJson && layoutStore.fullscreenPopupSourceJson !== "") {
        try {
            parsedSource = JSON.parse(layoutStore.fullscreenPopupSourceJson)
        } catch (error) {
            parsedSource = {}
        }
    }

    var parsedState = {}
    if (layoutStore.fullscreenPopupStateJson && layoutStore.fullscreenPopupStateJson !== "") {
        try {
            parsedState = JSON.parse(layoutStore.fullscreenPopupStateJson)
        } catch (error) {
            parsedState = {}
        }
    }

    fullscreenPopupSourceByHost = parsedSource && typeof parsedSource === "object" && !Array.isArray(parsedSource)
                               ? parsedSource : ({})
    fullscreenPopupStateByHost = parsedState && typeof parsedState === "object" && !Array.isArray(parsedState)
                              ? parsedState : ({})
}

function popupNormalizedStateEntry(rawEntry) {
    var x = null
    var y = null
    var expandedWidth = null
    var expandedHeight = null
    var xRatio = 1
    var yRatio = 1
    var collapsed = false

    if (rawEntry && typeof rawEntry === "object" && !Array.isArray(rawEntry)) {
        if (typeof rawEntry.x === "number")
            x = rawEntry.x
        if (typeof rawEntry.y === "number")
            y = rawEntry.y
        if (typeof rawEntry.expandedWidth === "number" && rawEntry.expandedWidth > 0)
            expandedWidth = rawEntry.expandedWidth
        if (typeof rawEntry.expandedHeight === "number" && rawEntry.expandedHeight > 0)
            expandedHeight = rawEntry.expandedHeight
        if (typeof rawEntry.xRatio === "number")
            xRatio = rawEntry.xRatio
        if (typeof rawEntry.yRatio === "number")
            yRatio = rawEntry.yRatio
        collapsed = rawEntry.collapsed === true
    }

    return {
        x: x,
        y: y,
        expandedWidth: expandedWidth,
        expandedHeight: expandedHeight,
        xRatio: clamp(xRatio, 0, 1),
        yRatio: clamp(yRatio, 0, 1),
        collapsed: collapsed
    }
}

function sanitizeFullscreenPopupConfig() {
    if (!layoutTree) {
        if (Object.keys(fullscreenPopupSourceByHost).length > 0 || Object.keys(fullscreenPopupStateByHost).length > 0) {
            fullscreenPopupSourceByHost = ({})
            fullscreenPopupStateByHost = ({})
            saveFullscreenPopupState()
        }
        return
    }

    var ids = []
    allLeafIds(layoutTree, ids)

    function hasLeafInLayout(leafId) {
        return ids.indexOf(leafId) !== -1
    }

    var nextSource = {}
    for (var hostKey in fullscreenPopupSourceByHost) {
        if (!Object.prototype.hasOwnProperty.call(fullscreenPopupSourceByHost, hostKey))
            continue

        var hostLeafId = parseLeafId(hostKey)
        var sourceLeafId = parseLeafId(fullscreenPopupSourceByHost[hostKey])
        if (hostLeafId === -1 || sourceLeafId === -1)
            continue
        if (hostLeafId === sourceLeafId)
            continue
        if (!hasLeafInLayout(hostLeafId) || !hasLeafInLayout(sourceLeafId))
            continue

        nextSource[String(hostLeafId)] = sourceLeafId
    }

    var nextState = {}
    for (var stateHostKey in nextSource) {
        if (!Object.prototype.hasOwnProperty.call(nextSource, stateHostKey))
            continue
        var rawState = fullscreenPopupStateByHost[stateHostKey]
        nextState[stateHostKey] = popupNormalizedStateEntry(rawState)
    }

    var sourceChanged = JSON.stringify(fullscreenPopupSourceByHost) !== JSON.stringify(nextSource)
    var stateChanged = JSON.stringify(fullscreenPopupStateByHost) !== JSON.stringify(nextState)

    if (sourceChanged)
        fullscreenPopupSourceByHost = nextSource
    if (stateChanged)
        fullscreenPopupStateByHost = nextState

    if (sourceChanged || stateChanged)
        saveFullscreenPopupState()
    if (sourceChanged)
        updateCurrentLayoutFavoriteState()
}

function popupSourceLeafIdForHost(hostLeafId) {
    var hostId = parseLeafId(hostLeafId)
    if (hostId === -1 || !layoutTree || !hasLeafIdInTree(layoutTree, hostId))
        return -1

    var sourceLeafId = parseLeafId(fullscreenPopupSourceByHost[String(hostId)])
    if (sourceLeafId === -1 || sourceLeafId === hostId)
        return -1
    if (!hasLeafIdInTree(layoutTree, sourceLeafId))
        return -1

    return sourceLeafId
}

function popupCandidateItemsForHost(hostLeafId) {
    var hostId = parseLeafId(hostLeafId)
    if (hostId === -1 || !layoutTree || !hasLeafIdInTree(layoutTree, hostId))
        return []

    var ids = []
    allLeafIds(layoutTree, ids)
    var items = []

    for (var i = 0; i < ids.length; ++i) {
        var leafId = ids[i]
        if (leafId === hostId)
            continue

        var pane = paneByLeafId(layoutTree, leafId)
        if (!pane)
            continue

        items.push({
            leafId: leafId,
            paneId: pane.paneId,
            title: pane.title,
            mode: normalizedPaneMode(pane.mode),
            color: pane.color
        })
    }

    items.sort(function(a, b) {
        return a.paneId - b.paneId
    })
    return items
}

function setPopupSourceForHost(hostLeafId, sourceLeafId) {
    var hostId = parseLeafId(hostLeafId)
    if (hostId === -1 || !layoutTree || !hasLeafIdInTree(layoutTree, hostId))
        return

    var sourceId = parseLeafId(sourceLeafId)
    var key = String(hostId)
    var nextSource = cloneObjectMap(fullscreenPopupSourceByHost)
    var nextState = cloneObjectMap(fullscreenPopupStateByHost)

    var sourceValid = sourceId !== -1
                   && sourceId !== hostId
                   && hasLeafIdInTree(layoutTree, sourceId)

    if (!sourceValid) {
        delete nextSource[key]
        delete nextState[key]
    } else {
        nextSource[key] = sourceId
        nextState[key] = popupNormalizedStateEntry(nextState[key])
    }

    fullscreenPopupSourceByHost = nextSource
    fullscreenPopupStateByHost = nextState
    sanitizeFullscreenPopupConfig()
    saveFullscreenPopupState()
    updateCurrentLayoutFavoriteState()
}

function popupStateForHost(hostLeafId) {
    var hostId = parseLeafId(hostLeafId)
    if (hostId === -1)
        return popupNormalizedStateEntry(null)
    return popupNormalizedStateEntry(fullscreenPopupStateByHost[String(hostId)])
}

function popupPositionForHost(hostLeafId, popupWidth, popupHeight) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return Qt.point(0, 0)

    var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
    var areaHeight = windowHeight > 0 ? windowHeight : workspaceHeight

    var spacing = popupMarginPx
    var minX = spacing
    var minY = spacing
    var maxX = areaWidth - popupWidth - spacing
    var maxY = areaHeight - popupHeight - spacing

    if (maxX < minX) {
        minX = 0
        maxX = Math.max(0, areaWidth - popupWidth)
    }
    if (maxY < minY) {
        minY = 0
        maxY = Math.max(0, areaHeight - popupHeight)
    }

    var state = popupStateForHost(hostId)
    var x
    var y
    if (typeof state.x === "number" && typeof state.y === "number") {
        x = state.x
        y = state.y
    } else {
        x = minX + (maxX - minX) * state.xRatio
        y = minY + (maxY - minY) * state.yRatio
    }
    return Qt.point(clamp(x, minX, maxX), clamp(y, minY, maxY))
}

function setPopupPositionForHost(hostLeafId, x, y, popupWidth, popupHeight) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return

    var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
    var areaHeight = windowHeight > 0 ? windowHeight : workspaceHeight

    var spacing = popupMarginPx
    var minX = spacing
    var minY = spacing
    var maxX = areaWidth - popupWidth - spacing
    var maxY = areaHeight - popupHeight - spacing

    if (maxX < minX) {
        minX = 0
        maxX = Math.max(0, areaWidth - popupWidth)
    }
    if (maxY < minY) {
        minY = 0
        maxY = Math.max(0, areaHeight - popupHeight)
    }

    var clampedX = clamp(x, minX, maxX)
    var clampedY = clamp(y, minY, maxY)
    var xRatio = maxX > minX ? (clampedX - minX) / (maxX - minX) : 0
    var yRatio = maxY > minY ? (clampedY - minY) / (maxY - minY) : 0

    var key = String(hostId)
    var nextState = cloneObjectMap(fullscreenPopupStateByHost)
    var current = popupNormalizedStateEntry(nextState[key])
    current.x = clampedX
    current.y = clampedY
    current.xRatio = clamp(xRatio, 0, 1)
    current.yRatio = clamp(yRatio, 0, 1)
    nextState[key] = current
    fullscreenPopupStateByHost = nextState
    saveFullscreenPopupState()
}

function popupExpandedSizeForHost(hostLeafId, defaultWidth, defaultHeight) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return Qt.size(Math.max(80, defaultWidth), Math.max(80, defaultHeight))

    var state = popupStateForHost(hostId)
    var width = state.expandedWidth !== null ? state.expandedWidth : defaultWidth
    var height = state.expandedHeight !== null ? state.expandedHeight : defaultHeight
    return Qt.size(Math.max(80, width), Math.max(80, height))
}

function setPopupExpandedSizeForHost(hostLeafId, expandedWidth, expandedHeight) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return

    var key = String(hostId)
    var nextState = cloneObjectMap(fullscreenPopupStateByHost)
    var current = popupNormalizedStateEntry(nextState[key])
    current.expandedWidth = Math.max(80, expandedWidth)
    current.expandedHeight = Math.max(80, expandedHeight)
    nextState[key] = current
    fullscreenPopupStateByHost = nextState
    saveFullscreenPopupState()
}

function popupCollapsedForHost(hostLeafId) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return false
    return popupStateForHost(hostId).collapsed === true
}

function setPopupCollapsedForHost(hostLeafId, collapsed) {
    var hostId = parseLeafId(hostLeafId)
    if (popupSourceLeafIdForHost(hostId) === -1)
        return

    var key = String(hostId)
    var nextState = cloneObjectMap(fullscreenPopupStateByHost)
    var current = popupNormalizedStateEntry(nextState[key])
    current.collapsed = collapsed === true
    nextState[key] = current
    fullscreenPopupStateByHost = nextState
    saveFullscreenPopupState()
}

function favoriteLayoutSnapshotFromNode(node, state) {
    if (!node)
        return null

    var snapshotState = state || {
        nextPaneId: 0,
        leafIdToPaneId: {}
    }

    if (node.type === "leaf") {
        snapshotState.nextPaneId += 1
        var paneId = snapshotState.nextPaneId
        var leafId = parseLeafId(node.leafId)
        if (leafId !== -1)
            snapshotState.leafIdToPaneId[String(leafId)] = paneId

        return {
            type: "leaf",
            mode: normalizedPaneMode(node.pane ? node.pane.mode : node.mode),
            paneId: paneId
        }
    }

    if (node.type === "split") {
        var ratio = typeof node.ratio === "number" ? node.ratio : 0.5
        var normalizedRatio = Math.round(clamp(ratio, 0.001, 0.999) * 1000) / 1000
        return {
            type: "split",
            orientation: node.orientation === "horizontal" ? "horizontal" : "vertical",
            ratio: normalizedRatio,
            first: favoriteLayoutSnapshotFromNode(node.first, snapshotState),
            second: favoriteLayoutSnapshotFromNode(node.second, snapshotState)
        }
    }

    return null
}

function isValidFavoriteLayoutSnapshot(node) {
    if (!node || typeof node !== "object")
        return false

    if (node.type === "leaf")
        return typeof node.mode === "string"
            && (node.paneId === undefined || typeof node.paneId === "number")

    if (node.type === "split")
        return (node.orientation === "vertical" || node.orientation === "horizontal")
                && typeof node.ratio === "number"
                && isValidFavoriteLayoutSnapshot(node.first)
                && isValidFavoriteLayoutSnapshot(node.second)

    return false
}

function normalizeFavoriteLayoutSnapshot(node, state) {
    if (!node)
        return null

    var snapshotState = state || { nextPaneId: 0 }

    if (node.type === "leaf") {
        snapshotState.nextPaneId += 1
        return {
            type: "leaf",
            mode: normalizedPaneMode(node.mode),
            paneId: snapshotState.nextPaneId
        }
    }

    if (node.type === "split") {
        var ratio = typeof node.ratio === "number" ? node.ratio : 0.5
        var normalizedRatio = Math.round(clamp(ratio, 0.001, 0.999) * 1000) / 1000
        return {
            type: "split",
            orientation: node.orientation === "horizontal" ? "horizontal" : "vertical",
            ratio: normalizedRatio,
            first: normalizeFavoriteLayoutSnapshot(node.first, snapshotState),
            second: normalizeFavoriteLayoutSnapshot(node.second, snapshotState)
        }
    }

    return null
}

function favoritePopupLinksFromLeafMapping(leafIdToPaneId, sourceByHostMap) {
    if (!leafIdToPaneId || !sourceByHostMap || typeof sourceByHostMap !== "object")
        return []

    var links = []
    var used = {}
    for (var hostKey in sourceByHostMap) {
        if (!Object.prototype.hasOwnProperty.call(sourceByHostMap, hostKey))
            continue

        var hostLeafId = parseLeafId(hostKey)
        var sourceLeafId = parseLeafId(sourceByHostMap[hostKey])
        if (hostLeafId === -1 || sourceLeafId === -1 || hostLeafId === sourceLeafId)
            continue

        var hostPaneId = leafIdToPaneId[String(hostLeafId)]
        var sourcePaneId = leafIdToPaneId[String(sourceLeafId)]
        if (typeof hostPaneId !== "number" || typeof sourcePaneId !== "number")
            continue

        var key = hostPaneId + ":" + sourcePaneId
        if (used[key] === true)
            continue
        used[key] = true

        links.push({
            hostPaneId: hostPaneId,
            sourcePaneId: sourcePaneId
        })
    }

    links.sort(function(a, b) {
        if (a.hostPaneId !== b.hostPaneId)
            return a.hostPaneId - b.hostPaneId
        return a.sourcePaneId - b.sourcePaneId
    })
    return links
}

function collectFavoriteSnapshotPaneIds(node, out) {
    if (!node || !out)
        return

    if (node.type === "leaf") {
        if (typeof node.paneId === "number")
            out.push(Math.round(node.paneId))
        return
    }

    if (node.type === "split") {
        collectFavoriteSnapshotPaneIds(node.first, out)
        collectFavoriteSnapshotPaneIds(node.second, out)
    }
}

function normalizeFavoritePopupLinks(popupLinks, layoutSnapshot) {
    var paneIds = []
    collectFavoriteSnapshotPaneIds(layoutSnapshot, paneIds)

    var validPaneIds = {}
    for (var i = 0; i < paneIds.length; ++i) {
        if (paneIds[i] > 0)
            validPaneIds[String(paneIds[i])] = true
    }

    if (!Array.isArray(popupLinks))
        return []

    var links = []
    var used = {}
    for (var j = 0; j < popupLinks.length; ++j) {
        var link = popupLinks[j]
        if (!link || typeof link !== "object")
            continue

        var hostPaneId = parseLeafId(link.hostPaneId)
        var sourcePaneId = parseLeafId(link.sourcePaneId)
        if (hostPaneId === -1 || sourcePaneId === -1 || hostPaneId === sourcePaneId)
            continue
        if (validPaneIds[String(hostPaneId)] !== true || validPaneIds[String(sourcePaneId)] !== true)
            continue

        var linkKey = hostPaneId + ":" + sourcePaneId
        if (used[linkKey] === true)
            continue
        used[linkKey] = true

        links.push({
            hostPaneId: hostPaneId,
            sourcePaneId: sourcePaneId
        })
    }

    links.sort(function(a, b) {
        if (a.hostPaneId !== b.hostPaneId)
            return a.hostPaneId - b.hostPaneId
        return a.sourcePaneId - b.sourcePaneId
    })
    return links
}

function normalizeFavoriteLayoutEntry(rawEntry) {
    if (!rawEntry || typeof rawEntry !== "object")
        return null

    var rawSnapshot = rawEntry.layout !== undefined ? rawEntry.layout : rawEntry
    if (!isValidFavoriteLayoutSnapshot(rawSnapshot))
        return null

    var normalizedLayout = normalizeFavoriteLayoutSnapshot(rawSnapshot)
    if (!normalizedLayout)
        return null

    var normalizedLinks = normalizeFavoritePopupLinks(rawEntry.popupLinks, normalizedLayout)
    return {
        layout: normalizedLayout,
        popupLinks: normalizedLinks
    }
}

function favoriteLayoutEntryFromCurrent() {
    if (!layoutTree)
        return null

    var snapshotState = {
        nextPaneId: 0,
        leafIdToPaneId: {}
    }
    var layoutSnapshot = favoriteLayoutSnapshotFromNode(layoutTree, snapshotState)
    if (!layoutSnapshot)
        return null

    return {
        layout: layoutSnapshot,
        popupLinks: favoritePopupLinksFromLeafMapping(snapshotState.leafIdToPaneId, fullscreenPopupSourceByHost)
    }
}

function favoriteLayoutSignatureFromEntry(entry) {
    if (!entry || !entry.layout)
        return ""
    return JSON.stringify({
        layout: entry.layout,
        popupLinks: Array.isArray(entry.popupLinks) ? entry.popupLinks : []
    })
}

function currentFavoriteLayoutSignature() {
    return favoriteLayoutSignatureFromEntry(favoriteLayoutEntryFromCurrent())
}

function favoriteLayoutIndexBySignature(signature) {
    if (typeof signature !== "string" || signature === "")
        return -1

    for (var i = 0; i < favoriteLayouts.length; ++i) {
        if (favoriteLayoutSignatureFromEntry(favoriteLayouts[i]) === signature)
            return i
    }
    return -1
}

function captureEdgeResizeFavoriteSignature() {
    edgeResizeFavoriteSignatureBefore = ""
    if (!layoutTree || favoriteLayouts.length === 0)
        return

    var currentSignature = currentFavoriteLayoutSignature()
    if (favoriteLayoutIndexBySignature(currentSignature) === -1)
        return

    edgeResizeFavoriteSignatureBefore = currentSignature
}

function replaceFavoriteLayoutBySignature(previousSignature) {
    if (typeof previousSignature !== "string" || previousSignature === "")
        return false

    var index = favoriteLayoutIndexBySignature(previousSignature)
    if (index === -1)
        return false

    var replacement = favoriteLayoutEntryFromCurrent()
    if (!replacement)
        return false

    var replacementSignature = favoriteLayoutSignatureFromEntry(replacement)
    if (replacementSignature === "")
        return false

    var nextFavorites = []
    var seen = {}
    var replaced = false
    for (var i = 0; i < favoriteLayouts.length; ++i) {
        var entry = favoriteLayouts[i]
        var signature = favoriteLayoutSignatureFromEntry(entry)

        if (i === index) {
            entry = replacement
            signature = replacementSignature
            replaced = true
        }

        if (signature === "" || seen[signature] === true)
            continue

        seen[signature] = true
        nextFavorites.push(entry)
    }

    if (!replaced)
        return false

    favoriteLayouts = nextFavorites
    saveFavoriteLayoutsState()
    updateCurrentLayoutFavoriteState()
    return true
}

function saveFavoriteLayoutsState() {
    layoutStore.favoriteLayoutsJson = JSON.stringify(favoriteLayouts)
}

function normalizedSettingsGroupKey(key) {
    return typeof key === "string" ? key.trim() : ""
}

function saveSettingsGroupsState() {
    layoutStore.settingsGroupExpandedJson = JSON.stringify(settingsGroupExpandedMap)
}

function loadSettingsGroupsState() {
    var parsed = {}
    if (layoutStore.settingsGroupExpandedJson && layoutStore.settingsGroupExpandedJson !== "") {
        try {
            parsed = JSON.parse(layoutStore.settingsGroupExpandedJson)
        } catch (error) {
            parsed = {}
        }
    }

    var nextMap = {}
    if (parsed && typeof parsed === "object" && !Array.isArray(parsed)) {
        for (var key in parsed) {
            if (!Object.prototype.hasOwnProperty.call(parsed, key))
                continue
            if (parsed[key] === true)
                nextMap[key] = true
        }
    }

    settingsGroupExpandedMap = nextMap
    saveSettingsGroupsState()
}

function isSettingsGroupExpanded(groupKey) {
    var key = normalizedSettingsGroupKey(groupKey)
    if (key === "")
        return false
    return settingsGroupExpandedMap[key] === true
}

function setSettingsGroupExpanded(groupKey, expanded) {
    var key = normalizedSettingsGroupKey(groupKey)
    if (key === "")
        return

    var nextValue = expanded === true
    var currentlyExpanded = isSettingsGroupExpanded(key)
    if (currentlyExpanded === nextValue)
        return

    var nextMap = {}
    for (var existingKey in settingsGroupExpandedMap) {
        if (!Object.prototype.hasOwnProperty.call(settingsGroupExpandedMap, existingKey))
            continue
        if (existingKey !== key && settingsGroupExpandedMap[existingKey] === true)
            nextMap[existingKey] = true
    }

    if (nextValue)
        nextMap[key] = true

    settingsGroupExpandedMap = nextMap
    saveSettingsGroupsState()
}

function loadFavoriteLayoutsState() {
    var parsed = []
    if (layoutStore.favoriteLayoutsJson && layoutStore.favoriteLayoutsJson !== "") {
        try {
            parsed = JSON.parse(layoutStore.favoriteLayoutsJson)
        } catch (error) {
            parsed = []
        }
    }

    var nextFavorites = []
    if (Array.isArray(parsed)) {
        for (var i = 0; i < parsed.length; ++i) {
            var normalized = normalizeFavoriteLayoutEntry(parsed[i])
            if (normalized)
                nextFavorites.push(normalized)
        }
    }

    favoriteLayouts = nextFavorites
    saveFavoriteLayoutsState()
    updateCurrentLayoutFavoriteState()
}

function updateCurrentLayoutFavoriteState() {
    if (!layoutTree || favoriteLayouts.length === 0) {
        currentLayoutFavoriteSignature = ""
        currentLayoutIsFavorite = false
        return
    }

    var currentSignature = currentFavoriteLayoutSignature()
    currentLayoutFavoriteSignature = currentSignature
    var isFavorite = false
    for (var i = 0; i < favoriteLayouts.length; ++i) {
        if (favoriteLayoutSignatureFromEntry(favoriteLayouts[i]) === currentSignature) {
            isFavorite = true
            break
        }
    }
    currentLayoutIsFavorite = isFavorite
}

function favoriteLayoutIsCurrent(favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= favoriteLayouts.length)
        return false
    if (currentLayoutFavoriteSignature === "")
        return false

    return favoriteLayoutSignatureFromEntry(favoriteLayouts[favoriteIndex]) === currentLayoutFavoriteSignature
}

function addCurrentLayoutToFavorites() {
    var entry = favoriteLayoutEntryFromCurrent()
    if (!entry)
        return

    var currentSignature = favoriteLayoutSignatureFromEntry(entry)
    for (var i = 0; i < favoriteLayouts.length; ++i) {
        if (favoriteLayoutSignatureFromEntry(favoriteLayouts[i]) === currentSignature) {
            currentLayoutIsFavorite = true
            return
        }
    }

    favoriteLayouts = favoriteLayouts.concat([entry])
    saveFavoriteLayoutsState()
    updateCurrentLayoutFavoriteState()
}

function removeCurrentLayoutFromFavorites() {
    if (!layoutTree || favoriteLayouts.length === 0)
        return

    var currentSignature = currentFavoriteLayoutSignature()
    var nextFavorites = []
    for (var i = 0; i < favoriteLayouts.length; ++i) {
        if (favoriteLayoutSignatureFromEntry(favoriteLayouts[i]) !== currentSignature)
            nextFavorites.push(favoriteLayouts[i])
    }

    favoriteLayouts = nextFavorites
    saveFavoriteLayoutsState()
    updateCurrentLayoutFavoriteState()
}

function toggleCurrentLayoutFavorite() {
    if (currentLayoutIsFavorite)
        removeCurrentLayoutFromFavorites()
    else
        addCurrentLayoutToFavorites()
}

function openModeSettingsForLeaf(leafId) {
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane)
        return

    modeSettingsLeafId = leafId
    modeSettingsMode = normalizedPaneMode(pane.mode)
    settingsPanelOpen = false
    modeSettingsPanelOpen = true
}

function paneNumberByLeafId(leafId) {
    var pane = paneByLeafId(layoutTree, leafId)
    return pane ? pane.paneId : -1
}

function paneColorByLeafId(leafId) {
    var pane = paneByLeafId(layoutTree, leafId)
    return pane ? pane.color : "transparent"
}

function synchronizeModeSettingsPanel() {
    if (!modeSettingsPanelOpen)
        return

    var pane = paneByLeafId(layoutTree, modeSettingsLeafId)
    if (!pane) {
        closeModeSettingsPanel()
        return
    }

    modeSettingsMode = normalizedPaneMode(pane.mode)
}

function clamp(v, minV, maxV) {
    return Math.max(minV, Math.min(maxV, v))
}

function paletteColor(index) {
    return Rules.paletteColor(index)
}

function allLeafPanes(node, out) {
    Tree.allLeafPanes(node, out)
}

function allLeafIds(node, out) {
    Tree.allLeafIds(node, out)
}

function firstLeafId() {
    var ids = []
    allLeafIds(layoutTree, ids)
    return ids.length > 0 ? ids[0] : -1
}

function maxLeafIdInTree(node) {
    return Tree.maxLeafIdInTree(node)
}

function normalizedSettingsSide(value) {
    return Rules.normalizedSettingsSide(value)
}

function normalizedPaneMode(value) {
    return Rules.normalizedPaneMode(value)
}

function normalizedPaneRotate3D(value) {
    return Rules.normalizedPaneRotate3D(value)
}

function normalizedPaneRotate2D(value) {
    return Rules.normalizedPaneRotate2D(value)
}

function paneWithMode(paneObj, mode) {
    return Rules.paneWithMode(paneObj, mode)
}

function paneRotate3DByLeafId(leafId) {
    var pane = paneByLeafId(layoutTree, leafId)
    return pane ? normalizedPaneRotate3D(pane.rotate3DLogoOnSphere) : false
}

function paneRotate2DByLeafId(leafId) {
    var pane = paneByLeafId(layoutTree, leafId)
    return pane ? normalizedPaneRotate2D(pane.rotate2DLogoHorizontal) : false
}

function paneRotateEnabledByLeafId(leafId) {
    return paneRotate3DByLeafId(leafId) || paneRotate2DByLeafId(leafId)
}

function setPaneRotate3DByLeafId(leafId, enabled) {
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane)
        return

    var nextValue = normalizedPaneRotate3D(enabled)
    if (normalizedPaneRotate3D(pane.rotate3DLogoOnSphere) === nextValue)
        return

    layoutTree = updatePaneInLeaf(layoutTree, leafId, {
        paneId: pane.paneId,
        title: pane.title,
        color: pane.color,
        mode: normalizedPaneMode(pane.mode),
        rotate3DLogoOnSphere: nextValue,
        rotate2DLogoHorizontal: normalizedPaneRotate2D(pane.rotate2DLogoHorizontal)
    })
}

function setPaneRotate2DByLeafId(leafId, enabled) {
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane)
        return

    var nextValue = normalizedPaneRotate2D(enabled)
    if (normalizedPaneRotate2D(pane.rotate2DLogoHorizontal) === nextValue)
        return

    layoutTree = updatePaneInLeaf(layoutTree, leafId, {
        paneId: pane.paneId,
        title: pane.title,
        color: pane.color,
        mode: normalizedPaneMode(pane.mode),
        rotate3DLogoOnSphere: normalizedPaneRotate3D(pane.rotate3DLogoOnSphere),
        rotate2DLogoHorizontal: nextValue
    })
}

function setPaneRotateEnabledByLeafId(leafId, enabled) {
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane)
        return

    var nextValue = enabled === true
    if (paneRotateEnabledByLeafId(leafId) === nextValue)
        return

    layoutTree = updatePaneInLeaf(layoutTree, leafId, {
        paneId: pane.paneId,
        title: pane.title,
        color: pane.color,
        mode: normalizedPaneMode(pane.mode),
        rotate3DLogoOnSphere: nextValue,
        rotate2DLogoHorizontal: nextValue
    })
}

function leafIdByPaneNumber(node, paneNumber) {
    return Tree.leafIdByPaneNumber(node, paneNumber)
}

function firstLeafIdByMode(node, mode) {
    return Tree.firstLeafIdByMode(node, mode)
}

function layoutHasAnyModeField(node) {
    return Rules.layoutHasAnyModeField(node)
}

function normalizeAndFixPaneModes(tree, setDefault3D) {
    var ids = []
    allLeafIds(tree, ids)

    var nextTree = tree
    var threeDLeafIds = []
    for (var i = 0; i < ids.length; ++i) {
        var leafId = ids[i]
        var pane = paneByLeafId(nextTree, leafId)
        if (!pane)
            continue

        var mode = normalizedPaneMode(pane.mode)
        var rotate3D = normalizedPaneRotate3D(pane.rotate3DLogoOnSphere)
        var rotate2D = normalizedPaneRotate2D(pane.rotate2DLogoHorizontal)
        if (pane.mode !== mode
                || pane.rotate3DLogoOnSphere !== rotate3D
                || pane.rotate2DLogoHorizontal !== rotate2D)
            nextTree = updatePaneInLeaf(nextTree, leafId, paneWithMode(pane, mode))

        if (mode === "3D")
            threeDLeafIds.push(leafId)
    }

    if (threeDLeafIds.length > 1) {
        for (var j = 1; j < threeDLeafIds.length; ++j) {
            var moveOutPane = paneByLeafId(nextTree, threeDLeafIds[j])
            if (moveOutPane)
                nextTree = updatePaneInLeaf(nextTree, threeDLeafIds[j], paneWithMode(moveOutPane, "2D"))
        }
        threeDLeafIds = [threeDLeafIds[0]]
    }

    if (setDefault3D && threeDLeafIds.length === 0) {
        var targetLeaf = leafIdByPaneNumber(nextTree, 1)
        if (targetLeaf === -1 && ids.length > 0)
            targetLeaf = ids[0]

        if (targetLeaf !== -1) {
            var targetPane = paneByLeafId(nextTree, targetLeaf)
            if (targetPane)
                nextTree = updatePaneInLeaf(nextTree, targetLeaf, paneWithMode(targetPane, "3D"))
        }
    }

    return nextTree
}

function maxSplitIdInTree(node) {
    return Tree.maxSplitIdInTree(node)
}

function hasLeafIdInTree(node, leafId) {
    return Tree.hasLeafIdInTree(node, leafId)
}

function isValidLayoutNode(node) {
    return Rules.isValidLayoutNode(node)
}

function saveLayoutState() {
    if (!layoutTree)
        return

    saveFullscreenPopupState()
    saveGlobalPopupPreferences()
    layoutStore.layoutJson = JSON.stringify(layoutTree)
    layoutStore.nextLeafSerialStored = Math.max(nextLeafSerial, maxLeafIdInTree(layoutTree))
    layoutStore.nextSplitSerialStored = Math.max(nextSplitSerial, maxSplitIdInTree(layoutTree))
    layoutStore.activeLeafIdStored = activeLeafId
    layoutStore.windowWidthStored = Math.round(windowWidth)
    layoutStore.windowHeightStored = Math.round(windowHeight)
    layoutStore.settingsPushContentStored = settingsPushContent
    layoutStore.settingsSideStored = settingsSide
    layoutStore.quickActionFavoritesEnabledStored = quickActionFavoritesEnabled
    layoutStore.quickActionMarkerEnabledStored = quickActionMarkerEnabled
    layoutStore.quickActionConnectionStatusEnabledStored = quickActionConnectionStatusEnabled
    layoutStore.selectedConnectionFilePathStored = selectedConnectionFilePath
}

function restoreLayoutState() {
    if (!layoutStore.layoutJson || layoutStore.layoutJson === "")
        return false

    var parsed
    try {
        parsed = JSON.parse(layoutStore.layoutJson)
    } catch (error) {
        return false
    }

    if (!isValidLayoutNode(parsed))
        return false

    if (layoutStore.windowWidthStored > 0 && layoutStore.windowHeightStored > 0)
        windowGeometryRestoreRequested(layoutStore.windowWidthStored, layoutStore.windowHeightStored)

    var hadModeField = layoutHasAnyModeField(parsed)
    layoutTree = normalizeAndFixPaneModes(parsed, !hadModeField)
    layoutTree = renumberPanes(layoutTree)
    layoutTree = ensureContentIds(layoutTree)
    nextLeafSerial = Math.max(layoutStore.nextLeafSerialStored, maxLeafIdInTree(layoutTree))
    nextSplitSerial = Math.max(layoutStore.nextSplitSerialStored, maxSplitIdInTree(layoutTree))
    activeLeafId = hasLeafIdInTree(layoutTree, layoutStore.activeLeafIdStored) ? layoutStore.activeLeafIdStored : firstLeafId()
    settingsPushContent = layoutStore.settingsPushContentStored
    settingsSide = normalizedSettingsSide(layoutStore.settingsSideStored)
    quickActionFavoritesEnabled = layoutStore.quickActionFavoritesEnabledStored
    quickActionMarkerEnabled = layoutStore.quickActionMarkerEnabledStored
    quickActionConnectionStatusEnabled = layoutStore.quickActionConnectionStatusEnabledStored
    selectedConnectionFilePath = layoutStore.selectedConnectionFilePathStored
    sanitizeFullscreenPopupConfig()
    maximizedLeafId = -1
    clearModePickerSelection()
    closeModeSettingsPanel()
    rebuildLayoutCaches()
    return true
}

function resetWindowConfiguration() {
    dragActive = false
    draggedLeafId = -1
    dropTargetLeafId = -1
    maximizedLeafId = -1
    clearModePickerSelection()
    closeModeSettingsPanel()
    clearEdgeResizeState()

    nextLeafSerial = 0
    nextSplitSerial = 0
    fullscreenPopupSourceByHost = ({})
    fullscreenPopupStateByHost = ({})
    saveFullscreenPopupState()
    var firstLeaf = makeLeaf(makePane(1, "3D"))
    layoutTree = firstLeaf
    activeLeafId = firstLeaf.leafId

    rebuildLayoutCaches()
    saveLayoutState()
}

function buildPresetTree(presetId) {
    if (presetId === 1) {
        var p1TopLeft = makeLeaf(makePane(1))
        var p1TopRight = makeLeaf(makePane(2))
        var p1BottomWide = makeLeaf(makePane(3))
        var p1Top = makeSplit("vertical", p1TopLeft, p1TopRight, 0.5)
        return makeSplit("horizontal", p1Top, p1BottomWide, 0.5)
    }

    if (presetId === 2) {
        var p2TopLeft = makeLeaf(makePane(1))
        var p2TopRight = makeLeaf(makePane(2))
        var p2BottomLeft = makeLeaf(makePane(3))
        var p2BottomRight = makeLeaf(makePane(4))
        var p2LeftCol = makeSplit("horizontal", p2TopLeft, p2BottomLeft, 0.5)
        var p2RightCol = makeSplit("horizontal", p2TopRight, p2BottomRight, 0.5)
        return makeSplit("vertical", p2LeftCol, p2RightCol, 0.5)
    }

    if (presetId === 3) {
        var p3TopWide = makeLeaf(makePane(1))
        var p3BottomLeft = makeLeaf(makePane(2))
        var p3BottomRight = makeLeaf(makePane(3))
        var p3Bottom = makeSplit("vertical", p3BottomLeft, p3BottomRight, 0.5)
        return makeSplit("horizontal", p3TopWide, p3Bottom, 0.5)
    }

    return null
}

function applyLayoutPreset(presetId) {
    dragActive = false
    draggedLeafId = -1
    dropTargetLeafId = -1
    maximizedLeafId = -1
    clearModePickerSelection()
    clearEdgeResizeState()
    closeModeSettingsPanel()

    nextLeafSerial = 0
    nextSplitSerial = 0
    fullscreenPopupSourceByHost = ({})
    fullscreenPopupStateByHost = ({})
    saveFullscreenPopupState()

    var tree = buildPresetTree(presetId)
    if (!tree)
        return

    layoutTree = renumberPanes(tree)
    activeLeafId = firstLeafId()

    var ids = []
    allLeafIds(layoutTree, ids)
    setModePickerLeafIds(ids)

    rebuildLayoutCaches()
    saveLayoutState()
}

function buildTreeFromFavoriteSnapshot(snapshot, state) {
    if (!snapshot || typeof snapshot !== "object")
        return null

    if (snapshot.type === "leaf") {
        state.paneNumber += 1
        return makeLeaf(makePane(state.paneNumber, normalizedPaneMode(snapshot.mode)))
    }

    if (snapshot.type === "split") {
        var first = buildTreeFromFavoriteSnapshot(snapshot.first, state)
        var second = buildTreeFromFavoriteSnapshot(snapshot.second, state)
        if (!first || !second)
            return null

        var orientation = snapshot.orientation === "horizontal" ? "horizontal" : "vertical"
        var ratio = typeof snapshot.ratio === "number" ? snapshot.ratio : 0.5
        return makeSplit(orientation, first, second, ratio)
    }

    return null
}

function favoriteLayoutSnapshotFromEntry(entry) {
    if (!entry || typeof entry !== "object")
        return null

    if (entry.layout !== undefined)
        return entry.layout

    // Backward compatibility with legacy records stored as plain snapshot trees.
    return entry
}

function popupSourceMapFromFavoriteEntry(entry, targetTree) {
    var nextSource = {}
    if (!entry || !targetTree || !Array.isArray(entry.popupLinks))
        return nextSource

    for (var i = 0; i < entry.popupLinks.length; ++i) {
        var link = entry.popupLinks[i]
        if (!link || typeof link !== "object")
            continue

        var hostPaneId = parseLeafId(link.hostPaneId)
        var sourcePaneId = parseLeafId(link.sourcePaneId)
        if (hostPaneId === -1 || sourcePaneId === -1 || hostPaneId === sourcePaneId)
            continue

        var hostLeafId = leafIdByPaneNumber(targetTree, hostPaneId)
        var sourceLeafId = leafIdByPaneNumber(targetTree, sourcePaneId)
        if (hostLeafId === -1 || sourceLeafId === -1 || hostLeafId === sourceLeafId)
            continue

        nextSource[String(hostLeafId)] = sourceLeafId
    }
    return nextSource
}

function applyFavoriteLayout(favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= favoriteLayouts.length)
        return

    var entry = favoriteLayouts[favoriteIndex]
    var snapshot = favoriteLayoutSnapshotFromEntry(entry)
    if (!snapshot)
        return

    dragActive = false
    draggedLeafId = -1
    dropTargetLeafId = -1
    maximizedLeafId = -1
    clearModePickerSelection()
    clearEdgeResizeState()
    closeModeSettingsPanel()

    nextLeafSerial = 0
    nextSplitSerial = 0

    var buildState = { paneNumber: 0 }
    var tree = buildTreeFromFavoriteSnapshot(snapshot, buildState)
    if (!tree)
        return

    var normalizedTree = normalizeAndFixPaneModes(renumberPanes(tree), false)
    layoutTree = normalizedTree
    fullscreenPopupSourceByHost = popupSourceMapFromFavoriteEntry(entry, normalizedTree)
    fullscreenPopupStateByHost = ({})
    sanitizeFullscreenPopupConfig()
    saveFullscreenPopupState()
    activeLeafId = firstLeafId()
    rebuildLayoutCaches()
    saveLayoutState()
    updateCurrentLayoutFavoriteState()
}

function removeFavoriteLayoutAt(favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= favoriteLayouts.length)
        return

    var nextFavorites = []
    for (var i = 0; i < favoriteLayouts.length; ++i) {
        if (i !== favoriteIndex)
            nextFavorites.push(favoriteLayouts[i])
    }

    favoriteLayouts = nextFavorites
    saveFavoriteLayoutsState()
    updateCurrentLayoutFavoriteState()
}

function leafCount() {
    var ids = []
    allLeafIds(layoutTree, ids)
    return ids.length
}

function nextPaneNumber() {
    var panes = []
    allLeafPanes(layoutTree, panes)

    var used = {}
    for (var i = 0; i < panes.length; ++i)
        used[panes[i].paneId] = true

    var candidate = 1
    while (used[candidate])
        candidate++

    return candidate
}

function renumberPanes(tree) {
    if (!tree)
        return tree

    var leafIds = []
    allLeafIds(tree, leafIds)

    var ordered = []
    for (var i = 0; i < leafIds.length; ++i) {
        var pane = paneByLeafId(tree, leafIds[i])
        if (!pane)
            continue
        ordered.push({
            leafId: leafIds[i],
            paneId: pane.paneId
        })
    }

    ordered.sort(function(a, b) {
        return a.paneId - b.paneId
    })

    var nextTree = tree
    for (var j = 0; j < ordered.length; ++j) {
        var leafId = ordered[j].leafId
        var targetPaneId = j + 1
        var paneObj = paneByLeafId(nextTree, leafId)
        if (!paneObj)
            continue

        var targetTitle = "Pane " + targetPaneId
        var targetColor = paletteColor(targetPaneId - 1)
        if (paneObj.paneId === targetPaneId && paneObj.title === targetTitle && paneObj.color === targetColor)
            continue

        nextTree = updatePaneInLeaf(nextTree, leafId, {
            paneId: targetPaneId,
            title: targetTitle,
            color: targetColor,
            mode: normalizedPaneMode(paneObj.mode),
            rotate3DLogoOnSphere: normalizedPaneRotate3D(paneObj.rotate3DLogoOnSphere),
            rotate2DLogoHorizontal: normalizedPaneRotate2D(paneObj.rotate2DLogoHorizontal),
            contentId: paneObj.contentId || ""
        })
    }

    return nextTree
}

function makePane(paneNumber, mode) {
    return Rules.makePane(paneNumber, mode)
}

function makeLeaf(paneObj) {
    nextLeafSerial += 1
    var pane = paneObj.contentId ? paneObj : {
        paneId: paneObj.paneId,
        title: paneObj.title,
        color: paneObj.color,
        mode: paneObj.mode,
        rotate3DLogoOnSphere: paneObj.rotate3DLogoOnSphere,
        rotate2DLogoHorizontal: paneObj.rotate2DLogoHorizontal,
        contentId: appUtils.generateUuid()
    }
    return {
        type: "leaf",
        leafId: nextLeafSerial,
        pane: pane
    }
}

function makeSplit(orientation, firstNode, secondNode, ratio) {
    nextSplitSerial += 1
    return {
        type: "split",
        splitId: nextSplitSerial,
        orientation: orientation,
        ratio: clamp(ratio, 0.2, 0.8),
        first: firstNode,
        second: secondNode
    }
}

function splitRectByHandle(node, x, y, w, h, outRects, outHandles) {
    Resize.splitRectByHandle(node, x, y, w, h, outRects, outHandles, splitterThickness)
}

function syncSlotContentIds(newRects) {
    var next = slotContentIds.slice()
    while (next.length < maxPaneCount) next.push("")

    // Keep existing slot assignments that still have a live contentId in newRects.
    var liveIds = {}
    for (var i = 0; i < newRects.length; ++i) {
        var cid = newRects[i].pane && newRects[i].pane.contentId
        if (cid) liveIds[cid] = true
    }
    for (var s = 0; s < next.length; ++s) {
        if (next[s] && !liveIds[next[s]])
            next[s] = ""
    }

    // Assign any new contentIds that have no slot yet.
    for (var r = 0; r < newRects.length; ++r) {
        var rcid = newRects[r].pane && newRects[r].pane.contentId
        if (!rcid) continue
        var already = false
        for (var ss = 0; ss < next.length; ++ss) {
            if (next[ss] === rcid) { already = true; break }
        }
        if (!already) {
            for (var fs = 0; fs < next.length; ++fs) {
                if (!next[fs]) { next[fs] = rcid; break }
            }
        }
    }

    slotContentIds = next
}

function ensureContentIds(node) {
    if (!node) return node
    if (node.type === "leaf") {
        if (!node.pane || node.pane.contentId)
            return node
        var pane = {
            paneId: node.pane.paneId,
            title: node.pane.title,
            color: node.pane.color,
            mode: node.pane.mode,
            rotate3DLogoOnSphere: node.pane.rotate3DLogoOnSphere,
            rotate2DLogoHorizontal: node.pane.rotate2DLogoHorizontal,
            contentId: appUtils.generateUuid()
        }
        return { type: "leaf", leafId: node.leafId, pane: pane }
    }
    if (node.type === "split") {
        return {
            type: "split",
            splitId: node.splitId,
            orientation: node.orientation,
            ratio: node.ratio,
            first: ensureContentIds(node.first),
            second: ensureContentIds(node.second)
        }
    }
    return node
}

function rebuildLayoutCaches(updateHandles) {
    if (updateHandles === undefined)
        updateHandles = true

    if (!layoutTree) {
        leafRects = []
        splitHandles = []
        leafRectModel.clear()
        splitHandleModel.clear()
        closeModeSettingsPanel()
        return
    }

    var newRects = []
    var newHandles = []
    splitRectByHandle(layoutTree, 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), newRects, newHandles)
    leafRects = newRects
    syncRectModel(leafRectModel, "rectData", newRects, "leafId")
    syncSlotContentIds(newRects)

    if (updateHandles)
        splitHandles = newHandles
    if (updateHandles)
        syncRectModel(splitHandleModel, "handleData", newHandles, "splitId")

    synchronizeModeSettingsPanel()

    if (dragActive)
        updateDropTargetByCursor()
}

function findLeafRect(leafId) {
    for (var i = 0; i < leafRects.length; ++i) {
        if (leafRects[i].leafId === leafId)
            return leafRects[i]
    }
    return null
}

function replaceLeaf(node, leafId, replacementNode) {
    return Tree.replaceLeaf(node, leafId, replacementNode)
}

function removeLeafFromNode(node, leafId) {
    return Tree.removeLeafFromNode(node, leafId)
}

function updateSplitRatio(node, splitId, ratio) {
    return Tree.updateSplitRatio(node, splitId, ratio)
}

function splitRatioById(node, splitId) {
    return Tree.splitRatioById(node, splitId)
}

function updatePaneInLeaf(node, leafId, paneObj) {
    return Tree.updatePaneInLeaf(node, leafId, paneObj)
}

function paneByLeafId(node, leafId) {
    return Tree.paneByLeafId(node, leafId)
}

function findPathToLeaf(node, leafId, path) {
    return Tree.findPathToLeaf(node, leafId, path)
}

function splitNodeById(node, splitId) {
    return Tree.splitNodeById(node, splitId)
}

function firstAxisSplitFromBoundary(node, axis, boundarySide) {
    return Tree.firstAxisSplitFromBoundary(node, axis, boundarySide)
}

function collectLeafIds(node, out) {
    Tree.collectLeafIds(node, out)
}

function boundaryLeafForSplit(splitId, side) {
    var splitNode = splitNodeById(layoutTree, splitId)
    if (!splitNode)
        return -1
    var orientation = splitNode.orientation
    var subtree = side === "first" ? splitNode.first : splitNode.second
    var ids = []
    collectLeafIds(subtree, ids)
    if (ids.length === 0)
        return -1

    var chosen = -1
    var metric = side === "second" ? 1e12 : -1e12

    for (var i = 0; i < ids.length; ++i) {
        var rect = findLeafRect(ids[i])
        if (!rect)
            continue

        var value
        if (orientation === "horizontal")
            value = side === "second" ? rect.y : (rect.y + rect.height)
        else
            value = side === "second" ? rect.x : (rect.x + rect.width)

        if (side === "second") {
            if (value < metric) {
                metric = value
                chosen = ids[i]
            }
        } else {
            if (value > metric) {
                metric = value
                chosen = ids[i]
            }
        }
    }

    return chosen
}
function beginResizeForSplitHandle(splitId, orientation, absX, absY) {
    var candidates = []
    if (orientation === "vertical") {
        candidates.push({ leafId: boundaryLeafForSplit(splitId, "second"), edge: "left" })
        candidates.push({ leafId: boundaryLeafForSplit(splitId, "first"), edge: "right" })
    } else {
        candidates.push({ leafId: boundaryLeafForSplit(splitId, "second"), edge: "top" })
        candidates.push({ leafId: boundaryLeafForSplit(splitId, "first"), edge: "bottom" })
    }

    for (var i = 0; i < candidates.length; ++i) {
        var candidate = candidates[i]
        if (candidate.leafId === -1)
            continue

        clearEdgeResizeState()
        if (!beginEdgeResize(candidate.leafId, candidate.edge, absX, absY))
            continue

        // Important: keep dragging the exact split-handle that the user grabbed.
        if (edgeResizeMovingSplitId !== splitId)
            continue

        captureEdgeResizeFavoriteSignature()
        edgeResizeHighlightLeafId = candidate.leafId
        edgeResizeHighlightEdge = candidate.edge
        return true
    }

    clearEdgeResizeState()
    var splitCoord = splitCoordById(splitId)
    if (isNaN(splitCoord))
        return false

    edgeResizeAxis = orientation
    edgeResizeMovingSplitId = splitId
    edgeResizeMovingSide = "first"
    edgeResizeFixedSplitId = -1
    edgeResizeFixedSide = ""
    edgeResizePointerStart = orientation === "vertical" ? absX : absY
    edgeResizeMovingCoordStart = splitCoord
    edgeResizeFixedCoord = 0
    captureEdgeResizeFavoriteSignature()
    return true
}
function edgePlanForLeaf(leafId, edge) {
    var path = []
    if (!findPathToLeaf(layoutTree, leafId, path))
        return null

    var leftEntry = null
    var rightEntry = null
    var topEntry = null
    var bottomEntry = null

    for (var i = path.length - 1; i >= 0; --i) {
        var p = path[i]
        if (p.orientation === "vertical") {
            if (!leftEntry && p.side === "second")
                leftEntry = p
            if (!rightEntry && p.side === "first")
                rightEntry = p
        } else {
            if (!topEntry && p.side === "second")
                topEntry = p
            if (!bottomEntry && p.side === "first")
                bottomEntry = p
        }
    }

    var axis = (edge === "left" || edge === "right") ? "vertical" : "horizontal"
    var moving = null
    var fixed = null

    if (edge === "left") {
        moving = leftEntry
        fixed = rightEntry
    } else if (edge === "right") {
        moving = rightEntry
        fixed = leftEntry
    } else if (edge === "top") {
        moving = topEntry
        fixed = bottomEntry
    } else if (edge === "bottom") {
        moving = bottomEntry
        fixed = topEntry
    }

    if (!moving)
        return null

    if (!fixed) {
        var movingNode = splitNodeById(layoutTree, moving.splitId)
        if (movingNode) {
            var sibling = moving.side === "first" ? movingNode.second : movingNode.first
            var boundarySide = moving.side === "first" ? "start" : "end"
            fixed = firstAxisSplitFromBoundary(sibling, axis, boundarySide)
        }
    }

    return {
        axis: axis,
        moving: moving,
        fixed: fixed
    }
}

function splitGeometryById(node, x, y, w, h, splitId) {
    return Resize.splitGeometryById(node, x, y, w, h, splitId, splitterThickness)
}


function subtreeMinSize(node, axis) {
    return Resize.subtreeMinSize(node, axis, minPaneSize, splitterThickness)
}

function splitCoordLimitsById(splitId) {
    var geo = splitGeometryById(layoutTree, 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), splitId)
    var node = splitNodeById(layoutTree, splitId)
    if (!geo || !node)
        return null

    var axis = node.orientation
    var minFirst = subtreeMinSize(node.first, axis)
    var minSecond = subtreeMinSize(node.second, axis)

    var minCoord = geo.parentStart + minFirst
    var maxCoord = geo.parentStart + geo.parentLength - minSecond - splitterThickness

    if (maxCoord < minCoord) {
        var mid = (minCoord + maxCoord) / 2
        minCoord = mid
        maxCoord = mid
    }

    return {
        min: minCoord,
        max: maxCoord,
        parentStart: geo.parentStart,
        parentLength: geo.parentLength
    }
}

function clampRatioForSplit(splitId, ratio) {
    var limits = splitCoordLimitsById(splitId)
    if (!limits || limits.parentLength <= 0)
        return clamp(ratio, 0.001, 0.999)

    var minRatio = (limits.min - limits.parentStart + splitterThickness / 2) / limits.parentLength
    var maxRatio = (limits.max - limits.parentStart + splitterThickness / 2) / limits.parentLength

    if (maxRatio < minRatio)
        maxRatio = minRatio

    return clamp(ratio, minRatio, maxRatio)
}

function splitCoordById(splitId) {
    var geo = splitGeometryById(layoutTree, 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), splitId)
    var ratio = splitRatioById(layoutTree, splitId)
    if (!geo || ratio < 0)
        return NaN

    return geo.parentStart + geo.parentLength * ratio - splitterThickness / 2
}

function splitCoordForLeafSide(side, edgeCoord) {
    return side === "second" ? edgeCoord - splitterThickness : edgeCoord
}

function snappedSplitCoord(splitId, splitCoord) {
    var limits = splitCoordLimitsById(splitId)
    if (!limits)
        return splitCoord

    var splitNode = splitNodeById(layoutTree, splitId)
    if (!splitNode)
        return splitCoord

    var axisLength = splitNode.orientation === "vertical" ? effectiveWorkspaceWidth() : effectiveWorkspaceHeight()
    if (axisLength <= 0)
        return splitCoord

    var candidates = []
    for (var step = 1; step <= 7; ++step) {
        candidates.push(
            axisLength * (step / 8) - splitterThickness / 2
        )
    }

    var bestCoord = splitCoord
    var bestDistance = splitSnapThresholdPx + 1
    for (var i = 0; i < candidates.length; ++i) {
        var candidate = clamp(candidates[i], limits.min, limits.max)
        var distance = Math.abs(splitCoord - candidate)
        if (distance < bestDistance) {
            bestDistance = distance
            bestCoord = candidate
        }
    }

    return bestDistance <= splitSnapThresholdPx ? bestCoord : splitCoord
}

function setSplitCoordById(splitId, splitCoord, updateHandles) {
    if (updateHandles === undefined)
        updateHandles = false

    var limits = splitCoordLimitsById(splitId)
    if (!limits || limits.parentLength <= 0)
        return

    var clampedCoord = clamp(splitCoord, limits.min, limits.max)
    var ratio = (clampedCoord - limits.parentStart + splitterThickness / 2) / limits.parentLength
    setSplitRatioById(splitId, ratio, updateHandles)
}

function toggleLeafMaximize(leafId) {
    maximizedLeafId = maximizedLeafId === leafId ? -1 : leafId
}

function handleLeafTap(leafId) {
    activeLeafId = leafId
    var now = Date.now()
    if (!editableMode && lastTappedLeafId === leafId && now - lastTapTimestamp <= doubleTapIntervalMs) {
        toggleLeafMaximize(leafId)
        lastTappedLeafId = -1
        lastTapTimestamp = 0
        return
    }
    lastTappedLeafId = leafId
    lastTapTimestamp = now
}

function applyPaneModeSelection(leafId, mode) {
    var targetPane = paneByLeafId(layoutTree, leafId)
    if (!targetPane) {
        removeModePickerLeafId(leafId)
        return
    }

    var nextTree = layoutTree
    var targetMode = normalizedPaneMode(mode)
    var current3DLeaf = firstLeafIdByMode(nextTree, "3D")
    if (targetMode === "3D") {
        if (current3DLeaf !== -1 && current3DLeaf !== leafId)
            return
        if (globalPopupMode === "3D")
            return
    }

    var updatedTargetPane = paneByLeafId(nextTree, leafId)
    if (updatedTargetPane)
        nextTree = updatePaneInLeaf(nextTree, leafId, paneWithMode(updatedTargetPane, targetMode))

    layoutTree = nextTree
    removeModePickerLeafId(leafId)
    rebuildLayoutCaches()
}

function swapLeafPanes(leafA, leafB) {
    if (leafA === leafB)
        return

    var paneA = paneByLeafId(layoutTree, leafA)
    var paneB = paneByLeafId(layoutTree, leafB)
    if (!paneA || !paneB)
        return

    // Swap pane data (including contentId) between the two leaf nodes.
    // Each Plot2D slot tracks its contentId, so it automatically follows
    // the pane to its new host after rebuildLayoutCaches updates slotContentIds.
    var nextTree = updatePaneInLeaf(layoutTree, leafA, paneB)
    nextTree = updatePaneInLeaf(nextTree, leafB, paneA)
    layoutTree = nextTree
    rebuildLayoutCaches()
}
function setSplitRatioById(splitId, ratio, updateHandles) {
    if (updateHandles === undefined)
        updateHandles = true

    var boundedRatio = clampRatioForSplit(splitId, ratio)
    layoutTree = updateSplitRatio(layoutTree, splitId, boundedRatio)
    rebuildLayoutCaches(updateHandles)
}

function beginEdgeResize(leafId, edge, absX, absY) {
    var plan = edgePlanForLeaf(leafId, edge)
    if (!plan || !plan.moving)
        return false

    var rect = findLeafRect(leafId)
    if (!rect)
        return false

    edgeResizeAxis = plan.axis
    edgeResizeMovingSplitId = plan.moving.splitId
    edgeResizeMovingSide = plan.moving.side
    edgeResizeFixedSplitId = plan.fixed ? plan.fixed.splitId : -1
    edgeResizeFixedSide = plan.fixed ? plan.fixed.side : ""

    if (plan.axis === "vertical") {
        edgeResizePointerStart = absX
        edgeResizeMovingCoordStart = edge === "left" ? rect.x : rect.x + rect.width
        edgeResizeFixedCoord = edge === "left" ? rect.x + rect.width : rect.x
    } else {
        edgeResizePointerStart = absY
        edgeResizeMovingCoordStart = edge === "top" ? rect.y : rect.y + rect.height
        edgeResizeFixedCoord = edge === "top" ? rect.y + rect.height : rect.y
    }

    return true
}

function nearestSplitHandleAtPoint(absX, absY, orientation, maxDistancePx) {
    var best = null
    var bestDistance = maxDistancePx + 1

    for (var i = 0; i < splitHandles.length; ++i) {
        var handle = splitHandles[i]
        if (handle.orientation !== orientation)
            continue

        var distance = 1e9
        if (orientation === "vertical") {
            if (absY < handle.y - 8 || absY > handle.y + handle.height + 8)
                continue
            distance = Math.abs(absX - handle.x)
        } else {
            if (absX < handle.x - 8 || absX > handle.x + handle.width + 8)
                continue
            distance = Math.abs(absY - handle.y)
        }

        if (distance < bestDistance) {
            bestDistance = distance
            best = handle
        }
    }

    return bestDistance <= maxDistancePx ? best : null
}

function beginEdgeResizeWithFallback(leafId, edge, absX, absY) {
    edgeResizeHighlightLeafId = -1
    edgeResizeHighlightEdge = ""

    var started = beginEdgeResize(leafId, edge, absX, absY)
    if (!started) {
        var preferredOrientation = (edge === "left" || edge === "right") ? "vertical" : "horizontal"
        var nearestHandle = nearestSplitHandleAtPoint(absX, absY, preferredOrientation, 24)
        if (nearestHandle)
            started = beginResizeForSplitHandle(nearestHandle.splitId, nearestHandle.orientation, absX, absY)
    }

    if (started) {
        captureEdgeResizeFavoriteSignature()
        if (edgeResizeHighlightLeafId === -1 || edgeResizeHighlightEdge === "") {
            edgeResizeHighlightLeafId = leafId
            edgeResizeHighlightEdge = edge
        }
    }

    return started
}

function updateEdgeResize(absX, absY) {
    if (edgeResizeMovingSplitId < 0)
        return

    var pointer = edgeResizeAxis === "vertical" ? absX : absY
    var delta = pointer - edgeResizePointerStart
    var movingCoord = edgeResizeMovingCoordStart + delta

    var minSize = minPaneSize
    if (edgeResizeFixedSplitId >= 0) {
        if (edgeResizeMovingSide === "second")
            movingCoord = Math.min(movingCoord, edgeResizeFixedCoord - minSize)
        else
            movingCoord = Math.max(movingCoord, edgeResizeFixedCoord + minSize)
    } else {
        if (edgeResizeAxis === "vertical")
            movingCoord = clamp(movingCoord, 0, effectiveWorkspaceWidth())
        else
            movingCoord = clamp(movingCoord, 0, effectiveWorkspaceHeight())
    }

    var prevTree = layoutTree

    var movingSplitCoord = splitCoordForLeafSide(edgeResizeMovingSide, movingCoord)
    movingSplitCoord = snappedSplitCoord(edgeResizeMovingSplitId, movingSplitCoord)
    setSplitCoordById(edgeResizeMovingSplitId, movingSplitCoord, false)

    if (edgeResizeFixedSplitId >= 0) {
        var desiredFixedSplitCoord = splitCoordForLeafSide(edgeResizeFixedSide, edgeResizeFixedCoord)
        setSplitCoordById(edgeResizeFixedSplitId, desiredFixedSplitCoord, false)

        var actualFixedSplitCoord = splitCoordById(edgeResizeFixedSplitId)
        if (isNaN(actualFixedSplitCoord) || Math.abs(actualFixedSplitCoord - desiredFixedSplitCoord) > 0.5) {
            layoutTree = prevTree
            rebuildLayoutCaches(false)
        }
    }

}

function clearEdgeResizeState() {
    edgeResizeMovingSplitId = -1
    edgeResizeMovingSide = ""
    edgeResizeFixedSplitId = -1
    edgeResizeFixedSide = ""
    edgeResizeAxis = ""
    edgeResizeHighlightLeafId = -1
    edgeResizeHighlightEdge = ""
    edgeResizeFavoriteSignatureBefore = ""
}

function finishEdgeResize() {
    var hadResize = edgeResizeMovingSplitId >= 0

    clearEdgeResizeState()
    if (hadResize) {
        rebuildLayoutCaches(true)
        updateCurrentLayoutFavoriteState()
    }
}

function chooseSplitOrientation(edge, leafRect) {
    if (edge === "left" || edge === "right")
        return "vertical"
    if (edge === "top" || edge === "bottom")
        return "horizontal"

    if (!leafRect)
        return "vertical"

    return leafRect.width >= leafRect.height ? "vertical" : "horizontal"
}

function createPaneInLeaf(leafId, edge) {
    if (modePickerLeafId !== -1)
        return

    if (maximizedLeafId !== -1)
        maximizedLeafId = -1

    if (leafCount() >= maxPaneCount)
        return

    var paneNumber = nextPaneNumber()
    var newLeaf = makeLeaf(makePane(paneNumber))

    if (!layoutTree) {
        layoutTree = newLeaf
        activeLeafId = newLeaf.leafId
        rebuildLayoutCaches()
        return
    }

    var oldPane = paneByLeafId(layoutTree, leafId)
    if (!oldPane)
        return

    var targetRect = findLeafRect(leafId)
    var orientation = chooseSplitOrientation(edge, targetRect)

    var oldLeaf = {
        type: "leaf",
        leafId: leafId,
        pane: oldPane
    }

    var firstNode = oldLeaf
    var secondNode = newLeaf

    if (orientation === "vertical" && edge === "left") {
        firstNode = newLeaf
        secondNode = oldLeaf
    }
    if (orientation === "vertical" && edge === "right") {
        firstNode = oldLeaf
        secondNode = newLeaf
    }
    if (orientation === "horizontal" && edge === "top") {
        firstNode = newLeaf
        secondNode = oldLeaf
    }
    if (orientation === "horizontal" && edge === "bottom") {
        firstNode = oldLeaf
        secondNode = newLeaf
    }

    var splitNode = makeSplit(orientation, firstNode, secondNode, 0.5)
    layoutTree = replaceLeaf(layoutTree, leafId, splitNode)
    activeLeafId = newLeaf.leafId
    rebuildLayoutCaches()
    setModePickerLeafIds([newLeaf.leafId])
}

function removePane(leafId) {
    if (maximizedLeafId !== -1)
        maximizedLeafId = -1

    if (leafCount() <= 1)
        return

    layoutTree = removeLeafFromNode(layoutTree, leafId)
    layoutTree = renumberPanes(layoutTree)
    activeLeafId = firstLeafId()

    rebuildLayoutCaches()
    refreshModePickerLeafIds()

    if (!hasLeafIdInTree(layoutTree, modeSettingsLeafId))
        closeModeSettingsPanel()
}

function hitLeafByPoint(px, py) {
    for (var i = 0; i < leafRects.length; ++i) {
        var r = leafRects[i]
        if (px >= r.x && px <= r.x + r.width && py >= r.y && py <= r.y + r.height)
            return r.leafId
    }
    return -1
}

function updateDropTargetByCursor() {
    if (!dragActive || draggedLeafId === -1) {
        dropTargetLeafId = -1
        return
    }

    var target = hitLeafByPoint(dragCursor.x, dragCursor.y)
    if (target === draggedLeafId)
        target = -1
    dropTargetLeafId = target
}

function beginPaneDrag(leafId) {
    if (!editableMode)
        return

    draggedLeafId = leafId
    dropTargetLeafId = -1
    dragActive = true
}

function finishPaneDrag() {
    if (draggedLeafId !== -1 && dropTargetLeafId !== -1)
        swapLeafPanes(draggedLeafId, dropTargetLeafId)

    draggedLeafId = -1
    dropTargetLeafId = -1
    dragActive = false
}

Component.onCompleted: {
    loadSettingsGroupsState()
    loadFavoriteLayoutsState()
    loadFullscreenPopupState()
    loadGlobalPopupPreferences()
    if (!restoreLayoutState()) {
        var paneNumber = nextPaneNumber()
        var firstLeaf = makeLeaf(makePane(paneNumber, "3D"))
        layoutTree = firstLeaf
        activeLeafId = firstLeaf.leafId
        clearModePickerSelection()
        rebuildLayoutCaches()
    }
    sanitizeFullscreenPopupConfig()
    updateCurrentLayoutFavoriteState()
}

}

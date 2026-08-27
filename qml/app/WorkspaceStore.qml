import QtQuick 2.15
import QtCore
import kqml_types 1.0
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
property var liveEchogramStates: ({})

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
property bool edgeResizeGhostActive: false
property real edgeResizeGhostCoord: 0       // workspace-axis coord of ghost split line (render)
property real edgeResizeGhostSplitCoord: 0  // split-coord to apply on commit
property bool editableMode: false
property int maximizedLeafId: -1
property bool settingsPanelOpen: false
property bool filePathFocusRequested: false
property bool recordingFocusRequested: false
property bool echogramSettingsActive: false
property var echogramSettingsPlot: null     // the Plot2D whose gear was clicked
property int echogramSettingsLeafId: -1     // leaf of that plot (for focus dimming)
property string echogramSettingsTitle: ""   // header title on the sub-page

// True when a settings-internal drill-in (quickActions/uiSaving/tgc/console)
// is open. Echogram has its own flag (echogramSettingsActive) because it is
// pane-scoped — it drives focus dimming and a dynamic title.
property bool settingsSubPageActive: false
// Which component the settings sidebar's single drill-in slot shows. Set on
// open; kept sticky on close so the component doesn't reload mid slide-out.
property string settingsSubPageKind: "echogram"
readonly property bool anySettingsSubPageActive: echogramSettingsActive || settingsSubPageActive

// Leaf of the active 3D pane; mirrored from WorkspaceView for focus dimming.
property int active3DLeafId: -1

// Window/pane currently being configured → everything else dims for focus.
// -1 when no pane-scoped settings are open. Shared across windows via store.
readonly property int settingsFocusLeafId: {
    if (echogramSettingsActive && echogramSettingsLeafId !== -1)
        return echogramSettingsLeafId
    if (modeSettingsLeafId !== -1)
        return modeSettingsLeafId
    if (settingsPanelOpen) {
        var em = settingsGroupExpandedMap
        var expandedCount = 0
        for (var gk in em) {
            if (Object.prototype.hasOwnProperty.call(em, gk) && em[gk] === true)
                ++expandedCount
        }
        if (expandedCount === 1 && isSettingsGroupExpanded("app.scene3d"))
            return active3DLeafId
    }
    return -1
}
property bool modeSettingsPanelOpen: false
property bool pointerOverSidebar: false
property bool settingsPushContent: false
property bool resizeActive: false
property bool layoutTransitionSuspended: false
property string settingsSide: "left"
property string selectedConnectionFilePath: ""
property bool rotateLayoutEnabled: true
property bool quickActionLayoutsEnabled: true
property bool quickActionConnectionStatusEnabled: true
property bool quickActionLoggingEnabled: true
property bool quickActionBottomTrackEnabled: true
property bool quickActionProfilesEnabled: true
property bool quickActionWidgetsEnabled: true
property bool quickActionConsoleEnabled: true
property bool quickActionSecondWindowEnabled: true
property bool quickActionPowerOffEnabled: false
property bool quickActionInputLockEnabled: true

property bool inputLocked: false

function setInputLocked(on) {
    inputLocked = !!on
    if (inputLocked && typeof core !== "undefined" && core)
        core.requestDismissTransientUi()
}

function toggleInputLock() { setInputLocked(!inputLocked) }

property string quickActionDraggingKey: ""

readonly property var quickActionKeys: {
    var base = ["connections", "logging", "layouts", "widgets", "console", "bottomTrack", "profiles", "inputLock"]
    if (Qt.platform.os !== "android" && Qt.platform.os !== "ios")
        base.push("secondWindow")   // desktop-only; mobile drops it on normalize
    if (Qt.platform.os === "linux" || (typeof manualTesting !== "undefined" && manualTesting === true))
        base.push("powerOff")       // Ubuntu build (or any platform under MANUAL_TESTING)
    return base
}

property var quickActionOrderModel: ListModel {
    ListElement { key: "connections" }
    ListElement { key: "logging" }
    ListElement { key: "layouts" }
    ListElement { key: "widgets" }
    ListElement { key: "console" }
    ListElement { key: "bottomTrack" }
    ListElement { key: "profiles" }
    ListElement { key: "inputLock" }
    ListElement { key: "secondWindow" }
    ListElement { key: "powerOff" }
}

function normalizeQuickActionOrder(list) {
    var out = []
    if (Array.isArray(list)) {
        for (var i = 0; i < list.length; ++i) {
            var k = list[i] === "favorites" ? "layouts" : list[i]
            if (quickActionKeys.indexOf(k) !== -1 && out.indexOf(k) === -1)
                out.push(k)
        }
    }
    for (var j = 0; j < quickActionKeys.length; ++j)   // append any missing keys (new in this version)
        if (out.indexOf(quickActionKeys[j]) === -1) {
            if (quickActionKeys[j] === "logging" && out.indexOf("connections") !== -1)
                out.splice(out.indexOf("connections") + 1, 0, "logging")   // keep logging right after devices
            else if (quickActionKeys[j] === "console" && out.indexOf("widgets") !== -1)
                out.splice(out.indexOf("widgets") + 1, 0, "console")   // keep console right after widgets
            else if (quickActionKeys[j] === "inputLock" && out.indexOf("secondWindow") !== -1)
                out.splice(out.indexOf("secondWindow"), 0, "inputLock") // keep the lock right before the second window
            else
                out.push(quickActionKeys[j])
        }
    return out
}

function quickActionOrderCsv() {
    var a = []
    for (var i = 0; i < quickActionOrderModel.count; ++i)
        a.push(quickActionOrderModel.get(i).key)
    return a.join(",")
}

function applyQuickActionOrder(list) {
    var arr = normalizeQuickActionOrder(list)
    quickActionOrderModel.clear()
    for (var i = 0; i < arr.length; ++i)
        quickActionOrderModel.append({ key: arr[i] })
}

function moveQuickAction(from, to) {
    var n = quickActionOrderModel.count
    if (from < 0 || from >= n || to < 0 || to >= n || from === to)
        return
    quickActionOrderModel.move(from, to, 1)
}

function persistQuickActionOrder() {
    if (typeof layoutStore !== "undefined")
        layoutStore.quickActionOrderStored = quickActionOrderCsv()
}

property var rememberedLinks: []
property int _linkStateRev: 0

readonly property var reconnectInfo: {
    var rev = _linkStateRev
    var count = 0, open = 0, worst = 0
    if (typeof linkManagerWrapper !== "undefined" && linkManagerWrapper) {
        var arr = rememberedLinks || []
        for (var i = 0; i < arr.length; ++i) {
            var s = linkManagerWrapper.linkState(arr[i])
            if (s < 0 || s === 3) continue
            ++count
            if (s === 1 || s === 2) { ++open; if (s > worst) worst = s }
        }
    }
    return { count: count, open: open, worst: worst, allOpen: count > 0 && open === count }
}

property Connections _reconnectLinkConn: Connections {
    target: (typeof linkManagerWrapper !== "undefined") ? linkManagerWrapper : null
    ignoreUnknownSignals: true
    function onLinkOpened(uuid) { store.addRememberedLink(uuid) }
    function onLinkRemoved(uuid) { store.removeRememberedLink(uuid) }
}

property Connections _reconnectModelConn: Connections {
    target: (typeof linkManagerWrapper !== "undefined" && linkManagerWrapper) ? linkManagerWrapper.linkListModel : null
    ignoreUnknownSignals: true
    function onDataChanged() { store._linkStateRev++; store.saveRememberedLinks() }
    function onRowsInserted() { store._linkStateRev++; store.saveRememberedLinks() }
    function onRowsRemoved() { store._linkStateRev++; store.saveRememberedLinks() }
    function onModelReset() { store._linkStateRev++; store.saveRememberedLinks() }
}

function addRememberedLink(uuid) {
    var s = uuid ? String(uuid) : ""
    if (!s.length) return
    var arr = (rememberedLinks || []).slice(0)
    if (arr.indexOf(s) !== -1) return
    arr.push(s)
    rememberedLinks = arr
    saveRememberedLinks()
}

function removeRememberedLink(uuid) {
    var s = uuid ? String(uuid) : ""
    if (!s.length) return
    var arr = (rememberedLinks || []).slice(0)
    var i = arr.indexOf(s)
    if (i === -1) return
    arr.splice(i, 1)
    rememberedLinks = arr
    saveRememberedLinks()
}

function saveRememberedLinks() {
    if (typeof layoutStore === "undefined") return
    var arr = rememberedLinks || []
    var keepSet = {}
    if (typeof linkManagerWrapper !== "undefined" && linkManagerWrapper) {
        var pinned = linkManagerWrapper.pinnedUuids()
        for (var i = 0; i < pinned.length; ++i) keepSet[pinned[i]] = true
        var serial = linkManagerWrapper.serialUuids()
        for (var k = 0; k < serial.length; ++k) keepSet[serial[k]] = true
    }
    var keep = []
    for (var j = 0; j < arr.length; ++j)
        if (keepSet[arr[j]]) keep.push(arr[j])
    layoutStore.rememberedLinksJson = JSON.stringify(keep)
}

function loadRememberedLinks() {
    var parsed = []
    if (layoutStore.rememberedLinksJson && layoutStore.rememberedLinksJson !== "") {
        try { parsed = JSON.parse(layoutStore.rememberedLinksJson) } catch (e) { parsed = [] }
    }
    rememberedLinks = Array.isArray(parsed) ? parsed : []
}

function toggleRememberedLinks() {
    if (typeof linkManagerWrapper === "undefined" || !linkManagerWrapper) return
    var arr = rememberedLinks || []

    var closed = [], open = []
    for (var i = 0; i < arr.length; ++i) {
        var s = linkManagerWrapper.linkState(arr[i])
        if (s === 0) closed.push(arr[i])
        else if (s === 1 || s === 2) open.push(arr[i])
    }
    if (!closed.length && !open.length) return

    if (closed.length) {
        var hadFile = typeof core !== "undefined" && core && core.openedFilePath && core.openedFilePath.length > 0
        if (!open.length && typeof core !== "undefined" && core && typeof core.closeLogFile === "function")
            core.closeLogFile()
        if (!hadFile)
            for (var j = 0; j < closed.length; ++j)
                linkManagerWrapper.reopenLink(closed[j])
    } else {
        for (var k = 0; k < open.length; ++k)
            linkManagerWrapper.closeLink(open[k])
    }
}
property string hotkeysRevealKey: ""
property int hotkeysRevealNonce: 0
// Live reference to the HotkeysDialog while it's open (set by the dialog
// itself on Component.onCompleted/Destruction + onOpened/onClosed). The
// global Esc handler in MainWindow uses this to close ONLY the dialog
// instead of unwinding the whole settings UI.
property var activeHotkeysDialog: null
property int modeSettingsLeafId: -1
property string modeSettingsMode: "2D"
// Adaptive: fullscreen on compact (< 800px window), else min(50% window, 480 * scale).
readonly property real settingsPanelSizePx: {
    var w = windowWidth > 0 ? windowWidth : 800
    if (Tokens.isCompact(w)) return w
    return Math.min(w * 0.5, 480 * AppPalette.scale)
}
readonly property bool _compactMode: Tokens.isCompact(windowWidth)
// Effective push behaviour: user preference, OR forced by compact-mode. Read
// this instead of `settingsPushContent` for layout decisions — keeps the user
// preference value intact across compact↔wide window transitions.
readonly property bool effectivePushContent: settingsPushContent || _compactMode || editableMode
property int modePickerLeafId: -1
property var modePickerLeafIds: []
property int pendingCreatedLeafId: -1
property int hoveredPopupCandidateLeafId: -1
property int flashingLeafId: -1
property int highlightedLeafId: -1
readonly property int globalPopupLeafId: 9999
readonly property int secondaryEchogramKey: 10000
property bool globalPopupFullscreen: false
property var layouts: []
readonly property bool hasLayouts: layouts.length > 0
property int activeLayoutIndex: 0
property var settingsGroupExpandedMap: ({})
property var fullscreenPopupSourceByHost: ({})
property var fullscreenPopupStateByHost: ({})
property bool globalPopupEnabled: false
property string globalPopupMode: ""
property bool globalPopupModePickerOpen: false
property bool globalPopupPreferencesLoading: false

property bool secondaryWindowOpen: false
property string secondaryWindowMode: ""   // "" | "2D" | "3D"

// Closed window releases its slot even if "2D" is persisted in Settings.
readonly property string effectiveSecondaryMode: secondaryWindowOpen ? secondaryWindowMode : ""

// Total active 2D echograms across panes + globalPopup + secondary; limit is 5.
readonly property int activeTwoDCount: paneCountByMode("2D")
    + (globalPopupMode === "2D" ? 1 : 0)
    + (effectiveSecondaryMode === "2D" ? 1 : 0)
readonly property bool secondary2DAvailable: effectiveSecondaryMode === "2D" || activeTwoDCount < 5

// Active device by INDEX into deviceManagerWrapper.devs; -1 = none. Index (not
// devSN) because two identical devices can report the same serial — SN isn't a
// reliable selector. Session-only.
property int activeDeviceIndex: -1

readonly property var activeDeviceList: (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) ? deviceManagerWrapper.devs : []

readonly property int resolvedDeviceIndex: {
    var ds = activeDeviceList
    if (!ds || ds.length === 0) return -1
    var idx = activeDeviceIndex
    if (idx >= 0 && idx < ds.length && ds[idx] && ds[idx].isBoardInited) return idx
    var firstInited = -1
    for (var i = 0; i < ds.length; ++i) {
        if (ds[i] && ds[i].isBoardInited) {
            if (firstInited < 0) firstInited = i
            if (ds[i].isRecorder) return i
        }
    }
    return firstInited
}

readonly property var activeDevice: (resolvedDeviceIndex >= 0 && resolvedDeviceIndex < activeDeviceList.length) ? activeDeviceList[resolvedDeviceIndex] : null

onActiveDeviceListChanged: {
    if (!activeDeviceList || activeDeviceList.length === 0) {
        if (activeDeviceIndex !== -1) setActiveDeviceIndex(-1)
        if (settingsSubPageActive && settingsSubPageKind === "devices")
            closeActiveSettingsSubPage()
    } else if (activeDeviceIndex >= activeDeviceList.length) {
        setActiveDeviceIndex(-1)
    }
}

function selectDevice(dev) {
    var ds = activeDeviceList
    if (!dev || !ds) return
    for (var i = 0; i < ds.length; ++i) {
        if (ds[i] === dev) { setActiveDeviceIndex(i); return }
    }
}

property var globalPopupState: ({
    x: -1,
    y: -1,
    collapsed: false,
    expandedWidth: -1,
    expandedHeight: -1
})

property bool bottomTrackEditorOpen: false
property var btEditPopupState: ({ x: -1, y: -1 })

property bool profilesPopupOpen: false
property var settingsProfiles: []
property var profilesPopupState: ({ x: -1, y: -1 })

property var  widgets: []
readonly property bool hasWidgets: widgets.length > 0
readonly property int widgetLimit: 10
readonly property bool canCreateWidget: widgets.length < widgetLimit
property var  widgetShownMap: ({})
property var  widgetInstances: ({})
property int  widgetEditIndex: -1
property int  widgetEditStep: 1
property var  widgetDragLayer: null

property string widgetDraftKind: "grid"
property int  widgetDraftCols: 1
property int  widgetDraftRows: 1
property int  widgetDraftTransparency: 0
property real widgetDraftScale: 1.0
property bool widgetDraftBig: false
property var  widgetDraftCells: []
property string widgetDraftRep: "value"
readonly property var widgetScaleSteps: [0.75, 1.0, 1.25, 1.5, 2.0, 2.5]
readonly property bool widgetDraftBigAllowed: widgetDraftCols >= 2 && widgetDraftRows >= 2

function _cellSpan(rep, big) {
    var m = big ? 2 : 1
    return { sc: (rep === "labelValueRow" ? 2 : 1) * m, sr: 1 * m }
}

function widgetDraftRepAvailable(rep) {
    var s = _cellSpan(rep, widgetDraftBig)
    return s.sc <= widgetDraftCols && s.sr <= widgetDraftRows
}

function widgetDraftSetBig(b) {
    widgetDraftBig = (b === true) && widgetDraftBigAllowed
    if (!widgetDraftRepAvailable(widgetDraftRep))
        widgetDraftRep = "value"
    widgetDraftClearPreview()
}

function widgetDraftSetScale(s, maxScale) {
    var mx = (typeof maxScale === "number" && maxScale > 0) ? maxScale : Infinity
    var best = widgetScaleSteps[0]
    var bd = Infinity
    var haveAllowed = false
    for (var i = 0; i < widgetScaleSteps.length; ++i) {
        var step = widgetScaleSteps[i]
        if (step > mx) continue
        var d = Math.abs(s - step)
        if (!haveAllowed || d < bd) { bd = d; best = step; haveAllowed = true }
    }
    widgetDraftScale = best
}

property int  widgetDropRow: -1
property int  widgetDropCol: -1
property int  widgetDropSpan: 1
property int  widgetDropSpanRows: 1
property bool widgetDropValid: false
property bool widgetOverPalette: false
property int  widgetPreviewRow: -1
property int  widgetPreviewCol: -1
property int  widgetPreviewSpan: 1
property int  widgetPreviewSpanRows: 1
property bool widgetPreviewValid: false
property string widgetPreviewField: ""
property bool widgetDragActive: false
property string widgetDragRep: "value"
property bool widgetDragBig: false
property string widgetDragField: ""
property int  widgetHoverCols: 0
property int  widgetHoverRows: 0

function widgetDragBegin(rep, big, field) {
    widgetDragRep = rep
    widgetDragBig = (big === true)
    widgetDragField = field ? field : ""
    widgetDragActive = true
    widgetDraftClearPreview()
}

function widgetDragEnd() {
    widgetDragActive = false
}

readonly property bool widgetEditorActive: settingsSubPageActive && settingsSubPageKind === "widgetEdit"

// Local clock of the machine running the app (NOT device/GNSS time), ticking every
// second as HH:MM:SS. Empty (invalid) when the system clock is unset (year < 2001) —
// consumers hide the field and its setting in that case.
property string systemTimeHms: ""
readonly property bool systemTimeValid: systemTimeHms.length > 0
// The same tick as a number, for fields whose value is an AGE. DataFieldCatalog reads it
// (`_nowMs(store)`) and without it a widget's `usblAge` and the tracking/stale flip in
// `usblState` freeze: Date.now() inside a binding is evaluated once and never again, so the
// dependency that moves has to be a property.
property real nowMs: 0
function _updateSystemClock() {
    nowMs = Date.now()
    var d = new Date()
    if (isNaN(d.getTime()) || d.getFullYear() < 2001) { systemTimeHms = ""; return }
    var p = function(n) { return (n < 10 ? "0" : "") + n }
    systemTimeHms = p(d.getHours()) + ":" + p(d.getMinutes()) + ":" + p(d.getSeconds())
}
property Timer _systemClockTimer: Timer {
    interval: 1000; running: true; repeat: true; triggeredOnStart: true
    onTriggered: store._updateSystemClock()
}

property var popupDocks: ({})

function widgetIndexById(id) {
    if (!id) return -1
    for (var i = 0; i < widgets.length; ++i)
        if (widgets[i] && widgets[i].id === id)
            return i
    return -1
}

function widgetById(id) {
    var i = widgetIndexById(id)
    return i >= 0 ? widgets[i] : null
}

function generateWidgetId() {
    var id = ""
    do {
        id = "w_" + Math.floor((1 + Math.random()) * 0x10000000).toString(16)
    } while (widgetIndexById(id) >= 0)
    return id
}

// The panel KINDS. "grid" is the cols×rows field grid this system was built for; "usblNodes"
// is a list whose length comes from the USBL plan at runtime.
//
// A kind exists because the grid's size is a pure function of cols×rows×84px and that is
// load-bearing — occupancy validation, the drop maths, the editor overlay's cell grid and the
// aspect-ray scale snapping all derive from it. A panel whose height depends on how many
// beacons answered cannot be a cell in that grid without breaking each of them separately, so
// it is a different kind of panel that happens to reuse everything ELSE a panel has: the
// position/scale/z instance, the shown map, docking, the list, the limit.
readonly property var widgetKinds: ["grid", "usblNodes"]
function widgetKindOf(def) {
    // Absent means "grid": every blob written before kinds existed is one, and must load
    // unchanged rather than being dropped as malformed.
    return (def && def.kind === "usblNodes") ? "usblNodes" : "grid"
}
function _widgetKindIsFreeform(kind) { return kind === "usblNodes" }

readonly property string servoPanelId: "servo"
readonly property var servoPanelDef: ({ id: "servo", kind: "servo", name: "" })
readonly property bool servoPanelShown: widgetShown(servoPanelId)

property int servoPanelTransparency: 0
property bool servoPanelAutoShow: true

onServoPanelTransparencyChanged: layoutStore.servoPanelTransparencyStored = servoPanelTransparency
onServoPanelAutoShowChanged: { layoutStore.servoPanelAutoShowStored = servoPanelAutoShow; _syncServoPanelAuto() }

function loadServoPanelPreferences() {
    servoPanelTransparency = Math.max(0, Math.min(100, layoutStore.servoPanelTransparencyStored))
    servoPanelAutoShow = layoutStore.servoPanelAutoShowStored
}

readonly property bool servoDeviceAvailable: {
    var ds = activeDeviceList
    for (var i = 0; i < ds.length; ++i)
        if (ds[i] && ds[i].isBoardInited && ds[i].isServoSupport)
            return true
    return false
}

onServoDeviceAvailableChanged: _syncServoPanelAuto()

function _syncServoPanelAuto() {
    if (servoPanelAutoShow && servoPanelShown !== servoDeviceAvailable)
        setWidgetShown(servoPanelId, servoDeviceAvailable)
}

function setServoPanelShown(shown) { setWidgetShown(servoPanelId, shown) }
function openServoPanelSettings() { _openSettingsSubPage("servoPanel") }

function servoPanelPosition(popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    var inst = widgetInstance(servoPanelId)
    var x = (inst.x >= 0) ? inst.x : b.maxX
    var y = (inst.y >= 0) ? inst.y : b.minY
    return Qt.point(clamp(x, b.minX, b.maxX), clamp(y, b.minY, b.maxY))
}

property var _legacyServoIds: []

function _migrateServoPanel() {
    if (_legacyServoIds.length === 0)
        return
    var wasShown = servoPanelShown
    var pos = widgetInstance(servoPanelId)
    for (var i = 0; i < _legacyServoIds.length; ++i) {
        var old = _legacyServoIds[i]
        if (widgetShown(old))
            wasShown = true
        var oi = widgetInstance(old)
        if (pos.x < 0 && pos.y < 0 && oi.x >= 0 && oi.y >= 0)
            pos = oi
    }
    _legacyServoIds = []
    if (pos.x >= 0 || pos.y >= 0)
        _writeWidgetInstance(servoPanelId, pos)
    if (wasShown)
        setWidgetShown(servoPanelId, true)
}

function normalizeWidgetDef(raw) {
    if (!raw || typeof raw !== "object")
        return null

    if (_widgetKindIsFreeform(raw.kind)) {
        // No cells, no grid: the content is the bus's, and there is nothing in the def to
        // validate against a geometry. What it carries is what a panel carries.
        return { id: (typeof raw.id === "string" && raw.id.length) ? raw.id : "",
                 kind: raw.kind,
                 name: (typeof raw.name === "string") ? raw.name : "",
                 transparency: (typeof raw.transparency === "number" && isFinite(raw.transparency))
                               ? Math.max(0, Math.min(100, Math.round(raw.transparency))) : 0 }
    }

    var cols = Math.round(raw.cols)
    var rows = Math.round(raw.rows)
    if (!(cols >= 1 && cols <= 4) || !(rows >= 1 && rows <= 4))
        return null
    var name = (typeof raw.name === "string") ? raw.name : ""
    var transparency = (typeof raw.transparency === "number" && isFinite(raw.transparency))
                       ? Math.max(0, Math.min(100, Math.round(raw.transparency))) : 0
    var occupied = {}
    var cells = []
    var srcCells = Array.isArray(raw.cells) ? raw.cells : []
    for (var i = 0; i < srcCells.length; ++i) {
        var c = srcCells[i]
        if (!c || typeof c !== "object") continue
        var row = Math.round(c.row)
        var col = Math.round(c.col)
        if (!(row >= 0 && row < rows) || !(col >= 0 && col < cols)) continue
        if (!DataFieldCatalog.hasField(c.field)) continue
        var rep = (c.rep === "labelValueRow" || c.rep === "labelValueStacked") ? c.rep : "value"
        var cellBig = (c.big === true)
        var s = _cellSpan(rep, cellBig)
        if (row + s.sr > rows || col + s.sc > cols) continue
        var conflict = false
        var keys = []
        for (var dr = 0; dr < s.sr && !conflict; ++dr) {
            for (var dc = 0; dc < s.sc; ++dc) {
                var k = (row + dr) + "," + (col + dc)
                if (occupied[k]) { conflict = true; break }
                keys.push(k)
            }
        }
        if (conflict) continue
        for (var ki = 0; ki < keys.length; ++ki) occupied[keys[ki]] = true
        cells.push({ row: row, col: col, field: String(c.field), rep: rep, big: cellBig })
    }
    var id = (typeof raw.id === "string" && raw.id.length) ? raw.id : ""
    return { id: id, kind: "grid", name: name, cols: cols, rows: rows,
             transparency: transparency, cells: cells }
}

function saveWidgets() {
    layoutStore.widgetsJson = JSON.stringify(widgets)
}

function loadWidgets() {
    var parsed = []
    if (layoutStore.widgetsJson && layoutStore.widgetsJson !== "") {
        try { parsed = JSON.parse(layoutStore.widgetsJson) } catch (e) { parsed = [] }
    }
    var next = []
    var legacyServo = []
    if (Array.isArray(parsed)) {
        for (var i = 0; i < parsed.length; ++i) {
            if (parsed[i] && parsed[i].kind === "servo") {
                if (typeof parsed[i].id === "string" && parsed[i].id.length)
                    legacyServo.push(parsed[i].id)
                continue
            }
            var def = normalizeWidgetDef(parsed[i])
            if (!def) continue
            if (!def.id || def.id.length === 0) def.id = generateWidgetId()
            next.push(def)
            if (next.length >= widgetLimit) break
        }
    }
    widgets = next
    _legacyServoIds = legacyServo
    saveWidgets()
}

function saveWidget(def) {
    var norm = normalizeWidgetDef(def)
    if (!norm) return ""
    var next = widgets.slice(0)
    if (widgetEditIndex >= 0 && widgetEditIndex < next.length) {
        norm.id = next[widgetEditIndex].id
        next[widgetEditIndex] = norm
    } else {
        if (next.length >= widgetLimit) return ""
        norm.id = generateWidgetId()
        next.push(norm)
    }
    widgets = next
    saveWidgets()
    widgetEditIndex = -1
    return norm.id
}

function deleteWidgetAt(index) {
    if (index < 0 || index >= widgets.length) return
    var id = widgets[index].id
    var next = []
    for (var i = 0; i < widgets.length; ++i)
        if (i !== index) next.push(widgets[i])
    widgets = next
    saveWidgets()
    if (id && widgetInstances[id]) {
        var m = {}
        for (var k in widgetInstances) if (k !== id) m[k] = widgetInstances[k]
        widgetInstances = m
        saveWidgetInstances()
    }
    if (id && widgetShownMap[id] !== undefined) {
        var sm = {}
        for (var sk in widgetShownMap) if (sk !== id) sm[sk] = widgetShownMap[sk]
        widgetShownMap = sm
        saveWidgetShown()
    }
}

function widgetInstance(id) {
    var d = (widgetInstances && id) ? widgetInstances[id] : null
    return {
        x: (d && typeof d.x === "number") ? d.x : -1,
        y: (d && typeof d.y === "number") ? d.y : -1,
        scale: (d && typeof d.scale === "number" && d.scale > 0) ? d.scale : 1.0,
        z: (d && typeof d.z === "number") ? d.z : 0
    }
}

function widgetStackRank(id) {
    return Math.max(0, Math.min(widgetLimit - 1, widgetInstance(id).z))
}

function widgetBringToFront(id) {
    if (!id) return
    var ids = []
    for (var i = 0; i < widgets.length; ++i)
        if (widgets[i] && widgets[i].id) ids.push(widgets[i].id)
    if (ids.indexOf(id) < 0) return
    ids.sort(function(a, b) { return widgetInstance(a).z - widgetInstance(b).z })
    if (ids[ids.length - 1] === id) return
    ids.splice(ids.indexOf(id), 1)
    ids.push(id)
    var m = {}
    for (var k in widgetInstances) m[k] = widgetInstances[k]
    for (var j = 0; j < ids.length; ++j) {
        var inst = widgetInstance(ids[j])
        m[ids[j]] = { x: inst.x, y: inst.y, scale: inst.scale, z: j }
    }
    widgetInstances = m
    saveWidgetInstances()
}

function _writeWidgetInstance(id, inst) {
    if (!id) return
    var m = {}
    for (var k in widgetInstances) m[k] = widgetInstances[k]
    m[id] = inst
    widgetInstances = m
    saveWidgetInstances()
}

function widgetShown(id) { return !!(id && widgetShownMap[id] === true) }

function setWidgetShown(id, shown) {
    if (!id) return
    var m = {}
    for (var k in widgetShownMap) m[k] = widgetShownMap[k]
    m[id] = (shown === true)
    widgetShownMap = m
    saveWidgetShown()
}

function toggleWidgetShown(id) { setWidgetShown(id, !widgetShown(id)) }

function widgetScale(id) { return widgetInstance(id).scale }

function setWidgetScale(id, scale) {
    var inst = widgetInstance(id)
    inst.scale = clamp(scale, 0.75, 2.5)
    _writeWidgetInstance(id, inst)
}

function widgetPosition(id, popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    var inst = widgetInstance(id)
    var casc = (inst.x < 0 && inst.y < 0 && widgetIndexById(id) > 0) ? widgetIndexById(id) * 28 : 0
    var x = (inst.x >= 0) ? inst.x : Math.round((b.minX + b.maxX) / 2) + casc
    var y = (inst.y >= 0) ? inst.y : b.minY + casc
    return Qt.point(clamp(x, b.minX, b.maxX), clamp(y, b.minY, b.maxY))
}

// Bounds that also keep the popup clear of the open settings panel strip, so a
// widget shown while settings is open pops into the visible area instead of hiding behind it.
function widgetRevealBounds(popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    if (settingsPanelOpen && settingsPanelSizePx > 0) {
        var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
        var nb = { minX: b.minX, maxX: b.maxX, minY: b.minY, maxY: b.maxY }
        if (settingsSide === "left")
            nb.minX = Math.max(b.minX, settingsPanelSizePx + popupMarginPx)
        else if (settingsSide === "right")
            nb.maxX = Math.min(b.maxX, areaWidth - settingsPanelSizePx - popupWidth - popupMarginPx)
        if (nb.maxX >= nb.minX)
            return nb
    }
    return b
}

function setWidgetPosition(id, x, y, popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    var inst = widgetInstance(id)
    inst.x = clamp(x, b.minX, b.maxX)
    inst.y = clamp(y, b.minY, b.maxY)
    _writeWidgetInstance(id, inst)
}

function saveWidgetInstances() {
    layoutStore.widgetInstancesJson = JSON.stringify(widgetInstances)
}

function saveWidgetShown() {
    layoutStore.widgetShownJson = JSON.stringify(widgetShownMap)
}

function loadWidgetInstances() {
    var parsed = {}
    if (layoutStore.widgetInstancesJson && layoutStore.widgetInstancesJson !== "") {
        try { parsed = JSON.parse(layoutStore.widgetInstancesJson) } catch (e) { parsed = {} }
    }
    var m = {}
    if (parsed && typeof parsed === "object" && !Array.isArray(parsed)) {
        for (var k in parsed) {
            var d = parsed[k]
            if (!d || typeof d !== "object") continue
            m[k] = {
                x: (typeof d.x === "number") ? d.x : -1,
                y: (typeof d.y === "number") ? d.y : -1,
                scale: (typeof d.scale === "number" && d.scale > 0) ? d.scale : 1.0,
                z: (typeof d.z === "number") ? d.z : 0
            }
        }
    }
    widgetInstances = m
}

function loadWidgetShown() {
    var parsed = {}
    if (layoutStore.widgetShownJson && layoutStore.widgetShownJson !== "") {
        try { parsed = JSON.parse(layoutStore.widgetShownJson) } catch (e) { parsed = {} }
    }
    var m = {}
    if (parsed && typeof parsed === "object" && !Array.isArray(parsed)) {
        for (var k in parsed)
            m[k] = parsed[k] === true
    }
    widgetShownMap = m
}

function _reconcileWidgetMaps() {
    var alive = {}
    alive[servoPanelId] = true
    for (var i = 0; i < widgets.length; ++i)
        if (widgets[i] && widgets[i].id) alive[widgets[i].id] = true
    var mi = {}, changedI = false
    for (var ki in widgetInstances) {
        if (alive[ki]) mi[ki] = widgetInstances[ki]
        else changedI = true
    }
    if (changedI) { widgetInstances = mi; saveWidgetInstances() }
    var ms = {}, changedS = false
    for (var ks in widgetShownMap) {
        if (alive[ks]) ms[ks] = widgetShownMap[ks]
        else changedS = true
    }
    if (changedS) { widgetShownMap = ms; saveWidgetShown() }
}

onBottomTrackEditorOpenChanged: layoutStore.bottomTrackEditorOpenStored = bottomTrackEditorOpen
onProfilesPopupOpenChanged: layoutStore.profilesPopupOpenStored = profilesPopupOpen

readonly property real splitterThickness: 0
readonly property real minPaneSize: 120
readonly property real splitSnapThresholdPx: 18
readonly property int maxPaneCount: 4
readonly property int doubleTapIntervalMs: 320
readonly property int popupMarginPx: 16
readonly property int workspaceResizeDebounceMs: 120

signal workspaceSizeCommitRequested()

property Settings scene3dPersistedSettings: Settings {
    id: scene3dPersistedSettings
    category: "scene3d/view"
    property bool navigationViewButton: false
    property bool useAngleButton: false
    property bool trackLastDataButton: false
}
property alias navigationViewEnabled: scene3dPersistedSettings.navigationViewButton
property alias useAngleEnabled: scene3dPersistedSettings.useAngleButton
property alias trackLastDataEnabled: scene3dPersistedSettings.trackLastDataButton

onNavigationViewEnabledChanged: Scene3dToolBarController.onNavigatorLocationButtonChanged(navigationViewEnabled)
onUseAngleEnabledChanged: Scene3dToolBarController.onUseAngleLocationButtonChanged(useAngleEnabled)
onTrackLastDataEnabledChanged: Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(trackLastDataEnabled)

property Settings scene3dLayerVisibility: Settings {
    id: scene3dLayerVisibility
    category: "scene3d/view"
    property bool boatTrackCheckButton: true
    property bool bottomTrackCheckButton: false
    property bool isobathsCheckButton: false
    property bool mosaicViewCheckButton: false
}
property alias boatTrackVisible:   scene3dLayerVisibility.boatTrackCheckButton
property alias bottomTrackVisible: scene3dLayerVisibility.bottomTrackCheckButton
property alias isobathsVisible:    scene3dLayerVisibility.isobathsCheckButton
property alias mosaicVisible:      scene3dLayerVisibility.mosaicViewCheckButton

property Settings videoStore: Settings {
    id: videoStore
    category: "main/video"
    property string urlStored: ""
    property int fillModeStored: 0
}
property alias videoUrl: videoStore.urlStored
property alias videoFillMode: videoStore.fillModeStored
property string videoActiveUrl: ""
property string videoStatusText: ""
property int videoSourceWidth: 0
property int videoSourceHeight: 0

// Interface pref: hide echogram-settings controls whose data type isn't in the
// dataset (default on). Toggled from the Interface settings group.
property Settings echogramUiPrefs: Settings {
    id: echogramUiPrefs
    category: "scene2d/ui"
    property bool hideEmptyEchogramControls: true
}
property alias hideEmptyEchogramControls: echogramUiPrefs.hideEmptyEchogramControls

// Interface pref: show the surface-quality (cm/cell) label in the 3D scene (default off).
property Settings scene3dUiPrefs: Settings {
    id: scene3dUiPrefs
    category: "scene3d/ui"
    property bool showSurfaceQuality: false
}
property alias showSurfaceQuality: scene3dUiPrefs.showSurfaceQuality

property Settings notificationPrefs: Settings {
    id: notificationPrefs
    category: "main/ui"
    property bool hideImportantNotifications: false
}
property alias hideImportantNotifications: notificationPrefs.hideImportantNotifications

property Settings echogramLoupePrefs: Settings {
    id: echogramLoupePrefs
    category: "scene2d/echogramLoupe"
    property bool visible: false
    property int size: 1
    property int zoom: 100
}
property alias echogramLoupeVisible: echogramLoupePrefs.visible
property alias echogramLoupeSize: echogramLoupePrefs.size
property alias echogramLoupeZoom: echogramLoupePrefs.zoom

signal echogramLoupeApplyRequested()
onEchogramLoupeVisibleChanged: echogramLoupeApplyRequested()
onEchogramLoupeSizeChanged: echogramLoupeApplyRequested()
onEchogramLoupeZoomChanged: echogramLoupeApplyRequested()

signal echogramLoupePreviewPhase(string phase)
function echogramLoupePreview(phase) { echogramLoupePreviewPhase(phase) }

property Settings echogramSyncPrefs: Settings {
    id: echogramSyncPrefs
    category: "scene2d/echogramSync"
    property bool cursor: true
    property bool view: false
}
property alias echogramSyncCursor: echogramSyncPrefs.cursor
property alias echogramSyncView: echogramSyncPrefs.view

function applyEchogramSyncToCore() {
    if (typeof core === "undefined" || !core)
        return
    core.setEchogramSyncCursor(echogramSyncPrefs.cursor)
    core.setEchogramSyncView(echogramSyncPrefs.view)
}
onEchogramSyncCursorChanged: applyEchogramSyncToCore()
onEchogramSyncViewChanged: applyEchogramSyncToCore()

property Settings echogramAimPrefs: Settings {
    id: echogramAimPrefs
    category: "scene2d/echogramAim"
    property bool visible: true
    property bool channel: true
    property bool epoch: true
    property bool resolution: true
    property bool frequency: true
    property bool pulseCount: true
    property bool booster: true
    property bool soundSpeed: true
}
property alias aimPanelVisible: echogramAimPrefs.visible
property alias aimChannel: echogramAimPrefs.channel
property alias aimEpoch: echogramAimPrefs.epoch
property alias aimResolution: echogramAimPrefs.resolution
property alias aimFrequency: echogramAimPrefs.frequency
property alias aimPulseCount: echogramAimPrefs.pulseCount
property alias aimBooster: echogramAimPrefs.booster
property alias aimSoundSpeed: echogramAimPrefs.soundSpeed

function applyAimFieldsToCore() {
    if (typeof core === "undefined" || !core)
        return
    var mask = echogramAimPrefs.visible ? (
                 (1 << 0)
               | (echogramAimPrefs.channel    ? (1 << 1) : 0)
               | (echogramAimPrefs.epoch      ? (1 << 2) : 0)
               | (echogramAimPrefs.resolution ? (1 << 3) : 0)
               | (echogramAimPrefs.frequency  ? (1 << 4) : 0)
               | (echogramAimPrefs.pulseCount ? (1 << 5) : 0)
               | (echogramAimPrefs.booster    ? (1 << 6) : 0)
               | (echogramAimPrefs.soundSpeed ? (1 << 7) : 0)
               ) : 0
    core.setAimFieldsMask(mask)
}
onAimPanelVisibleChanged: applyAimFieldsToCore()
onAimChannelChanged: applyAimFieldsToCore()
onAimEpochChanged: applyAimFieldsToCore()
onAimResolutionChanged: applyAimFieldsToCore()
onAimFrequencyChanged: applyAimFieldsToCore()
onAimPulseCountChanged: applyAimFieldsToCore()
onAimBoosterChanged: applyAimFieldsToCore()
onAimSoundSpeedChanged: applyAimFieldsToCore()

property Settings loggingPersist: Settings {
    id: loggingPersist
    category: "main/logging"
    property bool loggingCheck: false   // KLF active state (for restore)
    property bool loggingCheck2: false  // CSV active state (for restore)
    property bool recordKlf: true       // selected record type (REC starts these)
    property bool recordCsv: false
    property string recordFolder: ""    // log output dir (empty = default Documents/KoggerApp/logs)
}

property alias recordKlf: loggingPersist.recordKlf
property alias recordCsv: loggingPersist.recordCsv
property alias recordFolder: loggingPersist.recordFolder

readonly property bool isRecording: (typeof core !== "undefined" && core) ? (core.loggingKlf || core.loggingCsv) : false

function setRecording(on) {
    if (typeof core === "undefined" || !core)
        return
    if (on && !core.prepareLogDirectory(recordFolder))
        return   // invalid path → core warned; do not start recording
    core.setKlfLogging(on && recordKlf)
    core.setCsvLogging(on && recordCsv)
}

function restoreLoggingFromSettings() {
    if (typeof core === "undefined" || !core)
        return
    if (loggingPersist.loggingCheck || loggingPersist.loggingCheck2) {
        if (!core.prepareLogDirectory(recordFolder))
            return   // saved path no longer usable → do not auto-resume
    }
    core.setKlfLogging(loggingPersist.loggingCheck)
    core.setCsvLogging(loggingPersist.loggingCheck2)
}

property Connections loggingSync: Connections {
    target: (typeof core !== "undefined") ? core : null
    function onLoggingKlfChanged() { loggingPersist.loggingCheck  = core.loggingKlf }
    function onLoggingCsvChanged() { loggingPersist.loggingCheck2 = core.loggingCsv }
}

// TGC lives here (not in the lazy TgcSettingsTab) so persisted values reach
// core at startup even if the user never opens the drill-in. Keys match the
// legacy app.tgc group aliases, so existing settings carry over.
property Settings tgcPersist: Settings {
    id: tgcPersist
    category: "main/tgc"
    property real appTgcGainNear: 50
    property real appTgcGainFar: 250
    property bool appTgcCompensate: false
}

property alias tgcGainNear: tgcPersist.appTgcGainNear
property alias tgcGainFar: tgcPersist.appTgcGainFar
property alias tgcCompensate: tgcPersist.appTgcCompensate

function applyTgcToCore() {
    if (typeof core === "undefined" || !core)
        return
    core.setTgcGainNear(tgcGainNear * 0.01)
    core.setTgcGainFar(tgcGainFar * 0.01)
    core.setTgcCompensate(tgcCompensate)
}

onTgcGainNearChanged:   applyTgcToCore()
onTgcGainFarChanged:    applyTgcToCore()
onTgcCompensateChanged: applyTgcToCore()

property Settings consolePersist: Settings {
    id: consolePersist
    category: "main/console"
    property bool consColorize: true
    property int consMaxRows: 500
}

property alias consoleColorize: consolePersist.consColorize
property alias consoleMaxRows: consolePersist.consMaxRows

function applyConsoleMaxRows() {
    if (typeof core !== "undefined" && core && core.consoleList)
        core.consoleList.setMaxRows(consoleMaxRows)
}

onConsoleMaxRowsChanged: applyConsoleMaxRows()

property Settings exportPersist: Settings {
    id: exportPersist
    category: "main/export"
    property var exportFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/KoggerApp/exports"
    property string exportFolderText: ""
    property bool exportDecimation: false
    property int exportDecimationValue: 10
}

property alias exportFolderUrl: exportPersist.exportFolder
property alias exportFolderSource: exportPersist.exportFolderText
property alias exportDecimationEnabled: exportPersist.exportDecimation
property alias exportDecimationValue: exportPersist.exportDecimationValue

// Isobaths/mosaic theme index — single source of truth so both the settings
// combo and the 3D toolbar swatch picker drive the same value. Keys match the
// legacy combo Settings aliases. Applied to the C++ controllers on change +
// at startup (controllers queue if not ready yet).
property Settings isobathsThemePersist: Settings {
    id: isobathsThemePersist
    category: "scene3d/isobaths"
    property int isobathsTheme: 0
}
property Settings mosaicThemePersist: Settings {
    id: mosaicThemePersist
    category: "scene3d/mosaic"
    property int mosaicTheme: 0
}
property alias isobathsThemeIndex: isobathsThemePersist.isobathsTheme
property alias mosaicThemeIndex: mosaicThemePersist.mosaicTheme

function applyLayerThemesToControllers() {
    if (typeof IsobathsViewControlMenuController !== "undefined" && IsobathsViewControlMenuController)
        IsobathsViewControlMenuController.onThemeChanged(isobathsThemeIndex)
    if (typeof MosaicViewControlMenuController !== "undefined" && MosaicViewControlMenuController)
        MosaicViewControlMenuController.onThemeChanged(mosaicThemeIndex)
}

onIsobathsThemeIndexChanged: if (typeof IsobathsViewControlMenuController !== "undefined" && IsobathsViewControlMenuController) IsobathsViewControlMenuController.onThemeChanged(isobathsThemeIndex)
onMosaicThemeIndexChanged:   if (typeof MosaicViewControlMenuController !== "undefined" && MosaicViewControlMenuController) MosaicViewControlMenuController.onThemeChanged(mosaicThemeIndex)

signal surfaceLayersRefreshRequested()

onBoatTrackVisibleChanged: {
    BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(boatTrackVisible)
}
onBottomTrackVisibleChanged: {
    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(bottomTrackVisible)
    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(bottomTrackVisible)
    if (bottomTrackVisible) surfaceLayersRefreshRequested()
    applyBottomTrackRealtimeToCore()
}

function applyBottomTrackRealtimeToCore() {
    if (typeof core === "undefined" || !core)
        return
    core.setBottomTrackRealtimeFromSettings(bottomTrackVisible)
}
onIsobathsVisibleChanged: {
    if (isobathsVisible) surfaceLayersRefreshRequested()
    IsobathsViewControlMenuController.onProcessStateChanged(isobathsVisible)
    IsobathsViewControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(isobathsVisible)
}
onMosaicVisibleChanged: {
    if (mosaicVisible) surfaceLayersRefreshRequested()
    MosaicViewControlMenuController.onUpdateStateChanged(mosaicVisible)
    MosaicViewControlMenuController.onVisibilityChanged(mosaicVisible)
}

function initLayerVisibilityControllers() {
    BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(boatTrackVisible)
    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(bottomTrackVisible)
    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(bottomTrackVisible)
    IsobathsViewControlMenuController.onProcessStateChanged(isobathsVisible)
    IsobathsViewControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(isobathsVisible)
    MosaicViewControlMenuController.onVisibilityChanged(mosaicVisible)
    MosaicViewControlMenuController.onUpdateStateChanged(mosaicVisible)
    pushMosaicChannelsFromCore()
}

// Hotkey dispatch — finds the matching SettingsGroup instance by stateKey and
// invokes the named hotkey function on it. Returns true on success so callers
// can short-circuit. Used by MainWindow.handleLegacyHotkey for Mosaic/Isobaths
// shortcut groups (prev/next theme, level adjust, step etc.).
function _invokeGroupHotkey(stateKey, fnName, arg) {
    for (var i = 0; i < _settingsGroupInstances.length; ++i) {
        var g = _settingsGroupInstances[i]
        if (!g || g.stateKey !== stateKey) continue
        if (typeof g[fnName] !== "function") return false
        if (arg === undefined) g[fnName]()
        else                   g[fnName](arg)
        return true
    }
    return false
}
function applyMosaicHotkey(fn, parameter) {
    switch (fn) {
    case "mosaicPrevTheme":     return _invokeGroupHotkey("app.mosaic", "prevTheme")
    case "mosaicNextTheme":     return _invokeGroupHotkey("app.mosaic", "nextTheme")
    case "mosaicLowLevelUp":    return _invokeGroupHotkey("app.mosaic", "lowLevelUp",   parameter)
    case "mosaicLowLevelDown":  return _invokeGroupHotkey("app.mosaic", "lowLevelDown", parameter)
    case "mosaicHighLevelUp":   return _invokeGroupHotkey("app.mosaic", "highLevelUp",  parameter)
    case "mosaicHighLevelDown": return _invokeGroupHotkey("app.mosaic", "highLevelDown",parameter)
    }
    return false
}
function applyIsobathsHotkey(fn, parameter) {
    switch (fn) {
    case "surfacePrevTheme": return _invokeGroupHotkey("app.isobaths", "prevTheme")
    case "surfaceNextTheme": return _invokeGroupHotkey("app.isobaths", "nextTheme")
    case "surfaceStepDown":  return _invokeGroupHotkey("app.isobaths", "stepDown", parameter)
    case "surfaceStepUp":    return _invokeGroupHotkey("app.isobaths", "stepUp",   parameter)
    }
    return false
}

function pushMosaicChannelsFromCore() {
    if (!core || !dataset) return
    if (typeof core.setMosaicChannels !== "function") return
    if (typeof dataset.channelsNameList !== "function") return
    for (var i = 0; i < _settingsGroupInstances.length; ++i) {
        var g = _settingsGroupInstances[i]
        if (g && g.stateKey === "app.mosaic") return
    }
    var list = dataset.channelsNameList()
    var ch1 = core.ch1Name
    var ch2 = core.ch2Name
    if (!ch1 || list.indexOf(ch1) <= 0) ch1 = list.length > 1 ? list[1] : ""
    // ch2 fallback: only second real channel (list[2]). When only one real
    // channel exists, leave ch2 empty — must NOT duplicate ch1 onto both slots
    // (would double-feed the mosaic processor).
    if (!ch2 || list.indexOf(ch2) <= 0) ch2 = list.length > 2 ? list[2] : ""
    if (!ch1 && !ch2) return
    core.setMosaicChannels(ch1, ch2)
}

property Connections _coreChannelConn: Connections {
    target: core
    ignoreUnknownSignals: true
    function onChannelListUpdated() { pushMosaicChannelsFromCore() }
}

property Settings layoutStore: Settings {
    category: "main/workspace"

    property string layoutJson: ""
    property int nextLeafSerialStored: 0
    property int nextSplitSerialStored: 0
    property int activeLeafIdStored: -1
    property bool settingsPushContentStored: false
    property string settingsSideStored: "left"
    property bool rotateLayoutEnabledStored: true
    property bool quickActionLayoutsEnabledStored: true
    property bool quickActionConnectionStatusEnabledStored: true
    property bool quickActionLoggingEnabledStored: true
    property bool quickActionBottomTrackEnabledStored: true
    property bool quickActionProfilesEnabledStored: true
    property string quickActionOrderStored: "connections,logging,layouts,bottomTrack,widgets,console,profiles,inputLock,secondWindow,powerOff"
    property string rememberedLinksJson: "[]"
    property string selectedConnectionFilePathStored: ""
    property string layoutsJson: "[]"
    property string favoriteLayoutsJson: ""
    property int activeLayoutIndexStored: 0
    property string settingsGroupExpandedJson: "{}"
    property string fullscreenPopupSourceJson: "{}"
    property string fullscreenPopupStateJson: "{}"
    property bool globalPopupEnabledStored: false
    property string globalPopupModeStored: ""
    property string globalPopupStateJson: "{\"x\":-1,\"y\":-1,\"collapsed\":false,\"expandedWidth\":-1,\"expandedHeight\":-1}"
    property string popupDocksJson: "{}"
    property string btEditPopupStateJson: "{\"x\":-1,\"y\":-1}"
    property string settingsProfilesJson: "[]"
    property string profilesPopupStateJson: "{\"x\":-1,\"y\":-1}"
    property bool profilesPopupOpenStored: false
    property bool bottomTrackEditorOpenStored: false
    property string widgetsJson: "[]"
    property string widgetInstancesJson: "{}"
    property string widgetShownJson: "{}"
    property int servoPanelTransparencyStored: 0
    property bool servoPanelAutoShowStored: true
    property bool quickActionWidgetsEnabledStored: true
    property bool quickActionConsoleEnabledStored: true
    property bool quickActionSecondWindowEnabledStored: true
    property bool quickActionPowerOffEnabledStored: false
    property bool quickActionInputLockEnabledStored: true
    property bool secondaryWindowOpenStored: false
    property string secondaryWindowModeStored: ""
    property string liveEchogramStatesJson: "{}"
}

property Timer liveEchogramPersistTimer: Timer {
    interval: 400
    repeat: false
    onTriggered: layoutStore.liveEchogramStatesJson = JSON.stringify(liveEchogramStates)
}

property Timer favoriteStateSaveTimer: Timer {
    interval: 400
    repeat: false
    onTriggered: saveFavoriteLayoutsState()
}

onEditableModeChanged: {
    if (editableMode) {
        if (maximizedLeafId !== -1)
            maximizedLeafId = -1
    } else if (modePickerLeafIds.length > 0) {
        layoutTree = normalizeAndFixPaneModes(layoutTree, true)
        clearModePickerSelection()
        rebuildLayoutCaches()
    }
}

onSettingsGroupExpandedMapChanged: {
    if (editableMode && !isSettingsGroupExpanded("app.layoutPlacement"))
        editableMode = false
}

onSettingsSubPageActiveChanged: {
    if (settingsSubPageActive && editableMode)
        editableMode = false
}

onSettingsPanelOpenChanged: {
    if (settingsPanelOpen) {
        closeModeSettingsPanel()
    } else {
        if (editableMode)
            editableMode = false
        echogramSettingsActive = false
        echogramSettingsLeafId = -1
        settingsSubPageActive = false
        _settingsNav = []
    }
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
    syncActiveLayout()
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
    echogramSettingsActive = false
    settingsSubPageActive = false
    settingsPanelOpen = true

    var key = "app.layoutPlacement"
    setSettingsGroupExpanded(key, true)
    pendingScrollGroupKey = key
    for (var i = 0; i < _settingsGroupInstances.length; ++i) {
        var g = _settingsGroupInstances[i]
        if (g && g.stateKey === key && typeof g._scrollToTop === "function") {
            Qt.callLater(g._scrollToTop)
            pendingScrollGroupKey = ""
            break
        }
    }
}

function openEchogramSettings(plot, title, leafId) {
    _settingsNav = []
    closeModeSettingsPanel()
    highlightedLeafId = -1            // drop hover-highlight when drilling in
    settingsSubPageActive = false
    settingsSubPageKind = "echogram"
    echogramSettingsPlot = plot
    echogramSettingsLeafId = (typeof leafId === "number") ? leafId : -1
    echogramSettingsTitle = title ? title : qsTr("Echogram")
    echogramSettingsActive = true
    settingsPanelOpen = true
    setSettingsGroupExpanded("app.echograms", true)
}

function closeEchogramSettings() {
    echogramSettingsActive = false
    echogramSettingsLeafId = -1
}

// Return-stack so a cross-link (e.g. the TGC link inside the echogram drill-in)
// can send "back" to where it was opened from instead of the settings root.
// Fresh navigations (gear, group) clear it; cross-links push the current page.
property var _settingsNav: []

function _settingsNavSnapshot() {
    if (echogramSettingsActive)
        return { kind: "echogram", plot: echogramSettingsPlot,
                 leafId: echogramSettingsLeafId, title: echogramSettingsTitle }
    if (settingsSubPageActive)
        return { kind: "subpage", subKind: settingsSubPageKind }
    return null
}

function _restoreSettingsNav(e) {
    closeModeSettingsPanel()
    highlightedLeafId = -1
    if (e && e.kind === "echogram") {
        settingsSubPageActive = false
        settingsSubPageKind = "echogram"
        echogramSettingsPlot = e.plot
        echogramSettingsLeafId = e.leafId
        echogramSettingsTitle = e.title
        echogramSettingsActive = true
        settingsPanelOpen = true
    } else if (e && e.kind === "subpage") {
        echogramSettingsActive = false
        echogramSettingsLeafId = -1
        settingsSubPageKind = e.subKind
        settingsSubPageActive = true
        settingsPanelOpen = true
    } else {
        echogramSettingsActive = false
        echogramSettingsLeafId = -1
        settingsSubPageActive = false
    }
}

// Open one of the settings-internal drill-ins (no pane scope, static title).
function _openSettingsSubPage(kind) {
    var prev = _settingsNavSnapshot()
    if (prev) {
        var st = _settingsNav.slice()
        st.push(prev)
        _settingsNav = st
    }
    closeModeSettingsPanel()
    highlightedLeafId = -1
    echogramSettingsActive = false
    echogramSettingsLeafId = -1
    settingsSubPageKind = kind
    settingsSubPageActive = true
    settingsPanelOpen = true
}

function openQuickActionsSettings() { _openSettingsSubPage("quickActions") }
function openWidgetSettings()       { openAppSettingsAtGroup("app.widgets") }
// Wizard steps: 0 = which kind, 1 = grid size, 2 = place fields, 3 = the acoustic-nodes panel,
// 4 = the servo panel. Creating starts at the kind choice; editing goes straight to the step
// that kind is edited on, because the kind of an existing panel is not something you change —
// you make the other one.
function widgetKindEditStep(kind) {
    return (kind === "usblNodes") ? 3 : 2
}
function openWidgetCreateSettings() { widgetEditIndex = -1; widgetDraftReset(); widgetEditStep = 0; _openSettingsSubPage("widgetEdit") }
function openWidgetEditSettings(index) {
    widgetEditIndex = index
    widgetDraftReset()
    widgetEditStep = widgetKindEditStep(widgetDraftKind)
    _openSettingsSubPage("widgetEdit")
}
// Chosen on step 0. A grid still has a size to pick; the freeform panels have nothing to lay
// out, so they go straight to their own (short) page.
function widgetDraftSetKind(kind) {
    widgetDraftKind = _widgetKindIsFreeform(kind) ? kind : "grid"
    widgetEditStep = (widgetDraftKind === "grid") ? 1 : widgetKindEditStep(widgetDraftKind)
}

function widgetDraftReset() {
    if (widgetEditIndex >= 0 && widgetEditIndex < widgets.length) {
        var d = widgets[widgetEditIndex]
        widgetDraftKind = widgetKindOf(d)
        widgetDraftCols = (typeof d.cols === "number") ? d.cols : 1
        widgetDraftRows = (typeof d.rows === "number") ? d.rows : 1
        widgetDraftTransparency = (typeof d.transparency === "number") ? d.transparency : 0
        widgetDraftScale = widgetScale(d.id)
        widgetDraftCells = Array.isArray(d.cells) ? JSON.parse(JSON.stringify(d.cells)) : []
    } else {
        widgetDraftKind = "grid"
        widgetDraftCols = 1
        widgetDraftRows = 1
        widgetDraftTransparency = 0
        widgetDraftScale = 1.0
        widgetDraftCells = []
    }
    widgetDraftBig = false
    widgetDraftRep = "value"
    widgetDropRow = -1
    widgetDropCol = -1
    widgetDropSpan = 1
    widgetDropSpanRows = 1
    widgetOverPalette = false
    widgetHoverCols = 0
    widgetHoverRows = 0
    widgetPreviewRow = -1
    widgetPreviewCol = -1
    widgetPreviewValid = false
    widgetDragActive = false
}

function widgetDraftIsPlaced(field) {
    for (var i = 0; i < widgetDraftCells.length; ++i)
        if (widgetDraftCells[i].field === field) return true
    return false
}

function widgetDraftOccupantAt(r, c) {
    for (var i = 0; i < widgetDraftCells.length; ++i) {
        var cell = widgetDraftCells[i]
        var s = _cellSpan(cell.rep, cell.big === true)
        if (r >= cell.row && r < cell.row + s.sr && c >= cell.col && c < cell.col + s.sc)
            return { kind: (r === cell.row && c === cell.col) ? "anchor" : "tail", cell: cell }
    }
    return null
}

function _wDraftCoordFree(r, c, ignoreField) {
    var occ = widgetDraftOccupantAt(r, c)
    if (!occ) return true
    return occ.cell.field === ignoreField
}

function _wDraftCellCovers(cell, r, c) {
    var s = _cellSpan(cell.rep, cell.big === true)
    return r >= cell.row && r < cell.row + s.sr && c >= cell.col && c < cell.col + s.sc
}

// Drop validity = footprint fits the grid. Occupied cells are allowed — the drop
// replaces whatever is there (see widgetDraftAdd). Only out-of-grid is rejected.
function widgetDraftCanDrop(r, c, rep, big, ignoreField) {
    if (r < 0 || c < 0) return false
    var s = _cellSpan(rep, big)
    return r + s.sr <= widgetDraftRows && c + s.sc <= widgetDraftCols
}

// Occupancy-aware fit (used by tap-to-change-type, which must not overlap others).
function _widgetDraftFits(r, c, rep, big, ignoreField) {
    if (!widgetDraftCanDrop(r, c, rep, big, ignoreField)) return false
    var s = _cellSpan(rep, big)
    for (var dr = 0; dr < s.sr; ++dr)
        for (var dc = 0; dc < s.sc; ++dc)
            if (!_wDraftCoordFree(r + dr, c + dc, ignoreField)) return false
    return true
}

function widgetDraftAdd(r, c, field, rep, big) {
    var ns = _cellSpan(rep, big)
    var next = []
    for (var i = 0; i < widgetDraftCells.length; ++i) {
        var cell = widgetDraftCells[i]
        if (cell.field === field) continue
        var cs = _cellSpan(cell.rep, cell.big === true)
        var overlap = cell.row < r + ns.sr && r < cell.row + cs.sr
                   && cell.col < c + ns.sc && c < cell.col + cs.sc
        if (overlap) continue
        next.push(cell)
    }
    next.push({ row: r, col: c, field: field, rep: rep, big: big === true })
    widgetDraftCells = next
}

function widgetDraftRemove(field) {
    var next = []
    for (var i = 0; i < widgetDraftCells.length; ++i)
        if (widgetDraftCells[i].field !== field) next.push(widgetDraftCells[i])
    widgetDraftCells = next
}

function widgetDraftCycle(field) {
    var order = ["value", "labelValueRow", "labelValueStacked"]
    for (var i = 0; i < widgetDraftCells.length; ++i) {
        if (widgetDraftCells[i].field !== field) continue
        var cellBig = widgetDraftCells[i].big === true
        var cur = order.indexOf(widgetDraftCells[i].rep)
        for (var s = 1; s <= order.length; ++s) {
            var cand = order[(cur + s) % order.length]
            if (_widgetDraftFits(widgetDraftCells[i].row, widgetDraftCells[i].col, cand, cellBig, field)) {
                var next = widgetDraftCells.slice()
                next[i] = { row: widgetDraftCells[i].row, col: widgetDraftCells[i].col, field: field, rep: cand, big: cellBig }
                widgetDraftCells = next
                return
            }
        }
        return
    }
}

function widgetDraftSetSize(c, r) {
    if (c !== widgetDraftCols || r !== widgetDraftRows)
        widgetDraftScale = 1.0
    widgetDraftCols = c
    widgetDraftRows = r
    if (!widgetDraftBigAllowed)
        widgetDraftBig = false
    var norm = normalizeWidgetDef({ cols: c, rows: r, cells: widgetDraftCells })
    widgetDraftCells = norm ? norm.cells : []
    if (!widgetDraftRepAvailable(widgetDraftRep))
        widgetDraftRep = "value"
    widgetDraftClearPreview()
}

function widgetDraftCommitFromPalette(field, rep, big) {
    if (widgetDropRow >= 0 && widgetDropCol >= 0 && widgetDraftCanDrop(widgetDropRow, widgetDropCol, rep, big, field))
        widgetDraftAdd(widgetDropRow, widgetDropCol, field, rep, big)
    widgetDropRow = -1
    widgetDropCol = -1
    widgetOverPalette = false
    widgetDraftClearPreview()
}

function widgetDraftCommitMoveOrRemove(field, rep, big) {
    if (widgetDropRow >= 0 && widgetDropCol >= 0 && _widgetDraftFits(widgetDropRow, widgetDropCol, rep, big, field))
        widgetDraftAdd(widgetDropRow, widgetDropCol, field, rep, big)
    else if (widgetOverPalette)
        widgetDraftRemove(field)
    widgetDropRow = -1
    widgetDropCol = -1
    widgetOverPalette = false
}

function widgetDraftAutoPlace(field, rep, big) {
    for (var r = 0; r < widgetDraftRows; ++r)
        for (var c = 0; c < widgetDraftCols; ++c)
            if (_widgetDraftFits(r, c, rep, big, field)) {
                widgetDraftAdd(r, c, field, rep, big)
                widgetDraftClearPreview()
                return
            }
}

function widgetDraftPreviewFor(field, rep, big) {
    var s = _cellSpan(rep, big)
    for (var r = 0; r < widgetDraftRows; ++r)
        for (var c = 0; c < widgetDraftCols; ++c)
            if (_widgetDraftFits(r, c, rep, big, field)) {
                widgetPreviewRow = r
                widgetPreviewCol = c
                widgetPreviewSpan = s.sc
                widgetPreviewSpanRows = s.sr
                widgetPreviewValid = true
                widgetPreviewField = field
                return
            }
    widgetDraftClearPreview()
}

function widgetDraftClearPreview() {
    widgetPreviewRow = -1
    widgetPreviewCol = -1
    widgetPreviewField = ""
}

function widgetDraftSave() {
    var isCreate = widgetEditIndex < 0
    var draft = _widgetKindIsFreeform(widgetDraftKind)
        ? { kind: widgetDraftKind, transparency: widgetDraftTransparency }
        : { kind: "grid", cols: widgetDraftCols, rows: widgetDraftRows,
            transparency: widgetDraftTransparency, cells: widgetDraftCells }
    var id = saveWidget(draft)
    if (id) {
        setWidgetScale(id, widgetDraftScale)
        if (isCreate) {
            setWidgetShown(id, true)
            widgetBringToFront(id)
        }
    }
    closeActiveSettingsSubPage()
}
function openAimPanelSettings()     { _openSettingsSubPage("aimPanel") }
function openUiSavingSettings()     { _openSettingsSubPage("uiSaving") }
function openTgcSettings()          { _openSettingsSubPage("tgc") }
function openCsvExportSettings()    { _openSettingsSubPage("csvExport") }
function openConsoleSettings()      { _openSettingsSubPage("console") }

function closeActiveSettingsSubPage() {
    if (_settingsNav.length > 0) {
        var st = _settingsNav.slice()
        var e = st.pop()
        _settingsNav = st
        _restoreSettingsNav(e)
        return
    }
    if (settingsSubPageActive)
        settingsSubPageActive = false
    else
        closeEchogramSettings()
}

function toggleEchogramSettings(plot, title, leafId) {
    if (settingsPanelOpen && echogramSettingsActive && echogramSettingsPlot === plot) {
        echogramSettingsActive = false
        echogramSettingsLeafId = -1
        settingsPanelOpen = false
        return
    }
    openEchogramSettings(plot, title, leafId)
}

property string pendingScrollGroupKey: ""

function openAppSettingsAtGroup(stateKey) {
    _settingsNav = []
    closeModeSettingsPanel()
    echogramSettingsActive = false
    settingsSubPageActive = false
    settingsPanelOpen = true

    var key = normalizedSettingsGroupKey(stateKey)
    if (key === "")
        return

    var nextMap = {}
    nextMap[key] = true
    settingsGroupExpandedMap = nextMap
    saveSettingsGroupsState()
    pendingScrollGroupKey = key

    for (var i = 0; i < _settingsGroupInstances.length; ++i) {
        var g = _settingsGroupInstances[i]
        if (g && g.stateKey === key && typeof g._scrollToTop === "function") {
            Qt.callLater(g._scrollToTop)
            pendingScrollGroupKey = ""
            break
        }
    }
}

function toggleAppSettingsAtGroup(stateKey) {
    var key = normalizedSettingsGroupKey(stateKey)
    if (settingsPanelOpen && key !== "" && isSettingsGroupExpanded(key)) {
        settingsPanelOpen = false
        return
    }
    openAppSettingsAtGroup(stateKey)
}

function openConnectionsSettings() {
    closeModeSettingsPanel()
    echogramSettingsActive = false
    settingsSubPageActive = false
    settingsPanelOpen = true
    setSettingsGroupExpanded("app.connections", true)
}

function openRecordingSettings() {
    recordingFocusRequested = false
    openConnectionsSettings()
    recordingFocusRequested = true
}

function setActiveDeviceIndex(i) {
    if (typeof i !== "number")
        return
    activeDeviceIndex = i
}

function openDeviceSettings() { _openSettingsSubPage("devices") }

function openDeviceSettingsForIndex(idx) {
    setActiveDeviceIndex(idx)
    openDeviceSettings()
}

function toggleAppLayoutSettings() {
    if (settingsPanelOpen) {
        settingsPanelOpen = false
        return
    }

    closeModeSettingsPanel()
    echogramSettingsActive = false
    settingsSubPageActive = false
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
    pendingCreatedLeafId = -1
}

function cancelModePicker() {
    if (modePickerLeafId === -1)
        return
    var created = pendingCreatedLeafId
    clearModePickerSelection()
    if (created !== -1 && leafCount() > 1)
        removePane(created)
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
    if (leafId === pendingCreatedLeafId)
        pendingCreatedLeafId = -1

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

// Counts only panes with an explicit mode ("2D"/"3D"); picker state ("") excluded.
function paneCountByMode(mode) {
    if (!layoutTree)
        return 0
    var panes = []
    Tree.allLeafPanes(layoutTree, panes)
    var n = 0
    for (var i = 0; i < panes.length; ++i) {
        var p = panes[i]
        if (p && p.mode === mode)
            n++
    }
    return n
}

readonly property bool threeDOccupiesWorkspace: {
    if (!layoutTree)
        return false
    if (maximizedLeafId !== -1) {
        var mp = paneByLeafId(layoutTree, maximizedLeafId)
        return !!(mp && normalizedPaneMode(mp.mode) === "3D")
    }
    var panes = []
    Tree.allLeafPanes(layoutTree, panes)
    if (panes.length === 1)
        return normalizedPaneMode(panes[0].mode) === "3D"
    return false
}

// 3D loupe overlay shows when 3D fills the workspace (single 3D pane or a 3D
// pane maximized fullscreen), but NOT while the global popup is drawn over it.
// A fullscreen popup of a 2D pane is already excluded by threeDOccupiesWorkspace
// (the maximized pane isn't 3D → false); a fullscreen 3D keeps the loupe.
readonly property bool threeDLoupeAllowed: threeDOccupiesWorkspace
                                           && !globalPopupEnabled

// Shared 2D echogram limit: panes + globalPopup + secondary <= 5.
function canSecondaryWindowChoose2D() {
    if (effectiveSecondaryMode === "2D")
        return true   // already active, self-toggle ok
    var active = paneCountByMode("2D") + (globalPopupMode === "2D" ? 1 : 0)
    return active < 5
}

// Same gate for GlobalPopup; accounts for secondary's claim.
function canGlobalPopupChoose2D() {
    if (globalPopupMode === "2D")
        return true
    var active = paneCountByMode("2D") + (effectiveSecondaryMode === "2D" ? 1 : 0)
    return active < 5
}

function openSecondaryWindow() {
    secondaryWindowOpen = true
    // Secondary window always hosts a dedicated 2D plot (indx=6).
    // If 2D slot is available — activate; otherwise leave "" so the window shows
    // an "echogram limit reached" message and reactivates when a slot frees up.
    var active = paneCountByMode("2D") + (globalPopupMode === "2D" ? 1 : 0)
    secondaryWindowMode = (active < 5) ? "2D" : ""
    saveLayoutState()
}

function closeSecondaryWindow() {
    secondaryWindowOpen = false
    saveLayoutState()
}

// Auto-activate 2D plot in secondary when a 2D slot frees up (pane removed,
// popup switched off, etc.). Secondary stays "limit reached" until a slot opens.
onActiveTwoDCountChanged: {
    if (secondaryWindowOpen && secondaryWindowMode === "" && activeTwoDCount < 5) {
        secondaryWindowMode = "2D"
        saveLayoutState()
    }
}

function setSecondaryWindowMode(mode) {
    // 3D in secondary not supported yet — UI keeps the button disabled.
    var next = (mode === "2D") ? "2D" : ""
    if (next === "2D" && !canSecondaryWindowChoose2D())
        return false
    secondaryWindowMode = next
    saveLayoutState()
    return true
}

function setGlobalPopupMode(mode) {
    var nextMode = normalizedGlobalPopupMode(mode)
    if (nextMode === "")
        return false
    if (nextMode === "3D" && !globalPopupCanChoose3D())
        return false
    if (nextMode === "2D" && !canGlobalPopupChoose2D())
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

function _btEditPopupBounds(popupWidth, popupHeight) {
    var areaWidth = windowWidth > 0 ? windowWidth : workspaceWidth
    var areaHeight = windowHeight > 0 ? windowHeight : workspaceHeight
    var spacing = popupMarginPx
    var minX = spacing, minY = spacing
    var maxX = areaWidth - popupWidth - spacing
    var maxY = areaHeight - popupHeight - spacing
    if (maxX < minX) { minX = 0; maxX = Math.max(0, areaWidth - popupWidth) }
    if (maxY < minY) { minY = 0; maxY = Math.max(0, areaHeight - popupHeight) }
    return { minX: minX, minY: minY, maxX: maxX, maxY: maxY }
}

function btEditPopupPosition(popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    var s = btEditPopupState || { x: -1, y: -1 }
    var x = (typeof s.x === "number" && s.x >= 0) ? s.x : b.maxX   // default right
    var y = (typeof s.y === "number" && s.y >= 0) ? s.y : b.minY   // default top
    return Qt.point(clamp(x, b.minX, b.maxX), clamp(y, b.minY, b.maxY))
}

function setBtEditPopupPosition(x, y, popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    btEditPopupState = { x: clamp(x, b.minX, b.maxX), y: clamp(y, b.minY, b.maxY) }
    layoutStore.btEditPopupStateJson = JSON.stringify(btEditPopupState)
}

function loadBtEditPopupPreferences() {
    var parsed = { x: -1, y: -1 }
    if (layoutStore.btEditPopupStateJson && layoutStore.btEditPopupStateJson !== "") {
        try { parsed = JSON.parse(layoutStore.btEditPopupStateJson) } catch (e) { parsed = { x: -1, y: -1 } }
    }
    btEditPopupState = {
        x: (typeof parsed.x === "number") ? parsed.x : -1,
        y: (typeof parsed.y === "number") ? parsed.y : -1
    }
}

// ── Sibling docking (popup B glued to popup A's side) ──
function popupDock(id) {
    var d = (popupDocks && id) ? popupDocks[id] : null
    if (!d || !d.targetId)
        return { targetId: "", side: "", gap: 0, cross: 0 }
    return {
        targetId: d.targetId,
        side: d.side || "",
        gap: (typeof d.gap === "number") ? d.gap : 8,
        cross: (typeof d.cross === "number") ? d.cross : 0
    }
}

function _dockChainReaches(startId, goalId) {
    var cur = startId, steps = 0
    while (cur && steps < 16) {
        if (cur === goalId) return true
        var d = popupDocks ? popupDocks[cur] : null
        cur = (d && d.targetId) ? d.targetId : ""
        steps++
    }
    return false
}

function wouldDockCycle(fromId, toId) {
    if (!fromId || !toId) return false
    if (fromId === toId) return true
    return _dockChainReaches(toId, fromId)   // toId's chain already reaches fromId
}

function setPopupDock(id, dock) {
    if (!id) return
    var next = {}
    for (var k in popupDocks) next[k] = popupDocks[k]
    if (!dock || !dock.targetId || wouldDockCycle(id, dock.targetId)) {
        delete next[id]
    } else {
        next[id] = {
            targetId: dock.targetId,
            side: dock.side || "",
            gap: (typeof dock.gap === "number") ? dock.gap : 8,
            cross: (typeof dock.cross === "number") ? dock.cross : 0
        }
    }
    popupDocks = next
    layoutStore.popupDocksJson = JSON.stringify(popupDocks)
}

function loadPopupDocks() {
    var parsed = {}
    if (layoutStore.popupDocksJson && layoutStore.popupDocksJson !== "") {
        try { parsed = JSON.parse(layoutStore.popupDocksJson) } catch (e) { parsed = {} }
    }
    popupDocks = (parsed && typeof parsed === "object") ? parsed : {}
}

// ── Settings profiles list ──
function addSettingsProfile(path) {
    if (!path || !path.length) return
    var next = (settingsProfiles || []).slice(0)
    for (var i = 0; i < next.length; ++i)
        if (next[i] && next[i].path === path) return   // skip duplicates
    next.push({ path: path })
    settingsProfiles = next
    saveSettingsProfiles()
}

function removeSettingsProfile(index) {
    if (!settingsProfiles || index < 0 || index >= settingsProfiles.length) return
    var next = settingsProfiles.slice(0)
    next.splice(index, 1)
    settingsProfiles = next
    saveSettingsProfiles()
}

function saveSettingsProfiles() {
    layoutStore.settingsProfilesJson = JSON.stringify(settingsProfiles || [])
}

function loadSettingsProfiles() {
    var parsed = []
    if (layoutStore.settingsProfilesJson && layoutStore.settingsProfilesJson !== "") {
        try { parsed = JSON.parse(layoutStore.settingsProfilesJson) } catch (e) { parsed = [] }
    }
    settingsProfiles = Array.isArray(parsed) ? parsed : []
}

// ── Profiles popup position (mirror of btEdit; default top-right) ──
function profilesPopupPosition(popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    var s = profilesPopupState || { x: -1, y: -1 }
    var x = (typeof s.x === "number" && s.x >= 0) ? s.x : b.maxX
    var y = (typeof s.y === "number" && s.y >= 0) ? s.y : b.minY
    return Qt.point(clamp(x, b.minX, b.maxX), clamp(y, b.minY, b.maxY))
}

function setProfilesPopupPosition(x, y, popupWidth, popupHeight) {
    var b = _btEditPopupBounds(popupWidth, popupHeight)
    profilesPopupState = { x: clamp(x, b.minX, b.maxX), y: clamp(y, b.minY, b.maxY) }
    layoutStore.profilesPopupStateJson = JSON.stringify(profilesPopupState)
}

function loadProfilesPopupPreferences() {
    var parsed = { x: -1, y: -1 }
    if (layoutStore.profilesPopupStateJson && layoutStore.profilesPopupStateJson !== "") {
        try { parsed = JSON.parse(layoutStore.profilesPopupStateJson) } catch (e) { parsed = { x: -1, y: -1 } }
    }
    profilesPopupState = {
        x: (typeof parsed.x === "number") ? parsed.x : -1,
        y: (typeof parsed.y === "number") ? parsed.y : -1
    }
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
        syncActiveLayout()
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
    syncActiveLayout()
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
        return {
            type: "split",
            orientation: node.orientation === "horizontal" ? "horizontal" : "vertical",
            ratio: ratio,
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
    var normalizedStates = {}
    if (rawEntry.echogramStates && typeof rawEntry.echogramStates === "object") {
        for (var pk in rawEntry.echogramStates) {
            if (!rawEntry.echogramStates.hasOwnProperty(pk))
                continue
            var v = rawEntry.echogramStates[pk]
            if (typeof v === "string" && v.length > 0)
                normalizedStates[String(pk)] = v
        }
    }
    return {
        layout: normalizedLayout,
        popupLinks: normalizedLinks,
        echogramStates: normalizedStates
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
        popupLinks: favoritePopupLinksFromLeafMapping(snapshotState.leafIdToPaneId, fullscreenPopupSourceByHost),
        echogramStates: echogramStatesForCurrentTree(snapshotState.leafIdToPaneId)
    }
}

function persistLiveEchogramStatesSoon() {
    liveEchogramPersistTimer.restart()
}

function loadLiveEchogramStates() {
    var parsed = {}
    try { parsed = JSON.parse(layoutStore.liveEchogramStatesJson || "{}") } catch (e) { parsed = {} }
    liveEchogramStates = (parsed && typeof parsed === "object" && !Array.isArray(parsed)) ? parsed : ({})
}

function echogramStatesForCurrentTree(leafIdToPaneId) {
    var out = {}
    if (!leafIdToPaneId)
        return out
    for (var leafKey in leafIdToPaneId) {
        if (!leafIdToPaneId.hasOwnProperty(leafKey))
            continue
        var pane = paneByLeafId(layoutTree, parseInt(leafKey))
        if (!pane || normalizedPaneMode(pane.mode) !== "2D" || !pane.contentId)
            continue
        var s = liveEchogramStates[pane.contentId]
        if (s)
            out[String(leafIdToPaneId[leafKey])] = s
    }
    return out
}

function writeActiveFavoriteEchogramState(paneId, s) {
    var idx = activeLayoutIndex
    if (idx < 0 || idx >= layouts.length)
        return
    var next = layouts.slice(0)
    var entry = next[idx]
    var states = (entry.echogramStates && typeof entry.echogramStates === "object")
        ? Object.assign({}, entry.echogramStates) : ({})
    states[String(paneId)] = s
    next[idx] = { layout: entry.layout, popupLinks: entry.popupLinks, echogramStates: states }
    layouts = next
    favoriteStateSaveTimer.restart()
}

function captureEchogramState(plot, leafId, includeFavorite) {
    if (includeFavorite === undefined)
        includeFavorite = true
    if (!plot || leafId < 0)
        return
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane || normalizedPaneMode(pane.mode) !== "2D" || !pane.contentId)
        return
    var s = echogramStateSerializer.serialize(plot)
    if (!s || s.length === 0)
        return
    var next = Object.assign({}, liveEchogramStates)
    next[pane.contentId] = s
    liveEchogramStates = next
    persistLiveEchogramStatesSoon()
    if (includeFavorite && activeLayoutIndex >= 0 && activeLayoutIndex < layouts.length)
        writeActiveFavoriteEchogramState(pane.paneId, s)
}

function restoreEchogramStateForLeaf(plot, leafId) {
    if (!plot || leafId < 0)
        return
    var pane = paneByLeafId(layoutTree, leafId)
    if (!pane || normalizedPaneMode(pane.mode) !== "2D" || !pane.contentId)
        return
    var s = liveEchogramStates[pane.contentId]
    if (!s) {
        captureEchogramState(plot, leafId, false)
        return
    }
    if (echogramStateSerializer.deserialize(plot, s)) {
        if (plot.viewState && typeof plot.viewState.reloadFromPlot === "function")
            plot.viewState.reloadFromPlot()
    }
}

function seedLiveEchogramStatesFromFavorite(entry, tree) {
    if (!entry || !entry.echogramStates || !tree)
        return
    var panes = []
    allLeafPanes(tree, panes)
    var next = Object.assign({}, liveEchogramStates)
    var changed = false
    for (var i = 0; i < panes.length; ++i) {
        var p = panes[i]
        if (!p || normalizedPaneMode(p.mode) !== "2D" || !p.contentId)
            continue
        var s = entry.echogramStates[String(p.paneId)]
        if (s) {
            next[p.contentId] = s
            changed = true
        }
    }
    if (changed) {
        liveEchogramStates = next
        persistLiveEchogramStatesSoon()
    }
}

function pruneLiveEchogramStates() {
    if (!layoutTree)
        return
    var panes = []
    allLeafPanes(layoutTree, panes)
    var alive = {}
    for (var i = 0; i < panes.length; ++i)
        if (panes[i] && panes[i].contentId)
            alive[panes[i].contentId] = true
    var next = {}
    var changed = false
    for (var k in liveEchogramStates) {
        if (!liveEchogramStates.hasOwnProperty(k))
            continue
        if (alive[k])
            next[k] = liveEchogramStates[k]
        else
            changed = true
    }
    if (changed) {
        liveEchogramStates = next
        persistLiveEchogramStatesSoon()
    }
}

function saveFavoriteLayoutsState() {
    layoutStore.layoutsJson = JSON.stringify(layouts)
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
            if (parsed[key] === true) {
                nextMap[key] = true   // accordion: restore at most one open group
                break
            }
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

// Direct registry of live SettingsGroup instances. Each group adds itself
// in Component.onCompleted and removes itself in Component.onDestruction.
// Avoids fragile recursive parent-walks for bulk operations.
property var _settingsGroupInstances: []

function registerSettingsGroup(group) {
    if (!group) return
    if (_settingsGroupInstances.indexOf(group) >= 0) return
    var arr = _settingsGroupInstances.slice()
    arr.push(group)
    _settingsGroupInstances = arr
}

function unregisterSettingsGroup(group) {
    if (!group) return
    var idx = _settingsGroupInstances.indexOf(group)
    if (idx < 0) return
    var arr = _settingsGroupInstances.slice()
    arr.splice(idx, 1)
    _settingsGroupInstances = arr
}

function collapseAllSettingsGroups() {
    for (var i = 0; i < _settingsGroupInstances.length; ++i) {
        var g = _settingsGroupInstances[i]
        if (g && g.collapsible)
            g.expanded = false
    }
}

function setSettingsGroupExpanded(groupKey, expanded) {
    var key = normalizedSettingsGroupKey(groupKey)
    if (key === "")
        return

    var nextValue = expanded === true
    var currentlyExpanded = isSettingsGroupExpanded(key)
    if (currentlyExpanded === nextValue)
        return

    // Accordion: at most one group open — expanding one collapses the rest.
    var nextMap = {}
    if (nextValue)
        nextMap[key] = true

    settingsGroupExpandedMap = nextMap
    saveSettingsGroupsState()
}

function loadFavoriteLayoutsState() {
    var raw = layoutStore.layoutsJson
    if ((!raw || raw === "" || raw === "[]") && layoutStore.favoriteLayoutsJson && layoutStore.favoriteLayoutsJson !== "")
        raw = layoutStore.favoriteLayoutsJson

    var parsed = []
    if (raw && raw !== "") {
        try {
            parsed = JSON.parse(raw)
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

    layouts = nextFavorites
    saveFavoriteLayoutsState()
    syncActiveLayout()
}

function syncActiveLayout() {
    if (!layoutTree)
        return
    if (activeLayoutIndex < 0 || activeLayoutIndex >= layouts.length)
        return

    var entry = favoriteLayoutEntryFromCurrent()
    if (!entry)
        return

    var next = layouts.slice(0)
    next[activeLayoutIndex] = entry
    layouts = next
    favoriteStateSaveTimer.restart()
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
    var videoLeafIds = []
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
        else if (mode === "Video")
            videoLeafIds.push(leafId)
    }

    if (threeDLeafIds.length > 1) {
        for (var j = 1; j < threeDLeafIds.length; ++j) {
            var moveOutPane = paneByLeafId(nextTree, threeDLeafIds[j])
            if (moveOutPane)
                nextTree = updatePaneInLeaf(nextTree, threeDLeafIds[j], paneWithMode(moveOutPane, "2D"))
        }
        threeDLeafIds = [threeDLeafIds[0]]
    }

    if (videoLeafIds.length > 1) {
        for (var v = 1; v < videoLeafIds.length; ++v) {
            var extraVideoPane = paneByLeafId(nextTree, videoLeafIds[v])
            if (extraVideoPane)
                nextTree = updatePaneInLeaf(nextTree, videoLeafIds[v], paneWithMode(extraVideoPane, "2D"))
        }
        videoLeafIds = [videoLeafIds[0]]
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

    pruneLiveEchogramStates()
    liveEchogramPersistTimer.stop()
    layoutStore.liveEchogramStatesJson = JSON.stringify(liveEchogramStates)
    if (favoriteStateSaveTimer.running) {
        favoriteStateSaveTimer.stop()
        saveFavoriteLayoutsState()
    }
    saveFullscreenPopupState()
    saveGlobalPopupPreferences()
    layoutStore.layoutJson = JSON.stringify(layoutTree)
    layoutStore.nextLeafSerialStored = Math.max(nextLeafSerial, maxLeafIdInTree(layoutTree))
    layoutStore.nextSplitSerialStored = Math.max(nextSplitSerial, maxSplitIdInTree(layoutTree))
    layoutStore.activeLeafIdStored = activeLeafId
    layoutStore.activeLayoutIndexStored = activeLayoutIndex
    layoutStore.settingsPushContentStored = settingsPushContent
    layoutStore.settingsSideStored = settingsSide
    layoutStore.rotateLayoutEnabledStored = rotateLayoutEnabled
    layoutStore.quickActionLayoutsEnabledStored = quickActionLayoutsEnabled
    layoutStore.quickActionConnectionStatusEnabledStored = quickActionConnectionStatusEnabled
    layoutStore.quickActionLoggingEnabledStored = quickActionLoggingEnabled
    layoutStore.quickActionBottomTrackEnabledStored = quickActionBottomTrackEnabled
    layoutStore.quickActionProfilesEnabledStored = quickActionProfilesEnabled
    layoutStore.quickActionWidgetsEnabledStored = quickActionWidgetsEnabled
    layoutStore.quickActionConsoleEnabledStored = quickActionConsoleEnabled
    layoutStore.quickActionSecondWindowEnabledStored = quickActionSecondWindowEnabled
    layoutStore.quickActionPowerOffEnabledStored = quickActionPowerOffEnabled
    layoutStore.quickActionInputLockEnabledStored = quickActionInputLockEnabled
    layoutStore.quickActionOrderStored = quickActionOrderCsv()
    layoutStore.selectedConnectionFilePathStored = selectedConnectionFilePath
    layoutStore.secondaryWindowOpenStored = secondaryWindowOpen
    layoutStore.secondaryWindowModeStored = secondaryWindowMode
}

function restoreLayoutState() {
    settingsPushContent = layoutStore.settingsPushContentStored
    settingsSide = normalizedSettingsSide(layoutStore.settingsSideStored)
    rotateLayoutEnabled = layoutStore.rotateLayoutEnabledStored
    quickActionLayoutsEnabled = layoutStore.quickActionLayoutsEnabledStored
    quickActionConnectionStatusEnabled = layoutStore.quickActionConnectionStatusEnabledStored
    quickActionLoggingEnabled = layoutStore.quickActionLoggingEnabledStored
    quickActionBottomTrackEnabled = layoutStore.quickActionBottomTrackEnabledStored
    quickActionProfilesEnabled = layoutStore.quickActionProfilesEnabledStored
    quickActionWidgetsEnabled = layoutStore.quickActionWidgetsEnabledStored
    quickActionConsoleEnabled = layoutStore.quickActionConsoleEnabledStored
    quickActionSecondWindowEnabled = layoutStore.quickActionSecondWindowEnabledStored
    quickActionPowerOffEnabled = layoutStore.quickActionPowerOffEnabledStored
    quickActionInputLockEnabled = layoutStore.quickActionInputLockEnabledStored
    applyQuickActionOrder((layoutStore.quickActionOrderStored || "").split(","))
    selectedConnectionFilePath = layoutStore.selectedConnectionFilePathStored
    var storedSecondaryMode = layoutStore.secondaryWindowModeStored
    secondaryWindowMode = (storedSecondaryMode === "2D" || storedSecondaryMode === "3D") ? storedSecondaryMode : ""
    secondaryWindowOpen = layoutStore.secondaryWindowOpenStored

    if (layouts.length === 0)
        return false

    var idx = Math.round(layoutStore.activeLayoutIndexStored)
    if (isNaN(idx) || idx < 0 || idx >= layouts.length)
        idx = 0
    applyLayout(idx)
    return !!layoutTree
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

    if (presetId === 4) {
        return makeLeaf(makePane(1))
    }

    if (presetId === 5) {
        var p5Left = makeLeaf(makePane(1))
        var p5Right = makeLeaf(makePane(2))
        return makeSplit("vertical", p5Left, p5Right, 0.5)
    }

    return null
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

function applyLayout(favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= layouts.length)
        return

    var entry = layouts[favoriteIndex]
    var snapshot = favoriteLayoutSnapshotFromEntry(entry)
    if (!snapshot)
        return

    activeLayoutIndex = favoriteIndex
    layoutStore.activeLayoutIndexStored = favoriteIndex

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
    seedLiveEchogramStatesFromFavorite(entry, normalizedTree)
    fullscreenPopupSourceByHost = popupSourceMapFromFavoriteEntry(entry, normalizedTree)
    fullscreenPopupStateByHost = ({})
    sanitizeFullscreenPopupConfig()
    saveFullscreenPopupState()
    activeLeafId = firstLeafId()
    rebuildLayoutCaches()
    saveLayoutState()
    syncActiveLayout()
}

function deleteLayoutAt(favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= layouts.length)
        return
    if (layouts.length <= 1)
        return

    var nextFavorites = []
    for (var i = 0; i < layouts.length; ++i) {
        if (i !== favoriteIndex)
            nextFavorites.push(layouts[i])
    }

    layouts = nextFavorites
    saveFavoriteLayoutsState()

    if (favoriteIndex === activeLayoutIndex) {
        applyLayout(Math.min(favoriteIndex, layouts.length - 1))
    } else {
        if (favoriteIndex < activeLayoutIndex)
            activeLayoutIndex -= 1
        layoutStore.activeLayoutIndexStored = activeLayoutIndex
    }
}

function createLayoutFromCurrent() {
    var entry = favoriteLayoutEntryFromCurrent()
    if (entry)
        _appendLayoutAndEdit(entry)
}

function createLayoutFromLayout(index) {
    if (index < 0 || index >= layouts.length)
        return
    var entry
    try {
        entry = JSON.parse(JSON.stringify(layouts[index]))
    } catch (e) {
        return
    }
    _appendLayoutAndEdit(entry)
}

function createLayoutFromPreset(presetId) {
    var tree = buildPresetTree(presetId)
    if (!tree)
        return
    var snapState = { nextPaneId: 0, leafIdToPaneId: {} }
    var snapshot = favoriteLayoutSnapshotFromNode(tree, snapState)
    if (!snapshot)
        return
    _appendLayoutAndEdit({ layout: snapshot, popupLinks: [], echogramStates: {} })
}

function _appendLayoutAndEdit(entry) {
    var next = layouts.slice(0)
    next.push(entry)
    layouts = next
    saveFavoriteLayoutsState()

    var newIndex = layouts.length - 1
    applyLayout(newIndex)

    settingsSubPageActive = false
    settingsSubPageKind = "echogram"
    setSettingsGroupExpanded("app.layoutPlacement", true)
    pendingScrollGroupKey = "app.layoutPlacement"
    editableMode = true

    var ids = []
    allLeafIds(layoutTree, ids)
    setModePickerLeafIds(ids)
}

function openCreateLayoutSettings() { _openSettingsSubPage("createLayout") }

function seedDefaultLayouts() {
    var l1 = {
        layout: {
            type: "split", orientation: "vertical", ratio: 0.5,
            first:  { type: "leaf", mode: "3D", paneId: 1 },
            second: { type: "leaf", mode: "2D", paneId: 2 }
        },
        popupLinks: [], echogramStates: {}
    }
    var l2 = {
        layout: {
            type: "split", orientation: "horizontal", ratio: 0.5,
            first: {
                type: "split", orientation: "vertical", ratio: 0.5,
                first:  { type: "leaf", mode: "3D", paneId: 1 },
                second: { type: "leaf", mode: "2D", paneId: 2 }
            },
            second: { type: "leaf", mode: "2D", paneId: 3 }
        },
        popupLinks: [], echogramStates: {}
    }
    layouts = [l1, l2]
    saveFavoriteLayoutsState()
    activeLayoutIndex = 0
    layoutStore.activeLayoutIndexStored = 0
    applyLayout(0)
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

readonly property bool layoutPortrait: rotateLayoutEnabled
                                       && (Qt.platform.os === "android" || Qt.platform.os === "ios")
                                       && windowWidth > 0 && windowHeight > windowWidth
property bool layoutPortraitCW: true

function flipSplits(node, cw) {
    if (!node || node.type !== "split")
        return node
    var toHorizontal = node.orientation === "vertical"
    var swap = toHorizontal ? !cw : cw
    var ratio = swap ? (1.0 - node.ratio) : node.ratio
    return {
        type: "split",
        splitId: node.splitId,
        orientation: toHorizontal ? "horizontal" : "vertical",
        ratio: ratio,
        first:  flipSplits(swap ? node.second : node.first,  cw),
        second: flipSplits(swap ? node.first  : node.second, cw)
    }
}

function effectiveLayoutTree() {
    return layoutPortrait ? flipSplits(layoutTree, layoutPortraitCW) : layoutTree
}

onLayoutPortraitChanged:   rebuildLayoutCaches()
onLayoutPortraitCWChanged: if (layoutPortrait) rebuildLayoutCaches()

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
    splitRectByHandle(effectiveLayoutTree(), 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), newRects, newHandles)
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
    var geo = splitGeometryById(effectiveLayoutTree(), 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), splitId)
    var node = splitNodeById(effectiveLayoutTree(), splitId)
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
    var geo = splitGeometryById(effectiveLayoutTree(), 0, 0, effectiveWorkspaceWidth(), effectiveWorkspaceHeight(), splitId)
    var ratio = splitRatioById(effectiveLayoutTree(), splitId)
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

    var splitNode = splitNodeById(effectiveLayoutTree(), splitId)
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

    if (targetMode === "Video") {
        var currentVideoLeaf = firstLeafIdByMode(nextTree, "Video")
        if (currentVideoLeaf !== -1 && currentVideoLeaf !== leafId)
            return
    }

    if (targetMode === "2D") {
        var currentPaneMode = normalizedPaneMode(targetPane.mode)
        if (currentPaneMode !== "2D") {
            var projected = paneCountByMode("2D") + 1
                          + (globalPopupMode === "2D" ? 1 : 0)
                          + (effectiveSecondaryMode === "2D" ? 1 : 0)
            if (projected > 5)
                return
        }
    }

    var updatedTargetPane = paneByLeafId(nextTree, leafId)
    if (updatedTargetPane)
        nextTree = updatePaneInLeaf(nextTree, leafId, paneWithMode(updatedTargetPane, targetMode))

    layoutTree = nextTree
    removeModePickerLeafId(leafId)
    rebuildLayoutCaches()
}

function videoLeafId() {
    return firstLeafIdByMode(layoutTree, "Video")
}

function openVideoUrl(url) {
    var trimmed = ("" + url).trim()
    if (!trimmed.length)
        return

    videoStatusText = ""
    videoUrl = trimmed
    videoActiveUrl = trimmed
}

readonly property bool videoPaneExists: videoLeafId() !== -1

function stopVideo() {
    videoActiveUrl = ""
    videoStatusText = ""
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
    if (layoutPortrait) {
        var storedNode = splitNodeById(layoutTree, splitId)
        if (storedNode && (storedNode.orientation === "vertical" ? !layoutPortraitCW : layoutPortraitCW))
            boundedRatio = 1 - boundedRatio
    }
    layoutTree = updateSplitRatio(layoutTree, splitId, boundedRatio)
    rebuildLayoutCaches(updateHandles)
}

function beginEdgeResize(leafId, edge, absX, absY) {
    if (layoutPortrait)
        return false

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
    if (layoutPortrait)
        return false

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
        if (edgeResizeHighlightLeafId === -1 || edgeResizeHighlightEdge === "") {
            edgeResizeHighlightLeafId = leafId
            edgeResizeHighlightEdge = edge
        }
    }

    return started
}

function stepSnapSplitCoord(splitId, splitCoord) {
    var limits = splitCoordLimitsById(splitId)
    if (!limits || limits.parentLength <= 0)
        return splitCoord

    var best = splitCoord
    var bestDist = 1e9
    for (var step = 1; step <= 7; ++step) {
        var cand = clamp(limits.parentStart + limits.parentLength * (step / 8) - splitterThickness / 2,
                         limits.min, limits.max)
        var d = Math.abs(splitCoord - cand)
        if (d < bestDist) {
            bestDist = d
            best = cand
        }
    }
    return best
}

function updateEdgeResizePreview(absX, absY) {
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

    var movingSplitCoord = splitCoordForLeafSide(edgeResizeMovingSide, movingCoord)
    movingSplitCoord = stepSnapSplitCoord(edgeResizeMovingSplitId, movingSplitCoord)

    edgeResizeGhostSplitCoord = movingSplitCoord
    edgeResizeGhostCoord = movingSplitCoord  // splitterThickness == 0 → == workspace coord
    edgeResizeGhostActive = true
}

function clearEdgeResizeState() {
    edgeResizeMovingSplitId = -1
    edgeResizeMovingSide = ""
    edgeResizeFixedSplitId = -1
    edgeResizeFixedSide = ""
    edgeResizeAxis = ""
    edgeResizeHighlightLeafId = -1
    edgeResizeHighlightEdge = ""
    edgeResizeGhostActive = false
}

function commitEdgeResize() {
    if (!edgeResizeGhostActive || edgeResizeMovingSplitId < 0) {
        clearEdgeResizeState()
        return
    }

    var movingSplitId = edgeResizeMovingSplitId
    var movingSplitCoord = edgeResizeGhostSplitCoord
    var fixedSplitId = edgeResizeFixedSplitId
    var fixedSide = edgeResizeFixedSide
    var fixedCoord = edgeResizeFixedCoord

    clearEdgeResizeState()

    var prevTree = layoutTree
    setSplitCoordById(movingSplitId, movingSplitCoord, false)

    if (fixedSplitId >= 0) {
        var desiredFixedSplitCoord = splitCoordForLeafSide(fixedSide, fixedCoord)
        setSplitCoordById(fixedSplitId, desiredFixedSplitCoord, false)

        var actualFixedSplitCoord = splitCoordById(fixedSplitId)
        if (isNaN(actualFixedSplitCoord) || Math.abs(actualFixedSplitCoord - desiredFixedSplitCoord) > 0.5) {
            layoutTree = prevTree
        }
    }

    rebuildLayoutCaches(true)
    syncActiveLayout()
}

function cancelEdgeResizePreview() {
    clearEdgeResizeState()
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
    if (layoutPortrait)
        splitNode = flipSplits(splitNode, !layoutPortraitCW)
    layoutTree = replaceLeaf(layoutTree, leafId, splitNode)
    activeLeafId = newLeaf.leafId
    rebuildLayoutCaches()
    setModePickerLeafIds([newLeaf.leafId])
    pendingCreatedLeafId = newLeaf.leafId
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

signal uiStateReapplied()

function loadPersistedUiState() {
    restoreLoggingFromSettings()
    applyEchogramSyncToCore()
    applyAimFieldsToCore()
    loadSettingsGroupsState()
    loadFavoriteLayoutsState()
    loadLiveEchogramStates()
    loadFullscreenPopupState()
    loadGlobalPopupPreferences()
    loadBtEditPopupPreferences()
    loadSettingsProfiles()
    loadRememberedLinks()
    loadProfilesPopupPreferences()
    loadServoPanelPreferences()
    profilesPopupOpen = layoutStore.profilesPopupOpenStored
    bottomTrackEditorOpen = layoutStore.bottomTrackEditorOpenStored
    loadPopupDocks()
    loadWidgets()
    loadWidgetInstances()
    loadWidgetShown()
    _migrateServoPanel()
    _reconcileWidgetMaps()
    _syncServoPanelAuto()
    return restoreLayoutState()
}

function reapplyImportedUiState() {
    if (!loadPersistedUiState())
        seedDefaultLayouts()
    sanitizeFullscreenPopupConfig()
    uiStateReapplied()
}

Component.onCompleted: {
    if (!loadPersistedUiState())
        seedDefaultLayouts()
    sanitizeFullscreenPopupConfig()
    applyTgcToCore()
    applyLayerThemesToControllers()
    applyBottomTrackRealtimeToCore()
}

}

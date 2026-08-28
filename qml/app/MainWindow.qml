import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0

ApplicationWindow {
    id: root

    readonly property bool isMobilePlatform: Qt.platform.os === "android" || Qt.platform.os === "ios"

    readonly property int deviceOrientation: Screen.orientation

    // OS-active window tracker for F11 routing (updated via onActiveChanged of both windows).
    property var lastActiveWindow: root

    width: isMobilePlatform ? Screen.width : 1440
    height: isMobilePlatform ? Screen.height : 720
    minimumWidth: isMobilePlatform ? 0 : 900
    minimumHeight: isMobilePlatform ? 0 : 560
    visible: true
    visibility: isMobilePlatform ? Window.FullScreen : Window.Windowed
    title: core.fileTitle !== "" ? (core.fileTitle + " — KoggerApp, KOGGER") : qsTr("KoggerApp, KOGGER")
    onActiveChanged: if (active) root.lastActiveWindow = root

    WorkspaceStore {
        id: workspaceStore

        windowWidth: root.width
        windowHeight: root.height
        layoutPortraitCW: root.deviceOrientation !== Qt.InvertedPortraitOrientation

        onSurfaceLayersRefreshRequested: updateBottomTrackForRegisteredPlots()
        Component.onCompleted: initLayerVisibilityControllers()
    }

    // The USBL command plan and the interrogation loop, for the session.
    //
    // They used to be declared inside DeviceSettingsPage, which is a settings SUB-PAGE — the
    // loader swaps its component out as soon as the operator navigates elsewhere, so leaving
    // the page destroyed the schedule timer and the poll state and interrogation just stopped.
    // An on-scene panel reporting the plan is read precisely when that panel is closed, so
    // they belong here. The device settings page now receives both as properties.
    UsblPlanStore {
        id: appUsblPlan
        Component.onCompleted: load()
    }
    UsblEngine {
        id: appUsblEngine
        plan: appUsblPlan
        preferredDev: workspaceStore.activeDevice
    }

    // Читаем глобальные настройки при запуске (те же ключи, что сохраняет AppSettingsPage)
    Settings {
        id: startupSettings
        category: "main/ui"
        property int appTheme: 0
        property int instrumentsGradeList: 0
    }
    Settings {
        id: consoleVisibilitySettings
        category: "main/console"
        property bool consoleVisible: false
    }

    Settings {
        id: openFileDialogSettings
        category: "main/ui"
        property string lastLogFolder: ""
    }

    FileDialog {
        id: openLogFileDialog
        title: qsTr("Please choose a file")
        fileMode: FileDialog.OpenFile
        nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]
        onAccepted: {
            openFileDialogSettings.lastLogFolder = currentFolder
            if (!selectedFile)
                return
            var path = selectedFile.toString()
            if (path.startsWith("file:///"))
                path = Qt.platform.os === "windows" ? path.slice(8) : path.slice(7)
            else if (path.startsWith("file://"))
                path = path.slice(7)
            if (path.length && core && typeof core.openLogFile === "function") {
                workspaceStore.selectedConnectionFilePath = path
                core.openLogFile(path, false, false)
            }
        }
    }

    ApplicationWindow {
        id: secondWindow
        objectName: "secondaryAppWindow"
        width: 1080
        height: 540
        title: (core.fileTitle !== "" ? core.fileTitle + " — KoggerApp, KOGGER" : qsTr("KoggerApp, KOGGER"))
               + qsTr(" — Second window")
        visible: workspaceStore.secondaryWindowOpen
        onClosing: function(close) { workspaceStore.closeSecondaryWindow() }
        onActiveChanged: if (active) root.lastActiveWindow = secondWindow
        onVisibleChanged: {
            if (visible) {
                // Force default size every time the window opens — no persistence.
                width = 1080
                height = 540
                raise()
                requestActivate()
            }
        }

        SecondaryWindow {
            id: secondaryContent
            anchors.fill: parent
            store: workspaceStore
            onHotkeyReceived: function(event) {
                if (root.handleHotkeyKeyEvent(event))
                    event.accepted = true
            }
        }
    }

    Connections {
        target: theme
        ignoreUnknownSignals: true
        function onThemeIDChanged()          { startupSettings.appTheme          = theme.themeID }
        function onInstrumentsGradeChanged() { startupSettings.instrumentsGradeList = theme.instrumentsGrade }
        function onInterfaceChanged()        { consoleVisibilitySettings.consoleVisible = theme.consoleVisible }
    }

    // Sidebar moves first, workspace waits, then catches up.
    //
    //   t=0          : user toggles. Sidebar starts sliding (sidebarAnimMs).
    //   t=sidebarMs  : sidebar finished. Workspace target changes; Behavior
    //                  starts animating the inset (workspaceAnimMs).
    //   t=sidebarMs + workspaceMs : everything settled.
    //
    // If the user re-toggles before workspace started, the delay timer
    // just restarts — workspace never sees the intermediate intent.
    // If they re-toggle WHILE workspace is animating, the Behavior
    // re-targets and interpolates from the current value (no snap).
    readonly property int _sidebarAnimMs: AppPalette.sidebarAnimMs
    readonly property int _workspaceAnimMs: AppPalette.workspaceAnimMs
    readonly property bool _anyPushSidebarOpen: workspaceStore.effectivePushContent
                                                && (workspaceStore.settingsPanelOpen
                                                    || workspaceStore.modeSettingsPanelOpen)

    // Gated copy of the open state — flips only after sidebar has finished
    // sliding (workspaceShiftDelayTimer below). The Behavior is on the 0..1
    // *progress*, not on the inset itself — so a window-resize that changes
    // settingsPanelSizePx propagates immediately into the inset without
    // re-triggering a 166 ms animation every drag tick.
    property bool _workspaceShouldShift: false
    property real _workspaceShiftProgress: 0.0

    on_AnyPushSidebarOpenChanged: workspaceShiftDelayTimer.restart()
    on_WorkspaceShouldShiftChanged: _workspaceShiftProgress = _workspaceShouldShift ? 1.0 : 0.0

    Timer {
        id: workspaceShiftDelayTimer
        interval: root._sidebarAnimMs
        onTriggered: root._workspaceShouldShift = root._anyPushSidebarOpen
    }

    Behavior on _workspaceShiftProgress {
        NumberAnimation { duration: root._workspaceAnimMs; easing.type: Easing.OutCubic }
    }

    readonly property real settingsInsetLeft: workspaceStore.settingsSide === "left"
                                              ? _workspaceShiftProgress * workspaceStore.settingsPanelSizePx : 0
    readonly property real settingsInsetRight: workspaceStore.settingsSide === "right"
                                               ? _workspaceShiftProgress * workspaceStore.settingsPanelSizePx : 0
    readonly property bool hotkeysPreviewMode: workspaceStore.settingsPanelOpen && workspaceStore.hotkeysRevealKey !== ""
    property bool hotkeysPreviewClosing: false
    property bool legacyPanelOpen: false
    property var legacyTargetPlot: null
    readonly property bool hotkeysPreviewPinned: workspaceStore.settingsPanelOpen
                                                && (workspaceStore.hotkeysRevealKey !== ""
                                                    || hotkeysRevealCloseTimer.running
                                                    || hotkeysPreviewClosing)
    readonly property bool hotkeysPreviewSticky: workspaceStore.settingsPanelOpen
                                                && workspaceStore.settingsSubPageActive
                                                && workspaceStore.settingsSubPageKind === "quickActions"
    onHotkeysPreviewStickyChanged: {
        if (!hotActions)
            return
        if (hotkeysPreviewSticky) {
            hotkeysRevealCloseTimer.stop()
            hotkeysRevealUnpinTimer.stop()
            hotkeysPreviewClosing = false
            hotActions.layoutsMenuOpen = false   // no open dropdown escaping the preview input-blocker
            hotActions.expanded = true
        } else {
            hotActions.expanded = false
        }
    }
    readonly property int hotkeysPreviewGap: 10

    readonly property rect fullscreenPopupEffectiveBounds: {
        if (!fullscreenPanePopup.visible || !fullscreenPanePopup.popupVisible
                || fullscreenPanePopup.fullscreenMode)
            return Qt.rect(-1, -1, 0, 0)
        return Qt.rect(fullscreenPanePopup.panelX, fullscreenPanePopup.panelY,
                       fullscreenPanePopup.expandedWidth, fullscreenPanePopup.expandedHeight)
    }

    readonly property rect globalPopupEffectiveBounds: {
        var item = globalPopupLoader.item
        if (!item || !item.popupVisible || item.fullscreenMode)
            return Qt.rect(-1, -1, 0, 0)
        return Qt.rect(item.panelX, item.panelY, item.expandedWidth, item.expandedHeight)
    }

    readonly property rect btEditPopupEffectiveBounds: {
        if (!btEditPopup.visible || !btEditPopup.popupVisible)
            return Qt.rect(-1, -1, 0, 0)
        return Qt.rect(btEditPopup.panelX, btEditPopup.panelY,
                       btEditPopup.expandedWidth, btEditPopup.expandedHeight)
    }

    readonly property rect profilesPopupEffectiveBounds: {
        if (!profilesPopup.visible || !profilesPopup.popupVisible)
            return Qt.rect(-1, -1, 0, 0)
        return Qt.rect(profilesPopup.panelX, profilesPopup.panelY,
                       profilesPopup.expandedWidth, profilesPopup.expandedHeight)
    }

    function isValidUuidText(uuidValue) {
        if (uuidValue === undefined || uuidValue === null)
            return false

        var text = String(uuidValue)
        return text.length > 0 && text !== "{00000000-0000-0000-0000-000000000000}"
    }

    function refreshConnectionsIndicator() {
        var hasOpenConnection = false

        try {
            if (linkManagerWrapper && typeof linkManagerWrapper.getFirstOpened === "function")
                hasOpenConnection = isValidUuidText(linkManagerWrapper.getFirstOpened())
        } catch (error) {
            hasOpenConnection = false
        }

        hotActions.connectionsOnline = hasOpenConnection
    }

    function isTextInputFocused() {
        var focusItem = root.activeFocusItem
        return focusItem
               && (focusItem instanceof TextEdit
                   || focusItem instanceof TextField
                   || focusItem instanceof TextArea
                   || focusItem instanceof TextInput)
    }

    // Re-entrancy guard.
    property bool _closingTransientUi: false

    // Plot2D owning the currently-focused echogram (secondary window wins when active).
    function _activePlot2D() {
        if (root.lastActiveWindow === secondWindow && secondWindow.visible)
            return secondaryContent ? secondaryContent.plot2DInstance : null
        if (!workspaceView || !workspaceView.plotItemsByLeafId) return null
        if (workspaceStore.activeLeafId < 0) return null
        return workspaceView.plotItemsByLeafId[String(workspaceStore.activeLeafId)] || null
    }

    // "2D" | "3D" | "" — mode of the currently-focused pane (ESC routing).
    function _activeLeafMode() {
        if (root.lastActiveWindow === secondWindow && secondWindow.visible)
            return "2D"
        if (workspaceStore.activeLeafId < 0) return ""
        if (workspaceStore.activeLeafId === workspaceStore.globalPopupLeafId)
            return workspaceStore.globalPopupMode || "2D"
        var rects = workspaceStore.leafRects
        if (!rects) return ""
        for (var i = 0; i < rects.length; ++i) {
            if (rects[i].leafId === workspaceStore.activeLeafId)
                return rects[i].pane ? rects[i].pane.mode : ""
        }
        return ""
    }

    // ESC priority — one layer per call, innermost first. Reorder to repriortize.
    // Layers 1–3 are gated by _activeLeafMode() so the focused pane's UI always wins.
    readonly property var _transientUiLayers: [
        function() {  // HotkeysDialog (and other registered modal dialogs) — always wins.
            if (workspaceStore.activeHotkeysDialog
                    && typeof workspaceStore.activeHotkeysDialog.close === "function") {
                workspaceStore.activeHotkeysDialog.close()
                return true
            }
            return false
        },
        function() {  // 2D-only: Plot2D right-click menu + contact dialog
            if (root._activeLeafMode() !== "2D") return false
            var p = root._activePlot2D()
            if (!p || !p.hasTransientUi) return false
            p.closeTransientUi()
            return true
        },
        function() {  // 3D-only: context menu → ruler cancel → layers/geometry panels
            if (root._activeLeafMode() !== "3D") return false
            var p3 = workspaceView ? workspaceView.active3DPane : null
            if (!p3 || !p3.hasTransientUi) return false
            p3.closeTransientUi()
            return true
        },
        function() {  // 2D-only: Plot2D gear
            if (root._activeLeafMode() !== "2D") return false
            var p = root._activePlot2D()
            if (!p || !p.settingsOpen) return false
            p.closeSettings()
            return true
        },
        function() {  // pane mode picker
            if (workspaceStore.modePickerLeafId === -1) return false
            workspaceStore.cancelModePicker()
            return true
        },
        function() {  // pane mode settings
            if (!workspaceStore.modeSettingsPanelOpen) return false
            workspaceStore.closeModeSettingsPanel()
            return true
        },
        function() {  // bottom-track edit palette — Esc closes it (resets tool)
            var toolActive = typeof core !== "undefined" && core && core.bottomTrackEditTool !== 0
            if (!workspaceStore.bottomTrackEditorOpen && !toolActive) return false
            workspaceStore.bottomTrackEditorOpen = false
            if (typeof core !== "undefined" && core)
                core.setBottomTrackEditTool(0)
            return true
        },
        function() {  // settings-profile palette — Esc closes it
            if (!workspaceStore.profilesPopupOpen) return false
            workspaceStore.profilesPopupOpen = false
            return true
        },
        function() {  // console drawer — Esc closes it
            if (!theme || !theme.consoleVisible) return false
            theme.consoleVisible = false
            return true
        },
        function() {  // HotActions favorites popup
            if (!hotActions.layoutsMenuOpen) return false
            hotActions.layoutsMenuOpen = false
            return true
        },
        function() {  // HotActions widgets popup
            if (!hotActions.widgetsMenuOpen) return false
            hotActions.widgetsMenuOpen = false
            return true
        },
        function() {  // HotActions expanded — skip when it's a settings preview (Esc closes the tab instead)
            if (!hotActions.expanded) return false
            if (root.hotkeysPreviewPinned || root.hotkeysPreviewSticky) return false
            hotActions.expanded = false
            return true
        },
        function() {  // legacy main panel
            if (!legacyPanelOpen) return false
            legacyPanelOpen = false
            return true
        },
        function() {  // any settings drill-in (echogram / quick-actions / widget editor / UI saving / TGC)
            if (!workspaceStore.settingsPanelOpen || !workspaceStore.anySettingsSubPageActive)
                return false
            if (workspaceStore.settingsSubPageKind === "widgetEdit" && workspaceStore.widgetEditStep === 2) {
                workspaceStore.widgetEditStep = 1
                return true
            }
            workspaceStore.closeActiveSettingsSubPage()
            return true
        },
        function() {  // main app settings (last resort)
            if (!workspaceStore.settingsPanelOpen) return false
            workspaceStore.settingsPanelOpen = false
            return true
        }
    ]

    function closeTransientUi() {
        if (_closingTransientUi)
            return false
        _closingTransientUi = true

        var handled = false
        for (var i = 0; i < _transientUiLayers.length; ++i) {
            if (_transientUiLayers[i]()) {
                handled = true
                break
            }
        }

        _closingTransientUi = false
        return handled
    }

    function closeAllTransientUi() {
        if (_closingTransientUi)
            return
        _closingTransientUi = true

        for (var pass = 0; pass < _transientUiLayers.length; ++pass) {
            var handled = false
            for (var i = 0; i < _transientUiLayers.length; ++i) {
                if (_transientUiLayers[i]())
                    handled = true
            }
            if (!handled)
                break
        }

        _closingTransientUi = false
    }

    function toggleFullScreenMode() {
        if (root.isMobilePlatform)
            return false

        root.visibility = root.visibility === Window.FullScreen
                        ? Window.Windowed
                        : Window.FullScreen
        return true
    }

    // F11 toggles the active window (see lastActiveWindow).
    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        enabled: !workspaceStore.inputLocked
        onActivated: {
            if (root.lastActiveWindow === secondWindow && secondWindow.visible) {
                secondWindow.visibility = secondWindow.visibility === Window.FullScreen
                                          ? Window.Windowed
                                          : Window.FullScreen
            } else {
                root.toggleFullScreenMode()
            }
        }
    }

    // Global Esc — Plot2D/3D scene swallow keys before mainLayer.Keys.onReleased.
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        autoRepeat: false
        enabled: !workspaceStore.inputLocked
        onActivated: {
            if (root.isTextInputFocused())
                return
            root.closeTransientUi()
        }
    }

    // Desktop keyboard scrolling (PgUp/PgDn page, Home/End to top/bottom) for any
    // open scrollable surface: key-bindings dialog, console, settings panel.
    // Not on Android/iOS; suppressed while a text field is focused.
    // Reactive text-focus flag (bindings track activeFocusItem). When a text
    // field is focused the scroll shortcuts must NOT be enabled — otherwise they
    // would swallow Home/End/PgUp/PgDn from the field.
    readonly property bool _textInputFocused: {
        var f = root.activeFocusItem
        return !!f && (f instanceof TextEdit || f instanceof TextField
                       || f instanceof TextArea || f instanceof TextInput)
    }

    // The modal key-bindings dialog handles these keys itself (a modal Popup
    // blocks outside ApplicationShortcuts), so it's excluded here.
    readonly property bool _kbdScrollActive: !root.isMobilePlatform
                                             && (workspaceStore.settingsPanelOpen
                                                 || consoleDrawer.consoleOpen)
                                             && !workspaceStore.activeHotkeysDialog
                                             && !workspaceStore.inputLocked
                                             && !root._textInputFocused

    // "settings" | "console" — the surface most recently opened or clicked into.
    // Updated by the surfaces' onOpenChanged / interacted(); a click on the scene
    // (outside both) does NOT change it, so keys keep going to the last active one.
    property string _lastScrollSurface: ""

    function _kbdScrollTarget() {
        var cOpen = consoleDrawer.consoleOpen
        var sOpen = workspaceStore.settingsPanelOpen
        if (cOpen && sOpen)
            return (root._lastScrollSurface === "settings") ? settingsSidebar : consoleDrawer
        if (sOpen)
            return settingsSidebar
        if (cOpen)
            return consoleDrawer
        return null
    }

    function _kbdScroll(kind) {
        if (root.isTextInputFocused())
            return
        var t = _kbdScrollTarget()
        if (t && typeof t.kbdScroll === "function")
            t.kbdScroll(kind)
    }

    Shortcut {
        sequences: [ StandardKey.MoveToNextPage ]
        context: Qt.ApplicationShortcut
        enabled: root._kbdScrollActive
        onActivated: root._kbdScroll("down")
    }
    Shortcut {
        sequences: [ StandardKey.MoveToPreviousPage ]
        context: Qt.ApplicationShortcut
        enabled: root._kbdScrollActive
        onActivated: root._kbdScroll("up")
    }
    Shortcut {
        sequences: [ StandardKey.MoveToStartOfLine ]
        context: Qt.ApplicationShortcut
        enabled: root._kbdScrollActive
        onActivated: root._kbdScroll("top")
    }
    Shortcut {
        sequences: [ StandardKey.MoveToEndOfLine ]
        context: Qt.ApplicationShortcut
        enabled: root._kbdScrollActive
        onActivated: root._kbdScroll("bottom")
    }

    function openSelectedFile() {
        var filePath = workspaceStore.selectedConnectionFilePath
        if (!filePath && core && core.filePath && core.filePath.length > 0)
            filePath = core.filePath

        if (filePath && filePath.length > 0) {
            if (typeof core.openLogFile === "function")
                core.openLogFile(filePath, false, false)
            return true
        }

        workspaceStore.filePathFocusRequested = true
        workspaceStore.openAppSettingsAtGroup("app.files")
        return true
    }

    function closeSelectedFile() {
        if (typeof core.closeLogFile === "function") {
            core.closeLogFile()
            return true
        }
        return false
    }

    function updateBottomTrackForRegisteredPlots() {
        var handled = false
        if (workspaceView && workspaceView.plotItemsByLeafId) {
            for (var key in workspaceView.plotItemsByLeafId) {
                if (!Object.prototype.hasOwnProperty.call(workspaceView.plotItemsByLeafId, key))
                    continue

                var item = workspaceView.plotItemsByLeafId[key]
                if (item && typeof item.updateBottomTrackProcessing === "function") {
                    item.updateBottomTrackProcessing()
                    handled = true
                }
            }
        }
        return handled
    }

    function updateMosaicProcessing() {
        if (typeof core.refreshMosaicProcessing === "function") {
            core.refreshMosaicProcessing()
            return true
        }
        return false
    }

    function setActivePaneMode(mode) {
        if (!workspaceStore || typeof workspaceStore.applyPaneModeSelection !== "function")
            return false

        var leafId = workspaceStore.activeLeafId
        if (leafId === undefined || leafId === null || leafId < 0) {
            if (typeof workspaceStore.firstLeafId === "function")
                leafId = workspaceStore.firstLeafId()
        }
        if (leafId === undefined || leafId === null || leafId < 0)
            return false

        workspaceStore.applyPaneModeSelection(leafId, mode)
        return true
    }

    function handleLegacyHotkey(functionName, parameter) {
        var fn = typeof functionName === "string" ? functionName : ""
        if (fn === "")
            return false

        // F11 handled by Shortcut above (lastActiveWindow routing).
        if (fn === "openFile")
            return openSelectedFile()
        if (fn === "openFileDialog") {
            if (openFileDialogSettings.lastLogFolder.length)
                openLogFileDialog.currentFolder = openFileDialogSettings.lastLogFolder
            openLogFileDialog.open()
            return true
        }
        if (fn === "closeFile")
            return closeSelectedFile()
        if (fn === "updateBottomTrack")
            return updateBottomTrackForRegisteredPlots()
        if (fn === "updateMosaic")
            return updateMosaicProcessing()
        if (fn === "closeSettings")
            return closeTransientUi()

        if (isTextInputFocused())
            return false

        var handled = workspaceView.applyLegacy2DHotkey(fn, parameter)
        if (handled)
            return true

        if (workspaceStore.applyMosaicHotkey(fn, parameter))
            return true
        if (workspaceStore.applyIsobathsHotkey(fn, parameter))
            return true
        if (workspaceView.apply3DHotkey(fn, parameter))
            return true

        if (fn === "clickSettings") {
            legacyPanelOpen = false
            workspaceStore.toggleAppLayoutSettings()
            return true
        }

        return false
    }

    function handleHotkeyKeyEvent(event) {
        if (workspaceStore.inputLocked)
            return false

        // Esc handled by ApplicationShortcut — skip legacy hotkey (scanCode 1 → "closeSettings").
        if (event && event.key === Qt.Key_Escape)
            return false

        if (event && (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)))
            return false

        var scanCode = event && typeof event.nativeScanCode === "number" && event.nativeScanCode > 0
                     ? event.nativeScanCode.toString()
                     : ""
        var hotkeyData = scanCode !== "" && typeof hotkeysMapScan !== "undefined"
                       ? hotkeysMapScan[scanCode]
                       : undefined

        if (hotkeyData !== undefined) {
            return handleLegacyHotkey(hotkeyData.functionName, hotkeyData.parameter)
        }

        if (!event)
            return false

        // Android hardware back is handled window-level in onClosing (focus-
        // independent) — not here, where mainLayer must hold focus.
        if (event.key === Qt.Key_F10)
            return openSelectedFile()
        if (event.key === Qt.Key_F9)
            return closeSelectedFile()
        if (event.key === Qt.Key_F8)
            return updateBottomTrackForRegisteredPlots()
        if (event.key === Qt.Key_F7)
            return updateMosaicProcessing()

        return false
    }

    onClosing: function(close) {
        // On Android the hardware back button / gesture arrives here as a window
        // close request (focus-independent — unlike Keys.onReleased, which only
        // fires when mainLayer holds focus). Treat it like Esc: dismiss ONE
        // transient UI layer and ALWAYS cancel the close so back never minimizes
        // or exits the app. Home/Recents (real backgrounding) go through the OS,
        // not here. Desktop close (window X) proceeds normally and saves.
        if (root.isMobilePlatform) {
            closeTransientUi()
            close.accepted = false
            return
        }
        workspaceStore.saveLayoutState()
        if (typeof core !== "undefined" && core)
            core.saveCameraViewToSettings()
    }

    Component.onDestruction: workspaceStore.saveLayoutState()
    Component.onCompleted: {
        // Применяем сохранённые глобальные настройки сразу при запуске
        if (theme) {
            theme.themeID          = startupSettings.appTheme
            theme.instrumentsGrade = startupSettings.instrumentsGradeList
            theme.consoleVisible   = consoleVisibilitySettings.consoleVisible
        }

        refreshConnectionsIndicator()
        if ((!workspaceStore.selectedConnectionFilePath || workspaceStore.selectedConnectionFilePath.length === 0)
                && core && core.filePath && core.filePath.length > 0) {
            workspaceStore.selectedConnectionFilePath = core.filePath
        }
    }

    // Persist when the app leaves the foreground (Home / Recents). Android keeps
    // the process alive for a fast resume, but may kill it under memory pressure;
    // saving here means a later cold start restores the work, no data loss.
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive)
                workspaceStore.saveLayoutState()
        }
    }

    Item {
        id: mainLayer
        anchors.fill: parent
        focus: true

        Component.onCompleted: forceActiveFocus()

        readonly property int inputLockKey: Qt.Key_F8
        property bool inputLockHoldConsumed: false
        property bool inputLockKeyDown: false

        Timer {
            id: inputLockHoldTimer
            interval: 500
            repeat: false
            onTriggered: {
                if (!workspaceStore.inputLocked)
                    return
                workspaceStore.setInputLocked(false)
                mainLayer.inputLockHoldConsumed = true
            }
        }

        Keys.onPressed: function(event) {
            if (event.key !== mainLayer.inputLockKey)
                return
            if (!event.isAutoRepeat) {
                mainLayer.inputLockHoldConsumed = false
                mainLayer.inputLockKeyDown = true
                inputLockHoldTimer.restart()
            }
            event.accepted = true
        }

        Keys.onReleased: function(event) {
            if (event.key === mainLayer.inputLockKey) {
                if (!event.isAutoRepeat) {
                    inputLockHoldTimer.stop()
                    mainLayer.inputLockKeyDown = false
                    if (!mainLayer.inputLockHoldConsumed && !workspaceStore.inputLocked)
                        workspaceStore.setInputLocked(true)
                    mainLayer.inputLockHoldConsumed = false
                }
                event.accepted = true
                return
            }
            if (handleHotkeyKeyEvent(event)) {
                event.accepted = true
            }
        }

        Rectangle {
            // Full-width window fill — independent of sidebar/workspace
            // shift so the area being uncovered during sidebar close
            // doesn't flash the ApplicationWindow default background.
            anchors.fill: parent
            color: "#0B1220"
        }

        DropArea {
            id: fileDropArea
            anchors.fill: parent
            z: ZOrder.hotActionsActive + 1

            property string droppedFilePath: ""

            function _extractLogPath(urlList) {
                for (var i = 0; i < urlList.length; ++i) {
                    var path = String(urlList[i])
                    if (path.startsWith("file:///"))
                        path = Qt.platform.os === "windows" ? path.slice(8) : path.slice(7)
                    else if (path.startsWith("file://"))
                        path = path.slice(7)
                    var lower = path.toLowerCase()
                    if (lower.endsWith(".klf") || lower.endsWith(".xtf") || lower.endsWith(".ubx"))
                        return path
                }
                return ""
            }

            onEntered: function(drag) {
                droppedFilePath = drag.hasUrls ? _extractLogPath(drag.urls) : ""
                drag.accepted = droppedFilePath !== ""
            }

            onExited: droppedFilePath = ""

            onDropped: function(drop) {
                if (droppedFilePath !== "" && core && typeof core.openLogFile === "function") {
                    workspaceStore.selectedConnectionFilePath = droppedFilePath
                    core.openLogFile(droppedFilePath, false, true)
                    drop.accept()
                }
                droppedFilePath = ""
            }

            Rectangle {
                anchors.fill: parent
                color: AppPalette.accentBar
                opacity: fileDropArea.containsDrag && fileDropArea.droppedFilePath !== "" ? 0.18 : 0.0
                visible: opacity > 0.001
                border.width: Math.max(1, Math.round(2 * AppPalette.scale))
                border.color: AppPalette.accentBar
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }
        }

        Connections {
            target: workspaceStore
            function onSettingsPanelOpenChanged() {
                if (workspaceStore.settingsPanelOpen) {
                    hotActions.expanded = false
                    hotActions.layoutsMenuOpen = false
                    if (typeof core !== "undefined" && core) core.requestDismissTransientUi()
                }
            }
            function onModeSettingsPanelOpenChanged() {
                if (workspaceStore.modeSettingsPanelOpen) {
                    hotActions.expanded = false
                    hotActions.layoutsMenuOpen = false
                    if (typeof core !== "undefined" && core) core.requestDismissTransientUi()
                }
            }
            function onActiveLeafIdChanged() {
                if (typeof core !== "undefined" && core) core.requestDismissTransientUi()
            }
            function onInputLockedChanged() {
                if (workspaceStore.inputLocked)
                    root.closeAllTransientUi()
                mainLayer.inputLockKeyDown = false
                mainLayer.forceActiveFocus()
            }
        }

        MouseArea {
            anchors.fill: parent
            z: hotActions.z - 1
            visible: hotActions.expanded && hotActions.showToggleButton
            enabled: hotActions.expanded && hotActions.showToggleButton
            acceptedButtons: Qt.AllButtons
            hoverEnabled: false
            onPressed: function(mouse) {
                hotActions.expanded = false
                hotActions.layoutsMenuOpen = false
                mouse.accepted = false
            }
        }

        HotActionsPanel {
            id: hotActions

            visible: !workspaceStore.settingsPanelOpen
                     && !workspaceStore.modeSettingsPanelOpen
                     || hotkeysPreviewMode
                     || hotkeysPreviewPinned
                     || hotkeysPreviewSticky
                     || workspaceStore.inputLocked

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: ((hotkeysPreviewPinned || hotkeysPreviewSticky) && workspaceStore.settingsSide === "left")
                                ? Math.round(workspaceStore.settingsPanelSizePx * settingsSidebar.progress) + root.hotkeysPreviewGap
                                : 8
            anchors.topMargin: 8
            z: workspaceStore.inputLocked
               ? ZOrder.inputLockPanel
               : hotkeysPreviewMode || hotkeysPreviewSticky || (workspaceStore.settingsPanelOpen && hotActions.expanded)
               ? ZOrder.hotActionsActive
               : ZOrder.hotActions

            store: workspaceStore
            layoutsEnabled: workspaceStore.quickActionLayoutsEnabled
            connectionStatusToolVisible: workspaceStore.quickActionConnectionStatusEnabled
            loggingButtonEnabled: workspaceStore.quickActionLoggingEnabled
            secondWindowButtonEnabled: workspaceStore.quickActionSecondWindowEnabled
            layoutEditing: root.hotkeysPreviewSticky
            bottomTrackEditorEnabled: workspaceStore.quickActionBottomTrackEnabled
            profilesEnabled: workspaceStore.quickActionProfilesEnabled
            widgetsEnabled: workspaceStore.quickActionWidgetsEnabled
            consoleButtonEnabled: workspaceStore.quickActionConsoleEnabled
            powerOffEnabled: workspaceStore.quickActionPowerOffEnabled
            onPowerOffTriggered: powerOffOverlay.active = true
            inputLockEnabled: workspaceStore.quickActionInputLockEnabled
            inputLocked: workspaceStore.inputLocked
            inputLockKeyHeld: mainLayer.inputLockKeyDown
            inputDeviceLabel: workspaceView.inputDeviceLabel
            inputDeviceColor: workspaceView.inputDeviceColor
            showToggleButton: !workspaceStore.settingsPanelOpen && !workspaceStore.modeSettingsPanelOpen
            revealShiftX: 0

            onSettingsTriggered: {
                legacyPanelOpen = false
                workspaceStore.toggleAppLayoutSettings()
            }

            onConnectionsTriggered: {
                legacyPanelOpen = false
                workspaceStore.toggleConnectionsSettings()
                refreshConnectionsIndicator()
            }

            onLoggingIndicatorTriggered: {
                legacyPanelOpen = false
                workspaceStore.openRecordingSettings()
                refreshConnectionsIndicator()
            }

            onOpenFileTriggered: openSelectedFile()
            onCloseFileTriggered: closeSelectedFile()
            onUpdateBottomTrackTriggered: updateBottomTrackForRegisteredPlots()
            onUpdateMosaicTriggered: updateMosaicProcessing()
            onMode3DTriggered: setActivePaneMode("3D")
            onMode2DTriggered: setActivePaneMode("2D")

            onLegacyRequested: {
                legacyPanelOpen = false
            }

            secondWindowOpen: workspaceStore.secondaryWindowOpen
            onSecondWindowToggleRequested: {
                if (workspaceStore.secondaryWindowOpen)
                    workspaceStore.closeSecondaryWindow()
                else
                    workspaceStore.openSecondaryWindow()
            }

            devices: deviceManagerWrapper ? deviceManagerWrapper.devs : []
            onDeviceTriggered: function(devIndex) {
                workspaceStore.openDeviceSettingsForIndex(devIndex)
                refreshConnectionsIndicator()
            }
        }

        Timer {
            id: hotkeysRevealCloseTimer
            interval: 1600
            repeat: false
            onTriggered: {
                workspaceStore.hotkeysRevealKey = ""
                hotActions.clearQuickActionReveal()
                if (root.hotkeysPreviewSticky)
                    return
                hotkeysPreviewClosing = true
                hotActions.expanded = false
                hotkeysRevealUnpinTimer.restart()
            }
        }

        Timer {
            id: hotkeysRevealUnpinTimer
            interval: 260
            repeat: false
            onTriggered: {
                hotkeysPreviewClosing = false
            }
        }

        Timer {
            id: workspaceResizeCommitTimer
            interval: workspaceStore.workspaceResizeDebounceMs
            repeat: false
            onTriggered: workspaceStore.commitWorkspaceSize()
        }

        Connections {
            target: linkManagerWrapper ? linkManagerWrapper.linkListModel : null
            ignoreUnknownSignals: true

            function onDataChanged() {
                root.refreshConnectionsIndicator()
            }

            function onRowsInserted() {
                root.refreshConnectionsIndicator()
            }

            function onRowsRemoved() {
                root.refreshConnectionsIndicator()
            }

            function onModelReset() {
                root.refreshConnectionsIndicator()
            }
        }

        Connections {
            target: core
            ignoreUnknownSignals: true

            function onConnectionChanged() {
                root.refreshConnectionsIndicator()
            }

            function onFilePathChanged() {
                if (core && core.filePath && core.filePath.length > 0)
                    workspaceStore.selectedConnectionFilePath = core.filePath
            }
        }

        Connections {
            target: workspaceStore
            ignoreUnknownSignals: true

            function onWorkspaceSizeCommitRequested() {
                workspaceResizeCommitTimer.restart()
            }
        }

        Connections {
            target: workspaceStore
            ignoreUnknownSignals: true

            function onHotkeysRevealNonceChanged() {
                hotActions.revealQuickAction(workspaceStore.hotkeysRevealKey)
                hotkeysPreviewClosing = false
                hotkeysRevealUnpinTimer.stop()
                hotkeysRevealActivateTimer.restart()
                hotkeysRevealCloseTimer.restart()
            }
        }

        Timer {
            id: hotkeysRevealActivateTimer
            interval: 300
            repeat: false
            onTriggered: hotActions.pulseRevealedAction()
        }

        SettingsSidebarBase {
            id: settingsSidebar
            z: ZOrder.settingsSidebar

            anchors.fill: parent
            open: workspaceStore.settingsPanelOpen
            onOpenChanged: if (open) root._lastScrollSurface = "settings"
            onInteracted: root._lastScrollSurface = "settings"
            dimEnabled: !workspaceStore.effectivePushContent
            panelShadowEnabled: !workspaceStore.editableMode
            title: workspaceStore.echogramSettingsActive
                   ? workspaceStore.echogramSettingsTitle
                   : !workspaceStore.settingsSubPageActive
                     ? qsTr("Settings")
                     : workspaceStore.settingsSubPageKind === "quickActions" ? qsTr("Quick action menu")
                     : workspaceStore.settingsSubPageKind === "widgetEdit"   ? (workspaceStore.widgetEditIndex >= 0 ? qsTr("Edit panel") : qsTr("Create panel"))
                     : workspaceStore.settingsSubPageKind === "uiSaving"     ? qsTr("UI Saving")
                     : workspaceStore.settingsSubPageKind === "tgc"          ? qsTr("TGC")
                     : workspaceStore.settingsSubPageKind === "csvExport"    ? qsTr("Export to CSV")
                     : workspaceStore.settingsSubPageKind === "aimPanel"     ? qsTr("Information panel")
                     : workspaceStore.settingsSubPageKind === "console"      ? qsTr("Console")
                     : workspaceStore.settingsSubPageKind === "createLayout" ? qsTr("Create layout")
                     : workspaceStore.settingsSubPageKind === "devices"      ? qsTr("Devices")
                     : qsTr("Settings")
            side: workspaceStore.settingsSide
            gearMode: "app"
            panelSizePx: workspaceStore.settingsPanelSizePx
            store: workspaceStore
            onCloseRequested: workspaceStore.settingsPanelOpen = false

            showBack: workspaceStore.anySettingsSubPageActive
            onBackRequested: {
                if (workspaceStore.settingsSubPageActive
                    && workspaceStore.settingsSubPageKind === "widgetEdit"
                    && workspaceStore.widgetEditStep === 2)
                    workspaceStore.widgetEditStep = 1
                else
                    workspaceStore.closeActiveSettingsSubPage()
            }

            subPage: workspaceStore.settingsSubPageKind === "quickActions" ? quickActionsSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "widgetEdit" ? widgetEditTabComponent
                     : workspaceStore.settingsSubPageKind === "uiSaving"   ? uiSavingSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "tgc"        ? tgcSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "csvExport"  ? csvExportSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "aimPanel"   ? aimPanelSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "console"    ? consoleSettingsTabComponent
                     : workspaceStore.settingsSubPageKind === "createLayout" ? layoutCreateTabComponent
                     : workspaceStore.settingsSubPageKind === "devices"      ? deviceSettingsTabComponent
                     : echogramSettingsTabComponent
            subPageOpen: workspaceStore.anySettingsSubPageActive

            Loader {
                width: parent.width
                active: true
                asynchronous: true
                sourceComponent: appSettingsPageComponent
            }
        }

        ModeSettingsPanel {
            id: modeSettingsPanel
            z: ZOrder.modeSettings

            anchors.fill: parent
            store: workspaceStore
        }

        WidgetEditOverlay {
            anchors.fill: parent
            z: ZOrder.widgetEditorOverlay
            store: workspaceStore
        }

        Item {
            id: widgetEditorDragLayer
            anchors.fill: parent
            z: ZOrder.widgetEditorDrag
            Component.onCompleted: workspaceStore.widgetDragLayer = widgetEditorDragLayer
        }

        Binding {
            target: workspaceStore
            property: "pointerOverSidebar"
            value: settingsSidebar.pointerInside || modeSettingsPanel.pointerInside
        }

        Loader {
            id: globalPopupLoader

            anchors.fill: parent
            z: ZOrder.globalPopup
            active: workspaceStore.globalPopupEnabled
            sourceComponent: GlobalPanePopup {
                anchors.fill: parent
                store: workspaceStore
                workspaceRoot: workspaceView
                popupId: "global"
                siblingBoundsList: [root.fullscreenPopupEffectiveBounds]
                siblingIdList: ["fullscreen"]
            }
        }

        FullscreenPanePopup {
            id: fullscreenPanePopup
            anchors.fill: parent
            z: ZOrder.fullscreenPopup
            store: workspaceStore
            workspaceRoot: workspaceView
            hostLeafId: workspaceStore.maximizedLeafId
            sourceLeafId: workspaceStore.popupSourceLeafIdForHost(hostLeafId)
            visible: hostLeafId !== -1 && sourceLeafId !== -1
            popupId: "fullscreen"
            siblingBoundsList: [root.globalPopupEffectiveBounds]
            siblingIdList: ["global"]
        }

        BottomTrackEditPopup {
            id: btEditPopup
            anchors.fill: parent
            z: ZOrder.bottomTrackEditPopup   // поверх глобал/фуллскрин попапов
            store: workspaceStore
            popupId: "btEdit"
            siblingBoundsList: [root.profilesPopupEffectiveBounds]
            siblingIdList: ["profiles"]
        }

        ProfilesPopup {
            id: profilesPopup
            anchors.fill: parent
            z: ZOrder.profilesPopup
            store: workspaceStore
            popupId: "profiles"
            siblingBoundsList: [root.btEditPopupEffectiveBounds]
            siblingIdList: ["btEdit"]
        }

        // One delegate per panel, of whichever KIND the def names. The Loader is the branch: a
        // field grid and an acoustic-nodes list are different popups that happen to share the
        // whole of the panel machinery around them — position, scale, z-rank, docking, the
        // shown map — which is exactly why the branch is here and not inside either of them.
        Repeater {
            id: widgetsRepeater
            model: workspaceStore.widgets.length
            delegate: Item {
                id: widgetSlot
                required property int index
                readonly property var _wdef: workspaceStore.widgets[index] || null
                anchors.fill: parent
                z: ZOrder.widgetPopup + (_wdef ? workspaceStore.widgetStackRank(_wdef.id) : 0)

                // uiStateReapplied re-syncs every panel by walking the Repeater's items; the
                // Loader now sits between, so this has to forward rather than swallow the call.
                function syncFromStore() { if (slotLoader.item) slotLoader.item.syncFromStore() }

                Loader {
                    id: slotLoader
                    anchors.fill: parent
                    sourceComponent: !widgetSlot._wdef ? dataWidgetPanelComp
                                     : widgetSlot._wdef.kind === "usblNodes" ? usblNodesPanelComp
                                     : widgetSlot._wdef.kind === "stand"     ? standPanelComp
                                                                            : dataWidgetPanelComp
                }

                // BOTH COMPONENTS LIVE INSIDE THE DELEGATE, and they have to. An object created
                // from a Component gets that Component's creation context; declared beside the
                // Repeater instead, `widgetSlot` is not in scope and every binding reading it
                // is a runtime ReferenceError -- a panel that loads and then paints nothing.
                Component {
                    id: dataWidgetPanelComp
                    DataWidgetPopup {
                        readonly property var _wdef: widgetSlot._wdef
                        anchors.fill: parent
                        store: workspaceStore
                        def: _wdef
                        popupVisible: !!_wdef && !_beingEdited && workspaceStore.widgetShown(_wdef.id)
                        popupId: _wdef ? "widget:" + _wdef.id : ""
                        siblingBoundsList: [root.btEditPopupEffectiveBounds, root.profilesPopupEffectiveBounds]
                        siblingIdList: ["btEdit", "profiles"]
                    }
                }

                Component {
                    id: standPanelComp
                    StandPanelPopup {
                        readonly property var _wdef: widgetSlot._wdef
                        anchors.fill: parent
                        store: workspaceStore
                        def: _wdef
                        // The panel acts on ONE device, unlike the other kinds, which read
                        // device-agnostic caches. Without a stand-capable one it says so.
                        dev: workspaceStore.standDevice
                        popupVisible: !!_wdef && workspaceStore.standAvailable
                                      && workspaceStore.widgetShown(_wdef.id)
                        popupId: _wdef ? "widget:" + _wdef.id : ""
                        siblingBoundsList: [root.btEditPopupEffectiveBounds, root.profilesPopupEffectiveBounds]
                        siblingIdList: ["btEdit", "profiles"]
                    }
                }

                Component {
                    id: usblNodesPanelComp
                    UsblNodesPopup {
                        readonly property var _wdef: widgetSlot._wdef
                        anchors.fill: parent
                        store: workspaceStore
                        engine: appUsblEngine
                        def: _wdef
                        // No _beingEdited term: this panel has no on-scene editor to hide
                        // behind, so hiding it while its (text-only) settings page is open
                        // would blank the very thing being configured.
                        popupVisible: !!_wdef && workspaceStore.widgetShown(_wdef.id)
                        popupId: _wdef ? "widget:" + _wdef.id : ""
                        siblingBoundsList: [root.btEditPopupEffectiveBounds, root.profilesPopupEffectiveBounds]
                        siblingIdList: ["btEdit", "profiles"]
                    }
                }
            }
        }

        Connections {
            target: workspaceStore
            function onUiStateReapplied() {
                if (globalPopupLoader.item) globalPopupLoader.item.syncFromStore()
                fullscreenPanePopup.syncFromStore()
                btEditPopup.syncFromStore()
                profilesPopup.syncFromStore()
                for (var i = 0; i < widgetsRepeater.count; ++i) {
                    var it = widgetsRepeater.itemAt(i)
                    if (it) it.syncFromStore()
                }
            }
        }

        Component {
            id: echogramSettingsTabComponent

            EchogramSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: quickActionsSettingsTabComponent

            QuickActionSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: widgetEditTabComponent

            WidgetEditPage {
                store: workspaceStore
            }
        }

        Component {
            id: uiSavingSettingsTabComponent

            UiSavingSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: tgcSettingsTabComponent

            TgcSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: csvExportSettingsTabComponent

            CsvExportSettingsTab {
                store: workspaceStore
                targetPlot: workspaceView.primaryPlotItem
            }
        }

        Component {
            id: consoleSettingsTabComponent

            ConsoleSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: aimPanelSettingsTabComponent

            AimPanelSettingsTab {
                store: workspaceStore
            }
        }

        Component {
            id: layoutCreateTabComponent

            LayoutCreatePage {
                store: workspaceStore
            }
        }

        Component {
            id: deviceSettingsTabComponent

            DeviceSettingsTab {
                store: workspaceStore
                // NOT `usblPlan: usblPlan` -- property lookup finds the object's OWN property
                // before any id, so that binds the property to itself. Hence the app- prefix
                // on the ids.
                usblPlan: appUsblPlan
                usblEngine: appUsblEngine
            }
        }

        Component {
            id: appSettingsPageComponent

            AppSettingsPage {
                store: workspaceStore
                targetPlot: workspaceView.primaryPlotItem
                echograms: workspaceView.visibleEchograms
            }
        }

        WorkspaceView {
            id: workspaceView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.settingsInsetLeft
            anchors.rightMargin: root.settingsInsetRight
            anchors.bottomMargin: consoleDrawer.height
            store: workspaceStore
            usblPlan: appUsblPlan
            secondaryPlotItem: secondaryContent ? secondaryContent.plot2DInstance : null
        }

        ConsolePanelDrawer {
            id: consoleDrawer
            store: workspaceStore
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.settingsInsetLeft
            anchors.rightMargin: root.settingsInsetRight
            z: ZOrder.consolePanel
            consoleOpen: theme ? theme.consoleVisible : false
            onConsoleOpenChanged: if (consoleOpen) root._lastScrollSurface = "console"
            onInteracted: root._lastScrollSurface = "console"
            maxHeight: parent.height
            hotActionsRight: hotActions.visible ? hotActions.x + hotActions.width : 0
        }

        NotificationsOverlay {
            hideImportant: workspaceStore.hideImportantNotifications
        }

        FileOpeningOverlay { }

        SplashOverlay { }

        PowerOffConfirmOverlay {
            id: powerOffOverlay
            onConfirmed: if (typeof core !== "undefined" && core) core.powerOffSystem()
        }

        MouseArea {
            id: inputLockSwallower
            anchors.fill: parent
            z: ZOrder.inputLockOverlay
            visible: workspaceStore.inputLocked
            enabled: visible
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            preventStealing: true
            propagateComposedEvents: false
            onPressed:       function(mouse) { mouse.accepted = true }
            onReleased:      function(mouse) { mouse.accepted = true }
            onClicked:       function(mouse) { mouse.accepted = true }
            onDoubleClicked: function(mouse) { mouse.accepted = true }
            onPressAndHold:  function(mouse) { mouse.accepted = true }
            onWheel:         function(wheel) { wheel.accepted = true }
        }

        Rectangle {
            id: welcomeOverlay
            anchors.fill: parent
            z: ZOrder.welcomeOverlay
            color: "#000000B0"
            visible: !welcomeSettings.welcomeShown

            Settings { id: welcomeSettings; category: "main/ui"; property bool welcomeShown: false }

            function finish() {
                welcomeSettings.welcomeShown = true
                welcomeOverlay.visible = false
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons
                onWheel: function(wheel) { wheel.accepted = true }
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(Math.round(520 * AppPalette.scale), parent.width - Math.round(40 * AppPalette.scale))
                implicitHeight: welcomeCol.implicitHeight + 2 * Tokens.spaceXl
                height: implicitHeight
                radius: Tokens.radiusLg
                color: AppPalette.card
                border.width: 1
                border.color: AppPalette.border

                Column {
                    id: welcomeCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Tokens.spaceXl
                    spacing: Tokens.spaceLg

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: "qrc:/icons/app/kogger_app.png"
                        sourceSize.width: Math.round(64 * AppPalette.scale)
                        sourceSize.height: Math.round(64 * AppPalette.scale)
                        width: Math.round(64 * AppPalette.scale)
                        height: width
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Text {
                        width: parent.width
                        text: qsTr("Welcome to KoggerApp!")
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontXl
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        width: parent.width
                        text: qsTr("Language:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }
                    KTabBar {
                        width: parent.width
                        options: [
                            { label: "English", value: 0 },
                            { label: "Русский", value: 1 },
                            { label: "Polski",  value: 2 }
                        ]
                        currentValue: langController ? langController.currentIndex : 0
                        onValueSelected: function(v) { if (langController) langController.apply(v) }
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Choose which settings to show")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }
                    KTabBar {
                        width: parent.width
                        options: [
                            { label: qsTr("Fish Finders"), value: 0 },
                            { label: qsTr("Bottom Track"), value: 1 },
                            { label: qsTr("Maximum"),      value: 2 }
                        ]
                        currentValue: theme ? theme.instrumentsGrade : 0
                        onValueSelected: function(v) { if (theme) theme.instrumentsGrade = v }
                    }

                    Text {
                        width: parent.width
                        text: qsTr("You can change this later in settings.")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        topPadding: Tokens.spaceXs
                    }

                    KButton {
                        width: parent.width
                        text: qsTr("Continue")
                        normalBg: AppPalette.accentBgStrong
                        normalBorder: AppPalette.accentBorder
                        hoverBg: AppPalette.accentBorder
                        hoverBorder: AppPalette.accentBorder
                        onClicked: welcomeOverlay.finish()
                    }
                }
            }
        }
    }
}

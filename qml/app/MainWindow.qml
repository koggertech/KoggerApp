import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtCore
import kqml_types 1.0

ApplicationWindow {
    id: root

    readonly property bool isMobilePlatform: Qt.platform.os === "android" || Qt.platform.os === "ios"

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

        onSurfaceLayersRefreshRequested: updateBottomTrackForRegisteredPlots()
        Component.onCompleted: initLayerVisibilityControllers()
    }

    // Читаем глобальные настройки при запуске (те же ключи, что сохраняет AppSettingsPage)
    Settings {
        id: startupSettings
        property int appTheme: 0
        property int instrumentsGradeList: 0
        property bool consoleVisible: false
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
        function onInterfaceChanged()        { startupSettings.consoleVisible     = theme.consoleVisible }
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
                                                    || hotkeysRevealHideTimer.running
                                                    || hotkeysRevealCloseTimer.running
                                                    || hotkeysPreviewClosing)
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
            workspaceStore.clearModePickerSelection()
            return true
        },
        function() {  // pane mode settings
            if (!workspaceStore.modeSettingsPanelOpen) return false
            workspaceStore.closeModeSettingsPanel()
            return true
        },
        function() {  // HotActions favorites popup
            if (!hotActions.layoutsMenuOpen) return false
            hotActions.layoutsMenuOpen = false
            return true
        },
        function() {  // HotActions expanded
            if (!hotActions.expanded) return false
            hotActions.expanded = false
            return true
        },
        function() {  // legacy main panel
            if (!legacyPanelOpen) return false
            legacyPanelOpen = false
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
        onActivated: {
            if (root.isTextInputFocused())
                return
            root.closeTransientUi()
        }
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

        workspaceStore.openConnectionsSettings()
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

        if (fn === "clickConnections") {
            legacyPanelOpen = false
            workspaceStore.toggleConnectionsSettings()
            return true
        }
        if (fn === "clickSettings") {
            legacyPanelOpen = false
            workspaceStore.toggleAppLayoutSettings()
            return true
        }
        if (fn === "click3D")
            return setActivePaneMode("3D")
        if (fn === "click2D")
            return setActivePaneMode("2D")

        return false
    }

    function handleHotkeyKeyEvent(event) {
        // Esc handled by ApplicationShortcut — skip legacy hotkey (scanCode 1 → "closeSettings").
        if (event && event.key === Qt.Key_Escape)
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

        // Android hardware back.
        if (event.key === Qt.Key_Back)
            return closeTransientUi()
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
        workspaceStore.saveLayoutState()
    }

    Component.onDestruction: workspaceStore.saveLayoutState()
    Component.onCompleted: {
        // Применяем сохранённые глобальные настройки сразу при запуске
        if (theme) {
            theme.themeID          = startupSettings.appTheme
            theme.instrumentsGrade = startupSettings.instrumentsGradeList
            theme.consoleVisible   = startupSettings.consoleVisible
        }

        refreshConnectionsIndicator()
        if ((!workspaceStore.selectedConnectionFilePath || workspaceStore.selectedConnectionFilePath.length === 0)
                && core && core.filePath && core.filePath.length > 0) {
            workspaceStore.selectedConnectionFilePath = core.filePath
        }
    }

    Item {
        id: mainLayer
        anchors.fill: parent
        focus: true

        Component.onCompleted: forceActiveFocus()

        Keys.onReleased: function(event) {
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

        Connections {
            target: workspaceStore
            function onSettingsPanelOpenChanged() {
                if (workspaceStore.settingsPanelOpen) {
                    hotActions.expanded = false
                    hotActions.layoutsMenuOpen = false
                }
            }
            function onModeSettingsPanelOpenChanged() {
                if (workspaceStore.modeSettingsPanelOpen) {
                    hotActions.expanded = false
                    hotActions.layoutsMenuOpen = false
                }
            }
        }

        HotActionsPanel {
            id: hotActions

            visible: !workspaceStore.settingsPanelOpen
                     && !workspaceStore.modeSettingsPanelOpen
                     || hotkeysPreviewMode
                     || hotkeysPreviewPinned

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: (hotkeysPreviewPinned && workspaceStore.settingsSide === "left")
                                ? Math.round(workspaceStore.settingsPanelSizePx * settingsSidebar.progress) + root.hotkeysPreviewGap
                                : 12
            anchors.topMargin: 12
            z: hotkeysPreviewMode || (workspaceStore.settingsPanelOpen && hotActions.expanded)
               ? ZOrder.hotActionsActive
               : ZOrder.hotActions

            store: workspaceStore
            favoritesEnabled: workspaceStore.quickActionFavoritesEnabled
            connectionStatusToolVisible: workspaceStore.quickActionConnectionStatusEnabled
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

            onOpenFileTriggered: openSelectedFile()
            onCloseFileTriggered: closeSelectedFile()
            onUpdateBottomTrackTriggered: updateBottomTrackForRegisteredPlots()
            onUpdateMosaicTriggered: updateMosaicProcessing()
            onMode3DTriggered: setActivePaneMode("3D")
            onMode2DTriggered: setActivePaneMode("2D")

            onLegacyRequested: {
                workspaceStore.settingsPanelOpen = false
                if (workspaceStore.modeSettingsPanelOpen) {
                    workspaceStore.closeModeSettingsPanel()
                }
                legacyPanelOpen = !legacyPanelOpen
            }

            secondWindowOpen: workspaceStore.secondaryWindowOpen
            onSecondWindowToggleRequested: {
                if (workspaceStore.secondaryWindowOpen)
                    workspaceStore.closeSecondaryWindow()
                else
                    workspaceStore.openSecondaryWindow()
            }

            devices: deviceManagerWrapper ? deviceManagerWrapper.devs : []
            onDeviceTriggered: function(sn) {
                workspaceStore.openConnectionsWithDevice(sn)
                refreshConnectionsIndicator()
            }
        }

        Timer {
            // Pulse runs 90+180+280 = 550ms — keep the highlight a bit longer
            // so the icons sit visibly after the flash before fading out.
            id: hotkeysRevealHideTimer
            interval: 800
            repeat: false
            onTriggered: {
                workspaceStore.hotkeysRevealKey = ""
                hotActions.clearQuickActionReveal()
                hotkeysRevealCloseTimer.restart()
            }
        }

        Timer {
            // Pause after icons vanish (hide-timer cleared the override) and
            // before the panel itself collapses. Gives the user a clear beat
            // to see "the things I just toggled are gone" — then menu closes.
            id: hotkeysRevealCloseTimer
            interval: 450
            repeat: false
            onTriggered: {
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

            // Reveal sequence:
            //   t=0      → panel starts expanding + icons rendered (revealQuickAction)
            //   t=300ms  → icons start pulsing (pulseRevealedAction)
            //   t=300+800=1100ms → highlight + override cleared (revealHideTimer):
            //                       disabled icons VANISH while panel is still open
            //   t=1100+450=1550ms → panel collapses (revealCloseTimer)
            //                        — gives the user a clear beat to see
            //                        "the toggled items are gone" before close.
            function onHotkeysRevealNonceChanged() {
                hotActions.revealQuickAction(workspaceStore.hotkeysRevealKey)
                hotkeysPreviewClosing = false
                hotkeysRevealUnpinTimer.stop()
                hotkeysRevealCloseTimer.stop()
                hotkeysRevealHideTimer.stop()
                hotkeysRevealActivateTimer.restart()
            }
        }

        Timer {
            id: hotkeysRevealActivateTimer
            interval: 300
            repeat: false
            onTriggered: {
                hotActions.pulseRevealedAction()
                hotkeysRevealHideTimer.restart()
            }
        }

        SettingsSidebarBase {
            id: settingsSidebar
            z: ZOrder.settingsSidebar

            anchors.fill: parent
            open: workspaceStore.settingsPanelOpen
            dimEnabled: !workspaceStore.effectivePushContent
            panelShadowEnabled: !workspaceStore.editableMode
            title: qsTr("Settings")
            side: workspaceStore.settingsSide
            gearMode: "app"
            panelSizePx: workspaceStore.settingsPanelSizePx
            store: workspaceStore
            onCloseRequested: workspaceStore.settingsPanelOpen = false

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

        SettingsSidebarBase {
            id: legacySidebar
            z: ZOrder.legacySidebar

            anchors.fill: parent
            open: legacyPanelOpen
            dimEnabled: true
            panelShadowEnabled: !workspaceStore.editableMode
            title: "Legacy menus"
            side: workspaceStore.settingsSide
            gearMode: "app"
            panelSizePx: 560
            store: workspaceStore
            onCloseRequested: legacyPanelOpen = false

            Loader {
                width: parent.width
                active: legacySidebar.progress > 0.01 || legacyPanelOpen
                sourceComponent: legacyMenuComponent
            }
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
                siblingBounds: root.fullscreenPopupEffectiveBounds
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
            siblingBounds: root.globalPopupEffectiveBounds
        }

        Component {
            id: legacyMenuComponent

            Item {
                anchors.fill: parent

                Loader {
                    id: legacyMenuLoader
                    anchors.fill: parent
                    source: "qrc:/qml/menus/MainMenuBar.qml"

                    onLoaded: {
                        if (item)
                            item.targetPlot = workspaceView.primaryPlotItem
                    }
                }

                Connections {
                    target: workspaceView
                    ignoreUnknownSignals: true

                    function onPrimaryPlotItemChanged() {
                        if (legacyMenuLoader.item)
                            legacyMenuLoader.item.targetPlot = workspaceView.primaryPlotItem
                    }
                }
            }
        }

        Component {
            id: appSettingsPageComponent

            AppSettingsPage {
                store: workspaceStore
                targetPlot: workspaceView.primaryPlotItem
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
        }

        ConsolePanelDrawer {
            id: consoleDrawer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.settingsInsetLeft
            anchors.rightMargin: root.settingsInsetRight
            z: ZOrder.consolePanel
            consoleOpen: theme ? theme.consoleVisible : false
            maxHeight: parent.height
            hotActionsRight: hotActions.x + hotActions.width
        }

        FileOpeningOverlay { }
    }
}

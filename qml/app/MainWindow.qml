import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import kqml_types 1.0

ApplicationWindow {
    id: root

    readonly property bool isMobilePlatform: Qt.platform.os === "android" || Qt.platform.os === "ios"

    width: isMobilePlatform ? Screen.width : 1200
    height: isMobilePlatform ? Screen.height : 760
    minimumWidth: isMobilePlatform ? 0 : 900
    minimumHeight: isMobilePlatform ? 0 : 560
    visible: true
    visibility: isMobilePlatform ? Window.FullScreen : Window.Windowed
    title: "Qt 6.8.3 | Multi Pane Workspace"

    WorkspaceStore {
        id: workspaceStore

        windowWidth: root.width
        windowHeight: root.height

        onWindowGeometryRestoreRequested: function(width, height) {
            if (root.isMobilePlatform)
                return
            root.width = width
            root.height = height
        }
    }

    readonly property real anySettingsProgress: Math.max(settingsSidebar.progress, modeSettingsPanel.sidebarProgress)
    readonly property real settingsInsetLeft: workspaceStore.settingsPushContent && workspaceStore.settingsSide === "left"
                                             ? anySettingsProgress * workspaceStore.settingsPanelSizePx : 0
    readonly property real settingsInsetRight: workspaceStore.settingsPushContent && workspaceStore.settingsSide === "right"
                                              ? anySettingsProgress * workspaceStore.settingsPanelSizePx : 0
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

    function closeTransientUi() {
        var handled = false

        if (legacyPanelOpen) {
            legacyPanelOpen = false
            handled = true
        }
        if (workspaceStore.modeSettingsPanelOpen) {
            workspaceStore.closeModeSettingsPanel()
            handled = true
        }
        if (workspaceStore.settingsPanelOpen) {
            workspaceStore.settingsPanelOpen = false
            handled = true
        }
        if (workspaceStore.modePickerLeafId !== -1) {
            workspaceStore.clearModePickerSelection()
            handled = true
        }
        if (hotActions.layoutsMenuOpen) {
            hotActions.layoutsMenuOpen = false
            handled = true
        }
        if (hotActions.expanded) {
            hotActions.expanded = false
            handled = true
        }

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

        if (fn === "toggleFullScreen")
            return toggleFullScreenMode()
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

        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back)
            return closeTransientUi()
        if (event.key === Qt.Key_F11)
            return toggleFullScreenMode()
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
            x: root.settingsInsetLeft
            y: 0
            width: root.width - root.settingsInsetLeft - root.settingsInsetRight
            height: root.height
            color: "#0B1220"
        }

        HotActionsPanel {
            id: hotActions

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
            markerToolVisible: workspaceStore.quickActionMarkerEnabled
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
        }

        Timer {
            id: hotkeysRevealHideTimer
            interval: 900
            repeat: false
            onTriggered: {
                workspaceStore.hotkeysRevealKey = ""
                hotActions.clearQuickActionReveal()
                hotkeysRevealCloseTimer.restart()
            }
        }

        Timer {
            id: hotkeysRevealCloseTimer
            interval: 650
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

            function onHotkeysRevealNonceChanged() {
                hotActions.revealQuickAction(workspaceStore.hotkeysRevealKey)
                hotkeysPreviewClosing = false
                hotkeysRevealUnpinTimer.stop()
                hotkeysRevealCloseTimer.stop()
                hotkeysRevealHideTimer.restart()
            }
        }

        SettingsSidebarBase {
            id: settingsSidebar
            z: ZOrder.settingsSidebar

            anchors.fill: parent
            open: workspaceStore.settingsPanelOpen
            dimEnabled: !workspaceStore.settingsPushContent
            panelShadowEnabled: !workspaceStore.editableMode
            title: "Settings"
            side: workspaceStore.settingsSide
            gearMode: "app"
            panelSizePx: workspaceStore.settingsPanelSizePx
            onCloseRequested: workspaceStore.settingsPanelOpen = false

            Loader {
                width: parent.width
                active: settingsSidebar.progress > 0.01 || workspaceStore.settingsPanelOpen
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
            }
        }

        FullscreenPanePopup {
            anchors.fill: parent
            z: ZOrder.fullscreenPopup
            store: workspaceStore
            workspaceRoot: workspaceView
            hostLeafId: workspaceStore.maximizedLeafId
            sourceLeafId: workspaceStore.popupSourceLeafIdForHost(hostLeafId)
            visible: hostLeafId !== -1 && sourceLeafId !== -1
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
            store: workspaceStore
        }
    }
}

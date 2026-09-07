pragma Singleton
import QtQuick 2.15

QtObject {
    // ── WorkspaceView level ──────────────────────────────────────────────────
    readonly property int workspacePane:       0    // normal pane tile
    readonly property int dropZoneHighlight:  45    // drag-over highlight ring
    readonly property int dragOverlay:       100    // drag ghost / target overlay
    readonly property int maximizingPane:    130    // pane entering mode-select animation
    readonly property int maximizedPane:     140    // maximized/fullscreen pane tile

    // ── MainWindow level ─────────────────────────────────────────────────────
    readonly property int consolePanel:       10    // bottom console drawer
    readonly property int hotActionsActive: 2205    // HotActionsPanel when settings open

    readonly property int fullscreenPopup:  1300    // FullscreenPanePopup (per-pane)
    readonly property int globalPopup:      1400    // GlobalPanePopup (floating window)
    readonly property int bottomTrackEditPopup: 1500 // bottom-track tool palette (above window popups)
    readonly property int profilesPopup:        1510 // settings-profile palette (tool level, with bt-edit)
    readonly property int widgetPopup:           1520 // panels — base of the reserved stack band 1520..1539; most-recently-moved on top. 20 slots for WorkspaceStore.widgetStackSlots (widgetLimit 10 + the 3 pinned domain panels), with room left so a fourth pinned panel does not need this file changed
    readonly property int hotActions:       1540    // HotActionsPanel — above the whole panel band + tool palettes so its buttons + hotkey dropdowns stay on top and clickable
    readonly property int widgetEditorOverlay:   1900 // widget editor: dim + real widget (below settings sidebar)

    readonly property int settingsSidebar:  2000    // AppSettings sidebar
    readonly property int modeSettings:     2001    // ModeSettingsPanel
    readonly property int widgetEditorDrag: 2500    // widget editor: floating drag layer (above settings)

    readonly property int notificationsOverlay: 4000 // toast notifications (top-center stack)
    readonly property int fileOpeningOverlay: 5000  // modal "file is opening" banner
    readonly property int splashOverlay:    6000    // startup launch banner (fades out)
    readonly property int welcomeOverlay:   6500    // first-launch welcome / toolset picker (modal)
    readonly property int powerOffOverlay:  6700    // slide-to-confirm power off (modal, Ubuntu)
    readonly property int inputLockOverlay: 6800    // input lock: full-window event swallower (above every other overlay)
    readonly property int inputLockPanel:   6810    // HotActionsPanel while locked — the only surface above the swallower
}

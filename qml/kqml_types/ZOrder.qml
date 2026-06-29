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
    readonly property int hotActions:       1450    // HotActionsPanel — above window popups so its gear stays clickable
    readonly property int bottomTrackEditPopup: 1500 // bottom-track tool palette (always on top)
    readonly property int profilesPopup:        1510 // settings-profile palette (tool level, with bt-edit)
    readonly property int autopilotPopup:       1520 // autopilot telemetry plate (tool level)
    readonly property int extraInfoPopup:        1530 // extra info panel (tool level)

    readonly property int settingsSidebar:  2000    // AppSettings sidebar
    readonly property int modeSettings:     2001    // ModeSettingsPanel

    readonly property int notificationsOverlay: 4000 // toast notifications (top-center stack)
    readonly property int fileOpeningOverlay: 5000  // modal "file is opening" banner
    readonly property int splashOverlay:    6000    // startup launch banner (fades out)
    readonly property int welcomeOverlay:   6500    // first-launch welcome / toolset picker (modal)
}

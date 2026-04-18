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
    readonly property int hotActions:        220    // HotActionsPanel collapsed
    readonly property int hotActionsActive: 2205    // HotActionsPanel when settings open

    readonly property int fullscreenPopup:  1300    // FullscreenPanePopup (per-pane)
    readonly property int globalPopup:      1400    // GlobalPanePopup (floating window)

    readonly property int settingsSidebar:  2000    // AppSettings sidebar
    readonly property int modeSettings:     2001    // ModeSettingsPanel
    readonly property int legacySidebar:    2002    // legacy menus sidebar
}

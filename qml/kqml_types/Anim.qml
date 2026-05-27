pragma Singleton
import QtQuick 2.15

// Centralised animation presets, grouped by UI domain. Each domain exposes a
// duration (ms) + an easing curve so motion can be tuned in ONE place instead
// of editing dozens of inline literals scattered across QML.
//
// Usage:
//   Behavior on opacity {
//       NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing }
//   }
//
// Duration properties are writable → can be wired to Test-group sliders for
// runtime tuning (like AppPalette.sidebarAnimMs). Easing properties are
// readonly aliases of the shared curves below — flip a domain's feel by
// pointing its *Easing at a different shared curve.
QtObject {
    id: anim

    // ── Shared curve ──────────────────────────────────────────────────────────
    // The app is unified on a single snappy curve: OutCubic (f(t)=1−(1−t)³) —
    // fast start, gentle settle, feels responsive. All domain *Easing point here.
    // Aliases kept for API stability; they resolve to the same curve.
    readonly property int easingStd:       Easing.OutCubic
    readonly property int easingSmooth:    easingStd
    readonly property int easingInOutQuad: easingStd

    // ── Controls: button hover/press (fill, border, scale, white overlay) ─────
    property int controlMs: 110
    readonly property int controlEasing: easingStd

    // ── Toggles / switches: knob slide, track colour ─────────────────────────
    property int toggleMs: 120
    readonly property int toggleEasing: easingStd

    // ── Generic fade: opacity in/out (icons, overlays) ───────────────────────
    property int fadeMs: 120
    readonly property int fadeEasing: easingStd

    // ── Toolbars: 3D scene idle-transparency fade ────────────────────────────
    property int toolbarFadeMs: 150
    readonly property int toolbarFadeEasing: easingStd

    // ── Split / pane resize ──────────────────────────────────────────────────
    property int splitGhostMs: 90     // ghost edge glide between snap steps
    readonly property int splitGhostEasing: easingStd
    property int paneResizeMs: 140    // pane geometry settle on resize commit
    readonly property int paneResizeEasing: easingStd

    // ── Disclosure: settings groups + ParamCardGroup expand/collapse ─────────
    property int disclosureMs: 200
    readonly property int disclosureEasing: easingStd

    // ── Sidebars + workspace inset ────────────────────────────────────────────
    // Sourced from AppPalette so the existing Test-group sliders keep driving
    // them; exposed here for a single discoverable surface.
    readonly property int sidebarMs:   AppPalette.sidebarAnimMs
    readonly property int workspaceMs: AppPalette.workspaceAnimMs

    // ── Popups (BasePanePopup): snap, resize, fullscreen ─────────────────────
    property int popupMs:      220   // snap / resize settle
    property int popupEnterMs: 260   // fullscreen enter
    property int popupExitMs:  220   // fullscreen exit
    readonly property int popupEasing:     easingStd
    readonly property int popupExitEasing: easingInOutQuad
}

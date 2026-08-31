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
    readonly property int easingBack:      Easing.OutBack

    // ── Controls: button hover/press (fill, border, scale, white overlay) ─────
    property int controlMs: 110
    readonly property int controlEasing: easingStd

    // ── Hover / press feedback: ONE place for "how alive" every control feels ──
    // A pixel lift capped by a factor. Neither half works alone: a plain factor
    // is sub-pixel on a 30 px icon button and a jump on a 260 px one, while a
    // plain px lift over-pops small controls — `scale` grows BOTH axes, so 4 px
    // on a 30×30 square is 13 % in each direction, not just along the width.
    // So: grow by hoverLiftPx, but never by more than hoverLiftMaxFactor.
    property real hoverLiftPx: 4
    property real pressDipPx: 2
    property real hoverLiftMaxFactor: 0.05
    property real pressDipMaxFactor: 0.025
    property real hoverLighten: 1.10

    function liftScale(w) {
        return w > 1 ? 1 + Math.min(Math.round(hoverLiftPx * AppPalette.scale) / w,
                                    hoverLiftMaxFactor)
                     : 1.0
    }

    function dipScale(w) {
        return w > 1 ? 1 - Math.min(Math.round(pressDipPx * AppPalette.scale) / w,
                                    pressDipMaxFactor)
                     : 1.0
    }

    // ── Toggles / switches: knob slide, track colour ─────────────────────────
    property int toggleMs: 120
    readonly property int toggleEasing: easingStd

    // ── Generic fade: opacity in/out (icons, overlays) ───────────────────────
    property int fadeMs: 120
    readonly property int fadeEasing: easingStd

    property int tooltipMs: 170
    property int tooltipExitMs: 90
    property real tooltipEnterScale: 1.08
    property real tooltipExitScale: 0.94
    property real tooltipOvershoot: 0.9
    readonly property int tooltipEnterEasing: easingBack
    readonly property int tooltipEasing: easingStd

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

    // ── Settings sub-page push/pop (drill-in echogram page slide) ────────────
    property int subpageMs: 260
    readonly property int subpageEasing: easingStd

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

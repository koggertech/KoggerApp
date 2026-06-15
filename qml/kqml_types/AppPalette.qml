pragma Singleton
import QtQuick 2.15

QtObject {
    id: palette

    readonly property bool isDark: !theme || theme.themeID < 2 || theme.themeID > 3

    // ── Scaling ───────────────────────────────────────────────────────────────
    // Multiply hardcoded pixel values by scale to adapt to DPI / user preference.
    // E.g. `font.pixelSize: AppPalette.px(14)` / `height: AppPalette.px(30)`.
    readonly property real scale: theme ? theme.resCoeff : 1.0
    function px(base) { return base * scale }

    // ── Tap recognition tolerance (runtime-tunable) ───────────────────────────
    // KTapArea reads this for the double-tap distance threshold. Exposed
    // through the "Test" settings group when the app is compiled with
    // MANUAL_TESTING — lets us empirically dial in a comfortable value on
    // touch hardware without recompiling.
    property int doubleTapDistancePx: 100

    // Pane split-drag-zone hit area — the invisible thickness around the
    // line between panes where a press/drag is recognised as a resize gesture.
    // Wider = easier to grab with a finger; default 40 px (was 20 in code).
    // Tunable via the "Test" settings group.
    property int splitHitSizePx: 50

    // Unified drag-handle bar (pane-split bars, popup headers, bottom-track
    // palette). One finger-sized control everywhere. Base px — consumers scale.
    // Length runs along the drag axis; thickness is the finger-critical size.
    readonly property int dragBarLengthPx: 72
    readonly property int dragBarThicknessPx: 22

    // Sidebar slide-in/out animation duration (panel.x, opacity). Tunable
    // independently from the workspace rubber-band so the two motions can
    // be dialled separately. Used by SettingsSidebarBase.progress Behavior.
    property int sidebarAnimMs: 166

    // Workspace inset animation duration. Used by MainWindow's
    // Behavior on settingsInsetLeft/Right. Independent of sidebarAnimMs.
    property int workspaceAnimMs: 166

    // ── Backgrounds ───────────────────────────────────────────────────────────
    // menuBackColor has per-theme alpha (semi-transparent menus in old UI);
    // strip it here so new opaque panels get a solid background.
    readonly property color bg: theme
        ? Qt.rgba(theme.menuBackColor.r, theme.menuBackColor.g, theme.menuBackColor.b, 1.0)
        : (isDark ? "#0F172A" : "#F8FAFC")

    readonly property color bgDeep:  theme ? Qt.darker(bg, isDark ? 1.25 : 1.04) : (isDark ? "#0B1220" : "#F1F5F9")
    readonly property color bgHover: theme ? theme.hoveredBackColor              : (isDark ? "#111B2E" : "#EFF6FF")

    // ── Cards / controls ──────────────────────────────────────────────────────
    readonly property color card:      theme ? theme.controlBackColor            : (isDark ? "#1E293B" : "#FFFFFF")
    readonly property color cardHover: theme ? theme.hoveredBackColor            : (isDark ? "#172133" : "#F8FAFC")
    readonly property color headerBg:  theme ? Qt.darker(card, 1.08)            : (isDark ? "#1F2937" : "#F1F5F9")

    // ── Borders ───────────────────────────────────────────────────────────────
    readonly property color border:      theme ? theme.controlBorderColor       : (isDark ? "#334155" : "#E2E8F0")
    readonly property color borderHover: theme ? theme.controlSolidBorderColor  : (isDark ? "#475569" : "#CBD5E1")
    readonly property color borderFocus: theme ? theme.controlSolidBorderColor  : (isDark ? "#64748B" : "#94A3B8")

    // ── Text ──────────────────────────────────────────────────────────────────
    readonly property color text:       theme ? theme.textColor : (isDark ? "#E2E8F0" : "#0F172A")
    readonly property color textSecond: theme
        ? Qt.rgba(theme.textColor.r, theme.textColor.g, theme.textColor.b, 0.72)
        : (isDark ? "#CBD5E1" : "#334155")
    readonly property color textMuted: theme
        ? Qt.rgba(theme.textColor.r, theme.textColor.g, theme.textColor.b, 0.48)
        : (isDark ? "#94A3B8" : "#64748B")

    // ── Toggle controls ───────────────────────────────────────────────────────
    readonly property color trackOff:       theme ? theme.controlSolidBackColor   : (isDark ? "#475569" : "#CBD5E1")
    readonly property color trackOffBorder: theme ? theme.controlSolidBorderColor : (isDark ? "#6B7280" : "#94A3B8")
    readonly property color knob:           theme ? theme.sliderHandleColor       : (isDark ? "#E2E8F0" : "#FFFFFF")
    readonly property color knobBorder:     "#00000022"

    // ── Accent (design constant – blue; shifts light/dark only) ───────────────
    readonly property color accentBg:       isDark ? "#1E3A8A" : "#EFF6FF"
    readonly property color accentBgStrong: isDark ? "#1E3A8A" : "#93C5FD"
    readonly property color accentBorder:   isDark ? "#93C5FD" : "#60A5FA"
    readonly property color accentBar:      isDark ? "#60A5FA" : "#3B82F6"

    // ── Danger ────────────────────────────────────────────────────────────────
    readonly property color dangerBg:     isDark ? "#2A1313" : "#FEF2F2"
    readonly property color dangerHover:  isDark ? "#1F0F0F" : "#FEE2E2"
    readonly property color dangerBorder: isDark ? "#7F1D1D" : "#EF4444"
    readonly property color dangerText:   theme ? theme.textErrorColor : (isDark ? "#FCA5A5" : "#DC2626")

    // ── Link status (device connection dots: bright border on a tinted fill) ──
    readonly property color linkOkBg:       isDark ? "#0D2D1A" : "#BBF7D0"
    readonly property color linkOkBorder:   isDark ? "#10B981" : "#16A34A"
    readonly property color linkIdleBg:     isDark ? "#2D2200" : "#FEF08A"
    readonly property color linkIdleBorder: isDark ? "#F59E0B" : "#CA8A04"
    readonly property color linkDownBg:     isDark ? "#2D0D0D" : "#FECACA"
    readonly property color linkDownBorder: isDark ? "#EF4444" : "#DC2626"

    // ── Tooltip ───────────────────────────────────────────────────────────────
    readonly property color tooltipBg:     theme ? theme.tooltipBackColor   : (isDark ? "#0B1220" : "#FFFFFF")
    readonly property color tooltipBorder: theme ? theme.tooltipBorderColor : (isDark ? "#334155" : "#E2E8F0")
    readonly property color tooltipText:   theme ? theme.tooltipTextColor   : (isDark ? "#E2E8F0" : "#0F172A")

    // ── Overlays (black-based, universal) ─────────────────────────────────────
    readonly property color dim:       isDark ? "#88061722" : "#44000000"
    readonly property color shadow0:   isDark ? "#F0000000" : "#88000000"
    readonly property color shadowMid: isDark ? "#5A000000" : "#33000000"
}

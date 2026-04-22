pragma Singleton
import QtQuick 2.15

QtObject {
    id: palette

    readonly property bool isDark: !theme || theme.themeID < 2 || theme.themeID > 3

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
    readonly property color accentBg:     isDark ? "#1E3A8A" : "#EFF6FF"
    readonly property color accentBorder: isDark ? "#93C5FD" : "#60A5FA"
    readonly property color accentBar:    isDark ? "#60A5FA" : "#3B82F6"

    // ── Danger ────────────────────────────────────────────────────────────────
    readonly property color dangerBg:     isDark ? "#2A1313" : "#FEF2F2"
    readonly property color dangerHover:  isDark ? "#1F0F0F" : "#FEE2E2"
    readonly property color dangerBorder: isDark ? "#7F1D1D" : "#EF4444"
    readonly property color dangerText:   theme ? theme.textErrorColor : (isDark ? "#FCA5A5" : "#DC2626")

    // ── Tooltip ───────────────────────────────────────────────────────────────
    readonly property color tooltipBg:     theme ? theme.tooltipBackColor   : (isDark ? "#0B1220" : "#FFFFFF")
    readonly property color tooltipBorder: theme ? theme.tooltipBorderColor : (isDark ? "#334155" : "#E2E8F0")
    readonly property color tooltipText:   theme ? theme.tooltipTextColor   : (isDark ? "#E2E8F0" : "#0F172A")

    // ── Overlays (black-based, universal) ─────────────────────────────────────
    readonly property color dim:       isDark ? "#88061722" : "#44000000"
    readonly property color shadow0:   isDark ? "#F0000000" : "#88000000"
    readonly property color shadowMid: isDark ? "#5A000000" : "#33000000"
}

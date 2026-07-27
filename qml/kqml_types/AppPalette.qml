pragma Singleton
import QtQuick 2.15

QtObject {
    id: palette

    readonly property bool isDark: !theme
        || (0.299 * theme.menuBackColor.r + 0.587 * theme.menuBackColor.g + 0.114 * theme.menuBackColor.b) < 0.5

    // ── Scaling ───────────────────────────────────────────────────────────────
    // Multiply hardcoded pixel values by scale to adapt to DPI / user preference.
    // E.g. `font.pixelSize: AppPalette.px(14)` / `height: AppPalette.px(30)`.
    readonly property real scale: theme ? theme.resCoeff : 1.0
    property real appScaleBoost: 40 / 36
    readonly property real appScale: scale * appScaleBoost
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
    // Raw theme base (deepest-recess anchor, opaque).
    readonly property color _rawBg: theme
        ? Qt.rgba(theme.menuBackColor.r, theme.menuBackColor.g, theme.menuBackColor.b, 1.0)
        : (isDark ? "#0F172A" : "#F8FAFC")

    // Perceived lightness (0..1) of a colour — WCAG-ish luma.
    function luminance(c) { return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b }

    // Panel / base fill. Near-black themes (e.g. "S.Dark" = pure #000) are lifted
    // off black so the elevation ladder (card/controlRaised above, bgDeep below)
    // stays legible — otherwise every recessed fill collapses onto the panel and
    // becomes invisible. Non-near-black themes are returned unchanged.
    readonly property color bg: luminance(_rawBg) < 0.06
        ? Qt.tint(_rawBg, Qt.rgba(1, 1, 1, 0.14))
        : _rawBg

    // Deepest recess (expanded group bodies, pressed states). Derived from the
    // RAW base so a near-black theme keeps a genuinely dark body beneath the
    // lifted bg; identical to the old value for every other theme.
    readonly property color bgDeep:  theme ? Qt.darker(_rawBg, isDark ? 1.25 : 1.04) : (isDark ? "#0B1220" : "#F1F5F9")
    readonly property color bgHover: theme ? theme.hoveredBackColor              : (isDark ? "#111B2E" : "#EFF6FF")
    readonly property color groupBorder: Qt.darker(bgDeep, isDark ? 1.45 : 1.12)

    // ── Cards / controls ──────────────────────────────────────────────────────
    readonly property color card:      theme ? theme.controlBackColor            : (isDark ? "#1E293B" : "#FFFFFF")
    readonly property color cardHover: theme ? theme.hoveredBackColor            : (isDark ? "#172133" : "#F8FAFC")
    readonly property color headerBg:  theme ? Qt.darker(card, 1.08)            : (isDark ? "#1F2937" : "#F1F5F9")
    // Raised control surface for buttons that sit ON a card (they'd blend at
    // `card`); a solid, distinctly lighter/prominent fill in place of an outline.
    readonly property color controlRaised: theme ? theme.controlSolidBackColor  : (isDark ? "#475569" : "#CBD5E1")
    // Softer raised fill for setting ROWS (toggles / param rows / combo rows):
    // 60% of the way from bg toward card — reads as raised without the full
    // brightness of `card`. Tune the 0.6 to dial how much rows stand out.
    readonly property color rowRaised: theme ? Qt.tint(bg, Qt.rgba(card.r, card.g, card.b, 0.6)) : (isDark ? "#18212F" : "#FBFCFE")

    readonly property color chipRaised:      isDark ? controlRaised : Qt.darker(card, 1.12)
    readonly property color chipRaisedHover: isDark ? Qt.lighter(controlRaised, 1.2) : Qt.darker(card, 1.20)

    // ── Borders ───────────────────────────────────────────────────────────────
    readonly property color border:      theme ? theme.controlBorderColor       : (isDark ? "#334155" : "#E2E8F0")
    readonly property color borderHover: theme ? theme.controlSolidBorderColor  : (isDark ? "#475569" : "#CBD5E1")
    readonly property color borderFocus: theme ? theme.controlSolidBorderColor  : (isDark ? "#64748B" : "#94A3B8")

    // ── Text ──────────────────────────────────────────────────────────────────
    readonly property color text:       theme ? theme.textColor : (isDark ? "#E2E8F0" : "#0F172A")
    readonly property color textStrong: isDark ? "#FFFFFF" : text
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

    // ── Accent (per-theme, harmonised with each palette) ──────────────────────
    readonly property color accent: {
        if (!theme)
            return "#51A3D9"
        switch (theme.themeID) {
        case 0:  return "#3E8FD6"
        case 1:  return "#3E8FD6"
        case 2:  return "#2F80D8"
        case 3:  return "#2F80D8"
        case 4:  return "#4C8CD4"
        case 5:  return "#F97316"
        case 6:  return "#C56A2C"
        case 7:  return "#268BD2"
        case 8:  return "#45749B"
        case 9:  return "#51A3D9"
        case 10: return "#BD93F9"
        case 11: return "#88C0D0"
        default: return "#51A3D9"
        }
    }
    readonly property color accentText:   luminance(accent) < 0.55 ? "#FFFFFF" : "#15202B"

    readonly property color brandK:        "#51A3D9"

    readonly property color toggleOn:       accent
    readonly property color toggleOnBorder: Qt.lighter(accent, 1.25)

    readonly property color accentBg:       accent
    readonly property color accentBgStrong: accent
    readonly property color accentBorder:   Qt.lighter(accent, 1.25)
    readonly property color accentBar:      accent

    readonly property bool _desert: !!theme && theme.themeID === 8

    // ── Danger ────────────────────────────────────────────────────────────────
    readonly property color dangerBg:     _desert ? Qt.tint(card, Qt.rgba(0.71, 0.26, 0.18, 0.18))
                                         : isDark ? "#2A1313" : "#FEF2F2"
    readonly property color dangerHover:  _desert ? Qt.tint(card, Qt.rgba(0.71, 0.26, 0.18, 0.26))
                                         : isDark ? "#1F0F0F" : "#FEE2E2"
    readonly property color dangerBorder: _desert ? "#B5432F"
                                         : isDark ? "#7F1D1D" : "#EF4444"
    readonly property color dangerText:   _desert ? "#5C1810"
                                         : theme ? theme.textErrorColor : (isDark ? "#FCA5A5" : "#DC2626")

    // ── Link status (device connection dots: bright border on a tinted fill) ──
    readonly property color linkOkBg:       _desert ? Qt.tint(card, Qt.rgba(0.086, 0.639, 0.290, 0.40))
                                          : isDark ? "#0D2D1A" : "#BBF7D0"
    readonly property color linkOkBorder:   isDark ? "#10B981" : "#16A34A"
    readonly property color linkIdleBg:     isDark ? "#2D2200" : "#FEF08A"
    readonly property color linkIdleBorder: isDark ? "#F59E0B" : "#CA8A04"
    readonly property color linkDownBg:     _desert ? Qt.tint(card, Qt.rgba(0.910, 0.365, 0.306, 0.42))
                                          : isDark ? "#2D0D0D" : "#FECACA"
    readonly property color linkDownBorder: _desert ? "#E85D4E"
                                          : isDark ? "#EF4444" : "#DC2626"

    readonly property color linkOkText:   isDark ? linkOkBorder   : Qt.darker(linkOkBorder, 1.7)
    readonly property color linkIdleText: isDark ? linkIdleBorder : Qt.darker(linkIdleBorder, 1.7)
    readonly property color linkDownText: isDark ? linkDownBorder : Qt.darker(linkDownBorder, 1.7)

    // ── Tooltip ───────────────────────────────────────────────────────────────
    readonly property color tooltipBg:     theme ? theme.tooltipBackColor   : (isDark ? "#0B1220" : "#FFFFFF")
    readonly property color tooltipBorder: theme ? theme.tooltipBorderColor : (isDark ? "#334155" : "#E2E8F0")
    readonly property color tooltipText:   theme ? theme.tooltipTextColor   : (isDark ? "#E2E8F0" : "#0F172A")

    // ── Overlays (black-based, universal) ─────────────────────────────────────
    readonly property color dim:       isDark ? "#88061722" : "#44000000"
    readonly property color shadow0:   isDark ? "#F0000000" : "#88000000"
    readonly property color shadowMid: isDark ? "#5A000000" : "#33000000"

    readonly property var _logSyntaxDark: ({ time: "#6E7681", dirIn: "#7EE787", dirOut: "#79C0FF", info: "#E6EDF3", mode: "#D2A8FF", num: "#FFA657", comment: "#A5D6FF", error: "#FF7B72", warn: "#E3B341", payload: "#8B949E", plain: "#E6EDF3" })
    readonly property var _logSyntaxLight: ({ time: "#6E7781", dirIn: "#116329", dirOut: "#0550AE", info: "#1F2328", mode: "#8250DF", num: "#953800", comment: "#0A3069", error: "#CF222E", warn: "#9A6700", payload: "#57606A", plain: "#1F2328" })
    readonly property var consoleSyntax: {
        if (!theme)
            return _logSyntaxDark
        switch (theme.themeID) {
        case 2: case 3: return _logSyntaxLight
        case 4:  return ({ time: "#5C6370", dirIn: "#98C379", dirOut: "#61AFEF", info: "#ABB2BF", mode: "#C678DD", num: "#D19A66", comment: "#56B6C2", error: "#E06C75", warn: "#E5C07B", payload: "#7F8896", plain: "#ABB2BF" })
        case 5:  return ({ time: "#75715E", dirIn: "#A6E22E", dirOut: "#66D9EF", info: "#F8F8F2", mode: "#AE81FF", num: "#FD971F", comment: "#E6DB74", error: "#F92672", warn: "#FD971F", payload: "#A6A28C", plain: "#F8F8F2" })
        case 6:  return ({ time: "#A57A4C", dirIn: "#889B4A", dirOut: "#8AB1B0", info: "#D3AF86", mode: "#C9A554", num: "#F79A32", comment: "#B4823E", error: "#DC3958", warn: "#F06431", payload: "#9A7B52", plain: "#D3AF86" })
        case 7:  return ({ time: "#586E75", dirIn: "#859900", dirOut: "#268BD2", info: "#93A1A1", mode: "#6C71C4", num: "#CB4B16", comment: "#2AA198", error: "#DC322F", warn: "#B58900", payload: "#657B83", plain: "#839496" })
        case 8:  return ({ time: "#6B5A34", dirIn: "#3F6212", dirOut: "#1D4E89", info: "#241806", mode: "#6B21A8", num: "#9A3412", comment: "#4A3808", error: "#7A1212", warn: "#5C3D06", payload: "#5B4A28", plain: "#241806" })
        case 10: return ({ time: "#6272A4", dirIn: "#50FA7B", dirOut: "#8BE9FD", info: "#F8F8F2", mode: "#FF79C6", num: "#BD93F9", comment: "#F1FA8C", error: "#FF5555", warn: "#FFB86C", payload: "#7C8AC0", plain: "#F8F8F2" })
        case 11: return ({ time: "#6C7A94", dirIn: "#A3BE8C", dirOut: "#88C0D0", info: "#ECEFF4", mode: "#B48EAD", num: "#D08770", comment: "#8FBCBB", error: "#BF616A", warn: "#EBCB8B", payload: "#7B88A1", plain: "#E5E9F0" })
        default: return _logSyntaxDark
        }
    }
}

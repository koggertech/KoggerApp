pragma Singleton
import QtQuick 2.15

QtObject {
    id: tokens

    readonly property real s: AppPalette.scale

    // Breakpoints — raw px, NOT scaled (classify physical screens, not content).
    readonly property int bpCompact: 800
    readonly property int bpRegular: 1280
    function bp(w)        { return w < bpCompact ? "compact" : (w < bpRegular ? "regular" : "wide") }
    function isCompact(w) { return w > 0 && w < bpCompact }

    // Typography
    readonly property int fontXs:   Math.round(11 * s)
    readonly property int fontSm:   Math.round(12 * s)
    readonly property int fontMd:   Math.round(13 * s)
    readonly property int fontBase: Math.round(14 * s)
    readonly property int fontLg:   Math.round(16 * s)
    readonly property int fontXl:   Math.round(18 * s)
    readonly property int fontXxl:  Math.round(20 * s)

    // Spacing
    readonly property int spaceXxs: Math.round(2  * s)
    readonly property int spaceXs:  Math.round(4  * s)
    readonly property int spaceSm:  Math.round(6  * s)
    readonly property int spaceMd:  Math.round(8  * s)
    readonly property int spaceLg:  Math.round(12 * s)
    readonly property int spaceXl:  Math.round(16 * s)
    readonly property int spaceXxl: Math.round(24 * s)

    // Control heights. controlH mirrors theme.controlHeight for legacy alignment.
    readonly property int controlH:    theme ? theme.controlHeight : Math.round(26 * s)
    readonly property int controlHSm:  Math.round(24 * s)
    readonly property int controlHMd:  Math.round(30 * s)
    readonly property int controlHLg:  Math.round(36 * s)
    readonly property int controlHXl:  Math.round(48 * s)

    // Inline marks — chips, badges, the compact switches that sit INSIDE a control row rather
    // than being rows of their own. Only `controlH` follows `theme.controlHeight`, so anything
    // written as a literal stayed the size it was written at and read as undersized against
    // every theme asking for more; the USBL panes were full of hard-coded 20 px. Floored at
    // controlHSm so a theme with unusually short controls cannot shrink a chip below the
    // buttons standing next to it.
    readonly property int chipH: Math.max(controlHSm, Math.round(controlH * 0.92))

    // Iconography
    readonly property int iconSm: Math.round(16 * s)
    readonly property int iconMd: Math.round(20 * s)
    readonly property int iconLg: Math.round(24 * s)
    readonly property int iconXl: Math.round(36 * s)

    // Radii
    readonly property int radiusSm: Math.round(4 * s)
    readonly property int radiusMd: Math.round(6 * s)
    readonly property int radiusLg: Math.round(8 * s)

    readonly property int cardBorderWidth: 0

    // Responsive grid columns: how many cellMinW-wide cells fit into availableW.
    function gridColumns(availableW, cellMinW, gap, maxCols) {
        if (availableW <= 0 || cellMinW <= 0) return 1
        var cols = Math.floor((availableW + gap) / (cellMinW + gap))
        cols = Math.max(1, cols)
        if (maxCols && maxCols > 0) cols = Math.min(cols, maxCols)
        return cols
    }
}

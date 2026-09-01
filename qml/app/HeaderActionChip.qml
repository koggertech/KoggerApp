import QtQuick 2.15
import kqml_types 1.0

// Rounded chip for a settings-group header action: one place for the size, radius,
// fill and icon proportions shared by every header chip.
KCircleIconButton {
    id: chip

    property real size: Tokens.controlHLg
    property bool active: false

    width: size
    height: size
    cornerRadius: Tokens.radiusLg
    borderWidth: 0
    scaleOnHover: false
    iconPixelSize: Math.round(size * 0.5)
    iconTintColor: active ? AppPalette.accentText : AppPalette.text
    glyphColor: iconTintColor
    fillColor:      active ? AppPalette.accentBgStrong : AppPalette.chipRaised
    fillHoverColor: active ? AppPalette.accentBgStrong : AppPalette.chipRaisedHover
}

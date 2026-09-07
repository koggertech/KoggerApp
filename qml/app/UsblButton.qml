import QtQuick 2.15
import kqml_types 1.0

// Raised button matching DeviceSettingsPage's inline DevButton, which is not visible
// outside that file.
KButton {
    normalBg: AppPalette.controlRaised
    hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
    dangerBg: AppPalette.controlRaised
    dangerHoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
    borderWidth: danger ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : Tokens.cardBorderWidth
}

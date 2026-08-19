import QtQuick 2.15
import kqml_types 1.0

// The servo panel's editor overlay: the scene dims and the panel itself sits in the middle,
// exactly as a grid panel's editor behaves. There is nothing to drop here -- the panel has no
// cells -- so this is the presentation half of WidgetEditOverlay and none of its DnD.
//
// It shows a real ServoPanelBody rather than a mock-up, so the transparency slider is previewed
// against the panel it will apply to and the controls stay usable while the editor is open.
Item {
    id: overlay

    required property var store

    anchors.fill: parent
    visible: !!(store && store.widgetEditorActive && store.widgetDraftKind === "servo")

    readonly property real _pad: Tokens.spaceXs
    readonly property real _bgAlpha: Math.max(0, Math.min(1, 1 - (store ? store.widgetDraftTransparency : 0) / 100))

    readonly property real _visLeft: (store && store.settingsPanelOpen && store.settingsSide === "left") ? store.settingsPanelSizePx : 0
    readonly property real _visRight: (store && store.settingsPanelOpen && store.settingsSide === "right") ? store.settingsPanelSizePx : 0

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.82

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
            onWheel: function(wheel) { wheel.accepted = true }
        }
    }

    Rectangle {
        width: preview.width + overlay._pad * 2
        height: preview.height + overlay._pad * 2
        x: overlay._visLeft + Math.round((overlay.width - overlay._visLeft - overlay._visRight - width) / 2)
        y: Math.round((overlay.height - height) / 2)
        radius: Tokens.radiusLg
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, overlay._bgAlpha)
        border.width: Tokens.cardBorderWidth
        border.color: AppPalette.border

        ServoPanelScroll {
            id: preview
            x: overlay._pad
            y: overlay._pad
            maxHeight: Math.max(Math.round(160 * AppPalette.scale), overlay.height * 0.86 - overlay._pad * 2)
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

// Drill-in settings page for ONE echogram — the plot whose gear was clicked
// (store.echogramSettingsPlot). The header title + "back" button live in
// SettingsSidebarBase; this is just the scrollable body.
//
// The panel is rebuilt whenever the target plot changes (Loader.active toggle)
// so every value:/checked: binding re-attaches to the current plot — sidesteps
// the Qt Quick Controls quirk where a user toggle severs a two-way binding.
Column {
    id: page

    required property var store
    readonly property var plot: store ? store.echogramSettingsPlot : null

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Loader {
        id: panelLoader
        width: parent.width
        height: item ? item.implicitHeight : 0
        active: false
        sourceComponent: panelComp

        property var currentPlot: page.plot

        onCurrentPlotChanged: {
            active = false
            if (currentPlot)
                active = true
        }
        Component.onCompleted: if (currentPlot) active = true
    }

    Component {
        id: panelComp
        EchogramSettingsPanel {
            width: panelLoader.width
            plot: panelLoader.currentPlot
            store: page.store
            hideEmpty: page.store ? page.store.hideEmptyEchogramControls : true
        }
    }

    Text {
        width: parent.width
        visible: !page.plot
        text: qsTr("Open this page from an echogram's gear button.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontMd
        wrapMode: Text.WordWrap
    }
}

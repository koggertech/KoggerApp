import QtQuick 2.15
import kqml_types 1.0

// Drill-in: CSV export with customizable columns. The destination folder +
// decimation are shared with the Export settings group (WorkspaceStore) — set
// there, not repeated here. Column on/off lives in C++ (Core.csvExportField*),
// defaults match the built-in set; "Reset" restores them.
Column {
    id: page

    required property var store
    property var targetPlot: null

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    // Ordered column list — labels translatable here; keys match Core.
    readonly property var fieldDefs: [
        { key: "meas_nbr",         label: qsTr("Number") },
        { key: "event_id",         label: qsTr("Event (UNIX / timestamp / ID)") },
        { key: "rangefinder",      label: qsTr("Rangefinder") },
        { key: "bottom_depth",     label: qsTr("Beam distance") },
        { key: "pos_lat_lon",      label: qsTr("Position (lat / lon)") },
        { key: "pos_time",         label: qsTr("GNSS UTC date / time") },
        { key: "external_pos_lla", label: qsTr("External position (LLA)") },
        { key: "external_pos_neu", label: qsTr("External position (NEU)") },
        { key: "sonar_height",     label: qsTr("Sonar height") },
        { key: "bottom_height",    label: qsTr("Bottom height") },
        { key: "contact_info",     label: qsTr("Contact title") },
        { key: "contact_distance", label: qsTr("Contact distance") }
    ]

    function doExport() {
        if (!page.store || !page.targetPlot)
            return
        core.exportPlotAsCVS(page.store.exportFolderSource,
                             page.targetPlot.plotDatasetChannel(),
                             page.store.exportDecimationEnabled ? page.store.exportDecimationValue : 0)
    }

    // ── Description ───────────────────────────────────────────────────────
    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Export the dataset to CSV. Pick which columns to include below — the defaults match the built-in export. The destination folder is set in the Export group.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    // ── Decimation + export ───────────────────────────────────────────────
    ParamCard {
        id: decimationCard
        width: parent.width
        label: qsTr("Decimation, m:")
        slotWidth: Math.round(120 * AppPalette.scale)
        checked: page.store ? page.store.exportDecimationEnabled : false
        onToggled: function(v) { if (page.store) page.store.exportDecimationEnabled = v }

        KSpinBox {
            width: Math.round(120 * AppPalette.scale)
            height: Tokens.controlHMd
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            from: 0; to: 100; stepSize: 1
            value: page.store ? page.store.exportDecimationValue : 10
            onValueModified: function(v) { if (page.store) page.store.exportDecimationValue = v }
        }
    }

    KButton {
        width: parent.width
        height: Tokens.controlHLg
        text: qsTr("Export to CSV")
        onClicked: page.doExport()
    }

    // ── Columns ───────────────────────────────────────────────────────────
    Item { width: parent.width; height: Math.round(Tokens.spaceSm) }

    Row {
        width: parent.width
        spacing: Tokens.spaceMd

        Text {
            id: columnsHeader
            text: qsTr("Columns to export")
            color: AppPalette.text
            font.pixelSize: Tokens.fontMd
            font.bold: true
            width: parent.width - resetButton.width - Tokens.spaceMd
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            height: resetButton.height
        }
        KButton {
            id: resetButton
            text: qsTr("Reset")
            onClicked: core.resetCsvExportFields()
        }
    }

    // Dark rounded card around the toggles — same look as SettingsGroup body.
    Rectangle {
        width: parent.width
        height: fieldsLoader.height + 2 * Tokens.spaceMd
        radius: Tokens.radiusLg
        color: AppPalette.bgDeep
        border.color: AppPalette.border
        border.width: 1

        Loader {
            id: fieldsLoader
            x: Tokens.spaceMd
            y: Tokens.spaceMd
            width: parent.width - 2 * Tokens.spaceMd
            sourceComponent: fieldsComponent

            Connections {
                target: typeof core !== "undefined" ? core : null
                ignoreUnknownSignals: true
                // Rebuild the switches so they re-read Core (binding sever-proof).
                function onCsvExportFieldsReset() {
                    fieldsLoader.active = false
                    fieldsLoader.active = true
                }
            }
        }
    }

    Component {
        id: fieldsComponent
        Column {
            width: fieldsLoader.width
            spacing: Tokens.spaceMd
            Repeater {
                model: page.fieldDefs
                delegate: KSwitch {
                    required property var modelData
                    width: parent.width
                    text: modelData.label
                    checked: core.csvExportFieldEnabled(modelData.key)
                    onToggled: core.setCsvExportField(modelData.key, checked)
                }
            }
        }
    }
}

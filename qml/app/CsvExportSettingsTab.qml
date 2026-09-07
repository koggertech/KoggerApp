import QtQuick 2.15
import QtCore
import kqml_types 1.0

// Drill-in: CSV export with customizable columns. The destination folder +
// decimation are shared with the Export settings group (WorkspaceStore) — set
// there, not repeated here. Column on/off lives in C++ (Core.csvExportField*),
// defaults match the built-in set; "Reset" restores them.
Column {
    id: page

    required property var store
    property var targetPlot: null

    readonly property color _bright: AppPalette.isDark ? "#FFFFFF" : AppPalette.text

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceSm

    component Spacer: Item {
        property int size: Tokens.spaceMd
        width: page.width
        height: size
    }

    // Ordered column list — labels translatable here; keys match Core.
    readonly property var fieldDefs: [
        { key: "meas_nbr",         label: qsTr("Number"),                        tip: qsTr("Sequential measurement number (epoch)") + " — Number" },
        { key: "event_id",         label: qsTr("Event (UNIX / timestamp / ID)"), tip: qsTr("Event columns: UNIX time, timestamp and ID") + " — Event UNIX, Event timestamp, Event ID" },
        { key: "bottom_depth",     label: qsTr("Beam distance"),                 tip: qsTr("Post-processing distance (green line)") + " — Beam distance" },
        { key: "pos_lat_lon",      label: qsTr("Position (lat / lon)"),          tip: qsTr("Coordinates: latitude and longitude") + " — Latitude, Longitude" },
        { key: "pos_time",         label: qsTr("GNSS UTC date / time"),          tip: qsTr("UTC date and time from GNSS") + " — GNSS UTC Date, GNSS UTC Time" },
        { key: "external_pos_lla", label: qsTr("External position (LLA)"),       tip: qsTr("External position: latitude / longitude / altitude") + " — ExtLatitude, ExtLongitude, ExtAltitude" },
        { key: "external_pos_neu", label: qsTr("External position (NEU)"),       tip: qsTr("External position in local NEU (north / east / up)") + " — ExtNorth, ExtEast, ExtHeight" },
        { key: "sonar_height",     label: qsTr("Sonar height"),                  tip: qsTr("Absolute height of the sonar") + " — SonarHeight" },
        { key: "bottom_height",    label: qsTr("Bottom height"),                 tip: qsTr("Absolute height of the bottom") + " — BottomHeight" },
        { key: "contact_info",     label: qsTr("Contact title"),                 tip: qsTr("Title of the marked contact") + " — ContactTitle" },
        { key: "contact_distance", label: qsTr("Contact distance"),              tip: qsTr("Distance to the marked contact") + " — ContactDistance" },
        { key: "rangefinder",      label: qsTr("Rangefinder"),                   tip: qsTr("Rangefinder distance (red line)") + " — Rangefinder" }
    ]

    function doExport() {
        if (!page.store || !page.targetPlot)
            return
        page.targetPlot.setOffsetX(sonarOffsetSwitch.checked ? sonarOffsetX.value *  0.001 : 0)
        page.targetPlot.setOffsetY(sonarOffsetSwitch.checked ? sonarOffsetY.value *  0.001 : 0)
        page.targetPlot.setOffsetZ(sonarOffsetSwitch.checked ? sonarOffsetZ.value * -0.001 : 0)
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

    // ── Export + decimation ───────────────────────────────────────────────
    Spacer {}

    KButton {
        width: parent.width
        height: Tokens.controlHLg
        text: qsTr("Export to CSV")
        onClicked: page.doExport()
    }

    Spacer { size: Tokens.spaceXl }

    KIsland {
        KIslandRow {
            label: qsTr("Decimation, m:")
            toolTipText: qsTr("Thin out points: keep one per given distance interval (m); off exports every point")
            labelColor: page._bright
            interactive: true
            onClicked: decimationSwitch.click()

            Item {
                width: decimationSlotRow.width
                height: decimationSlotRow.height

                MouseArea { anchors.fill: parent }

                Row {
                    id: decimationSlotRow
                    spacing: Tokens.spaceSm

                    KSpinBox {
                        width: Math.round(120 * AppPalette.scale)
                        height: Tokens.controlHMd
                        fontPixelSize: Tokens.fontLg
                        textColor: page._bright
                        from: 0; to: 100; stepSize: 1
                        value: page.store ? page.store.exportDecimationValue : 10
                        onValueModified: function(v) { if (page.store) page.store.exportDecimationValue = v }
                    }

                    KSwitch {
                        id: decimationSwitch
                        flat: true
                        checked: page.store ? page.store.exportDecimationEnabled : false
                        onToggled: if (page.store) page.store.exportDecimationEnabled = checked
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("Sonar offset XYZ, mm")
            toolTipText: qsTr("Mount offset of the transducer, applied to the exported coordinates only.")
            labelColor: page._bright
            interactive: true
            onClicked: sonarOffsetSwitch.click()

            KSwitch {
                id: sonarOffsetSwitch
                flat: true
            }
        }

        KIslandRow {
            open: sonarOffsetSwitch.checked
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            stacked: true

            Row {
                width: parent.width
                height: Tokens.controlHMd
                spacing: Tokens.spaceXs
                readonly property real sw: (width - 2 * Tokens.spaceXs) / 3

                KSpinBox {
                    id: sonarOffsetX
                    width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                }
                KSpinBox {
                    id: sonarOffsetY
                    width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                }
                KSpinBox {
                    id: sonarOffsetZ
                    width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                }
            }
        }
    }

    Settings { category: "scene2d/bottomTrack"; property alias bottomTrackSensorOffset: sonarOffsetSwitch.checked }
    Settings { category: "scene2d/bottomTrack"; property alias bottomTrackSensorOffsetValueX: sonarOffsetX.value }
    Settings { category: "scene2d/bottomTrack"; property alias bottomTrackSensorOffsetValueY: sonarOffsetY.value }
    Settings { category: "scene2d/bottomTrack"; property alias bottomTrackSensorOffsetValueZ: sonarOffsetZ.value }

    // ── Columns ───────────────────────────────────────────────────────────
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

    Loader {
        id: fieldsLoader
        width: parent.width
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

    Component {
        id: fieldsComponent
        KIsland {
            Repeater {
                model: page.fieldDefs
                delegate: KIslandRow {
                    required property var modelData

                    label: modelData.label
                    toolTipText: modelData.tip
                    interactive: true
                    onClicked: fieldSwitch.click()

                    KSwitch {
                        id: fieldSwitch
                        flat: true
                        checked: core.csvExportFieldEnabled(modelData.key)
                        onToggled: core.setCsvExportField(modelData.key, checked)
                    }
                }
            }
        }
    }
}

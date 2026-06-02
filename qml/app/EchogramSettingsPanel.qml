import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

// New-style settings for ONE echogram. Binds two-way to plot.viewState (a hidden
// holder living inside Plot2D that owns persistence + C++ init + channel sync).
//
// IMPORTANT: this panel is rebuilt by EchogramSettingsTab whenever the selected
// echogram changes (Loader.active toggle), so every `value:`/`checked:` binding
// is fresh against the current plot. That sidesteps the Qt Quick Controls
// quirk where a user toggle severs a two-way `checked:` binding — on the next
// echogram switch the whole panel is re-instantiated with correct values.
Column {
    id: panel

    property var plot: null
    readonly property var vs: plot ? plot.viewState : null
    readonly property int instruments: theme ? theme.instrumentsGrade : 0

    readonly property int comboW: Math.round(150 * AppPalette.scale)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    // ── Reusable label + combo row ────────────────────────────────────────────
    component ComboRow: RowLayout {
        id: crow
        property string label: ""
        property var comboModel: []
        property int currentIndex: 0
        property bool enabledRow: true
        signal picked(int index)
        width: parent ? parent.width : implicitWidth
        spacing: Tokens.spaceMd
        Text {
            text: crow.label
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontMd
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        KCombo {
            Layout.preferredWidth: panel.comboW
            enabled: crow.enabledRow
            model: crow.comboModel
            currentIndex: crow.currentIndex
            onActivated: function(index) { crow.picked(index) }
        }
    }

    // ── Section header label ──────────────────────────────────────────────────
    component SectionLabel: Text {
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
        font.bold: true
        topPadding: Tokens.spaceXs
    }

    // ══ Channels (instruments > 1) ════════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        visible: panel.instruments > 1

        SectionLabel { text: qsTr("Channels") }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd
            KCombo {
                Layout.fillWidth: true
                model: panel.vs ? panel.vs.channelModel : []
                currentIndex: panel.vs ? panel.vs.ch1Index : 0
                onActivated: function(index) { if (panel.vs) panel.vs.ch1Index = index }
            }
            KCombo {
                Layout.fillWidth: true
                model: panel.vs ? panel.vs.channelModel : []
                currentIndex: panel.vs ? panel.vs.ch2Index : 0
                onActivated: function(index) { if (panel.vs) panel.vs.ch2Index = index }
            }
        }
    }

    // ══ Echogram + theme + compensation ════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        label: qsTr("Echogram")
        checked: panel.vs ? panel.vs.echogramVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.echogramVisible = v }

        ComboRow {
            label: qsTr("Theme")
            comboModel: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
            currentIndex: panel.vs ? panel.vs.echoThemeIndex : 0
            onPicked: function(index) { if (panel.vs) panel.vs.echoThemeIndex = index }
        }
        ComboRow {
            label: qsTr("Compensation")
            comboModel: [qsTr("Raw"), qsTr("Side-Scan"), qsTr("TGC")]
            currentIndex: panel.vs ? panel.vs.compensationIndex : 0
            onPicked: function(index) { if (panel.vs) panel.vs.compensationIndex = index }
        }
    }

    // ══ Bottom-Track (instruments > 0) ══════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        visible: panel.instruments > 0

        SectionLabel { text: qsTr("Bottom-Track") }

        KSwitch {
            width: parent.width
            text: qsTr("Value")
            checked: panel.vs ? panel.vs.bottomTrackValue : false
            onToggled: if (panel.vs) panel.vs.bottomTrackValue = checked
        }
        KSwitch {
            width: parent.width
            // Master "show the track graphic" toggle — the Type below (Line /
            // Points) only renders when this is on.
            text: qsTr("Graphics")
            checked: panel.vs ? panel.vs.bottomTrackLine : false
            onToggled: if (panel.vs) panel.vs.bottomTrackLine = checked
        }
        ComboRow {
            label: qsTr("Type")
            enabledRow: panel.vs ? panel.vs.bottomTrackLine : false
            comboModel: [qsTr("Line"), qsTr("Points")]
            currentIndex: panel.vs ? panel.vs.bottomTrackTheme : 0
            onPicked: function(index) { if (panel.vs) panel.vs.bottomTrackTheme = index }
        }
    }

    // ══ Rangefinder ═════════════════════════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs

        SectionLabel { text: qsTr("Rangefinder") }

        KSwitch {
            width: parent.width
            text: qsTr("Value")
            checked: panel.vs ? panel.vs.rangefinderValue : false
            onToggled: if (panel.vs) panel.vs.rangefinderValue = checked
        }
        KSwitch {
            width: parent.width
            text: qsTr("Graphics")
            checked: panel.vs ? panel.vs.rangefinderLine : false
            onToggled: if (panel.vs) panel.vs.rangefinderLine = checked
        }
        ComboRow {
            label: qsTr("Type")
            enabledRow: panel.vs ? panel.vs.rangefinderLine : false
            comboModel: [qsTr("Line"), qsTr("Points")]
            currentIndex: panel.vs ? panel.vs.rangefinderTheme : 0
            onPicked: function(index) { if (panel.vs) panel.vs.rangefinderTheme = index }
        }
    }

    // ══ Attitude / Temperature (instruments > 1) ════════════════════════════════
    KSwitch {
        width: parent.width
        visible: panel.instruments > 1
        text: qsTr("Attitude")
        checked: panel.vs ? panel.vs.ahrsVisible : false
        onToggled: if (panel.vs) panel.vs.ahrsVisible = checked
    }
    KSwitch {
        width: parent.width
        visible: panel.instruments > 1
        text: qsTr("Temperature")
        checked: panel.vs ? panel.vs.temperatureVisible : false
        onToggled: if (panel.vs) panel.vs.temperatureVisible = checked
    }

    // ══ Doppler Beams (instruments > 1) ══════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        visible: panel.instruments > 1
        label: qsTr("Doppler Beams")
        checked: panel.vs ? panel.vs.dopplerBeamVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.dopplerBeamVisible = v }

        Repeater {
            model: [
                { lbl: "1 " + qsTr("Depth"),    key: "dopplerBeam1A" },
                { lbl: "1 " + qsTr("Velocity"), key: "dopplerBeam1V" },
                { lbl: "1 " + qsTr("Mode"),     key: "dopplerBeam1M" },
                { lbl: "2 " + qsTr("Depth"),    key: "dopplerBeam2A" },
                { lbl: "2 " + qsTr("Velocity"), key: "dopplerBeam2V" },
                { lbl: "2 " + qsTr("Mode"),     key: "dopplerBeam2M" },
                { lbl: "3 " + qsTr("Depth"),    key: "dopplerBeam3A" },
                { lbl: "3 " + qsTr("Velocity"), key: "dopplerBeam3V" },
                { lbl: "3 " + qsTr("Mode"),     key: "dopplerBeam3M" },
                { lbl: "4 " + qsTr("Depth"),    key: "dopplerBeam4A" },
                { lbl: "4 " + qsTr("Velocity"), key: "dopplerBeam4V" },
                { lbl: "4 " + qsTr("Mode"),     key: "dopplerBeam4M" }
            ]
            delegate: KSwitch {
                required property var modelData
                width: parent ? parent.width : implicitWidth
                text: modelData.lbl
                checked: panel.vs ? panel.vs[modelData.key] : false
                onToggled: if (panel.vs) panel.vs[modelData.key] = checked
            }
        }
    }

    // ══ Doppler Instrument (instruments > 1) ═════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        visible: panel.instruments > 1
        label: qsTr("Doppler Instrument")
        checked: panel.vs ? panel.vs.dopplerInstrumentVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.dopplerInstrumentVisible = v }

        Repeater {
            model: [
                { lbl: "X",                  key: "dopplerInstrumentX" },
                { lbl: "Y",                  key: "dopplerInstrumentY" },
                { lbl: "Z",                  key: "dopplerInstrumentZ" },
                { lbl: qsTr("Abs. Velocity"), key: "dopplerInstrumentA" },
                { lbl: qsTr("Depth"),         key: "dopplerInstrumentDst" }
            ]
            delegate: KSwitch {
                required property var modelData
                width: parent ? parent.width : implicitWidth
                text: modelData.lbl
                checked: panel.vs ? panel.vs[modelData.key] : false
                onToggled: if (panel.vs) panel.vs[modelData.key] = checked
            }
        }
    }

    // ══ DVL Legend (instruments > 1) ═════════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        visible: panel.instruments > 1
        label: qsTr("DVL Legend")
        checked: panel.vs ? panel.vs.dvlLegendVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.dvlLegendVisible = v }

        ComboRow {
            label: qsTr("Position")
            comboModel: [qsTr("Top"), qsTr("Center"), qsTr("Bottom")]
            currentIndex: panel.vs ? panel.vs.dvlLegendPosition : 0
            onPicked: function(index) { if (panel.vs) panel.vs.dvlLegendPosition = index }
        }
    }

    // ══ Acoustic angle / Doppler Profiler / GNSS (instruments > 1) ═══════════════
    KSwitch {
        width: parent.width
        visible: panel.instruments > 1
        text: qsTr("Acoustic angle")
        checked: panel.vs ? panel.vs.acousticAngleVisible : false
        onToggled: if (panel.vs) panel.vs.acousticAngleVisible = checked
    }
    KSwitch {
        width: parent.width
        visible: panel.instruments > 1
        enabled: false
        text: qsTr("Doppler Profiler")
    }
    KSwitch {
        width: parent.width
        visible: panel.instruments > 1
        text: qsTr("GNSS data")
        checked: panel.vs ? panel.vs.gnssVisible : false
        onToggled: if (panel.vs) panel.vs.gnssVisible = checked
    }

    // ══ Grid + fill/invert/number ════════════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        label: qsTr("Grid")
        checked: panel.vs ? panel.vs.gridVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.gridVisible = v }

        KSwitch {
            width: parent.width
            text: qsTr("Fill width")
            checked: panel.vs ? panel.vs.gridFill : false
            onToggled: if (panel.vs) panel.vs.gridFill = checked
        }
        KSwitch {
            width: parent.width
            text: qsTr("Invert")
            checked: panel.vs ? panel.vs.gridInvert : false
            onToggled: if (panel.vs) panel.vs.gridInvert = checked
        }
        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd
            Text {
                text: qsTr("Vertical lines")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
            KSpinBox {
                Layout.preferredWidth: panel.comboW
                from: 1; to: 24; stepSize: 1
                value: panel.vs ? panel.vs.gridNumber : 5
                onValueModified: function(val) { if (panel.vs) panel.vs.gridNumber = val }
            }
        }
    }

    // ══ Angle range (instruments > 1) ════════════════════════════════════════════
    ParamCard {
        width: parent.width
        visible: panel.instruments > 1
        label: qsTr("Angle range, °")
        slotWidth: panel.comboW
        checked: panel.vs ? panel.vs.angleVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.angleVisible = v }
        KSpinBox {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: panel.comboW
            from: 1; to: 360; stepSize: 1
            value: panel.vs ? panel.vs.angleRange : 45
            onValueModified: function(val) { if (panel.vs) panel.vs.angleRange = val }
        }
    }

    // ══ Velocity range (instruments > 1) ═════════════════════════════════════════
    ParamCard {
        width: parent.width
        visible: panel.instruments > 1
        label: qsTr("Velocity range, m/s")
        slotWidth: panel.comboW
        checked: panel.vs ? panel.vs.velocityVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.velocityVisible = v }
        KSpinBox {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: panel.comboW
            from: 500; to: 8000; stepSize: 500; divisor: 1000; decimals: 1
            value: panel.vs ? panel.vs.velocityRange : 5000
            onValueModified: function(val) { if (panel.vs) panel.vs.velocityRange = val }
        }
    }

    // ══ Distance auto range + mode ════════════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        label: qsTr("Distance auto range")
        checked: panel.vs ? panel.vs.distanceAutoRange : false
        onToggled: function(v) { if (panel.vs) panel.vs.distanceAutoRange = v }

        ComboRow {
            label: qsTr("Mode")
            comboModel: [qsTr("Last data"), qsTr("Last on screen"), qsTr("Max on screen")]
            currentIndex: panel.vs ? panel.vs.distanceAutoRangeIndex : 0
            onPicked: function(index) { if (panel.vs) panel.vs.distanceAutoRangeIndex = index }
        }
    }

    // ══ Horizontal mode ═══════════════════════════════════════════════════════════
    KSwitch {
        width: parent.width
        text: qsTr("Horizontal")
        checked: panel.vs ? panel.vs.horizontalMode : false
        onToggled: if (panel.vs) panel.vs.horizontalMode = checked
    }

    // ══ Loupe + size + zoom ════════════════════════════════════════════════════════
    ParamCardGroup {
        width: parent.width
        label: qsTr("Loupe")
        checked: panel.vs ? panel.vs.loupeVisible : false
        onToggled: function(v) { if (panel.vs) panel.vs.loupeVisible = v }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd
            Text {
                text: qsTr("Size")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
            KSpinBox {
                Layout.preferredWidth: panel.comboW
                from: 1; to: 3; stepSize: 1
                value: panel.vs ? panel.vs.loupeSize : 1
                onValueModified: function(val) { if (panel.vs) panel.vs.loupeSize = val }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd
            Text {
                text: qsTr("Zoom")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                verticalAlignment: Text.AlignVCenter
            }
            KSlider {
                Layout.fillWidth: true
                from: 0; to: 300; stepSize: 1
                valueSuffix: "%"
                value: panel.vs ? panel.vs.loupeZoom : 100
                onPressedChanged: {
                    if (!panel.plot) return
                    if (pressed) panel.plot.beginLoupeZoomPreview()
                    else          panel.plot.endLoupeZoomPreview()
                }
                onMoved: {
                    if (!panel.plot) return
                    panel.vs.loupeZoom = Math.round(value)
                    panel.plot.updateLoupeZoomPreview()
                }
            }
            Text {
                text: (panel.vs ? Math.round(panel.vs.loupeZoom) : 0) + "%"
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                Layout.preferredWidth: Math.round(40 * AppPalette.scale)
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}

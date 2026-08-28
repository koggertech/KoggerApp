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
    property var store: null
    readonly property var vs: plot ? plot.viewState : null
    readonly property int instruments: theme ? theme.instrumentsGrade : 0
    readonly property var ds: (typeof dataset !== "undefined") ? dataset : null
    property bool hideEmpty: true
    function dataGate(present) { return !panel.hideEmpty || present }

    readonly property int comboW: Math.round(150 * AppPalette.scale)
    readonly property color labelInk: AppPalette.isDark ? "#FFFFFF" : AppPalette.text

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceXs

    // ── Section header label ──────────────────────────────────────────────────
    component SectionLabel: Text {
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontBase
        topPadding: Tokens.spaceXs
    }

    // ══ Channels (instruments > 1) ════════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        visible: panel.instruments > 1

        SectionLabel { text: qsTr("Channels") + ":" }

        Rectangle {
            width: parent.width
            implicitHeight: Math.round(38 * AppPalette.scale)
            radius: Tokens.radiusLg
            color: AppPalette.rowRaised

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Tokens.spaceMd
                anchors.rightMargin: Tokens.spaceMd
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
    }

    SectionLabel { text: qsTr("Echogram") + ":" }

    // ══ Echogram + theme + compensation ════════════════════════════════════════
    KIsland {
        KIslandRow {
            label: qsTr("Visibility")
            labelColor: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
            interactive: true
            onClicked: echogramVisibleSwitch.click()

            KSwitch {
                id: echogramVisibleSwitch
                flat: true
                checked: panel.vs ? panel.vs.echogramVisible : false
                onToggled: if (panel.vs) panel.vs.echogramVisible = checked
            }
        }

        KIslandRow {
            label: qsTr("Theme")
            labelColor: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
            verticalPadding: Tokens.spaceSm
            showSeparator: false
            open: panel.vs ? panel.vs.echogramVisible : false

            KCombo {
                width: panel.comboW
                model: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
                currentIndex: panel.vs ? panel.vs.echoThemeIndex : 0
                swatchFor: panel.plot ? function(i) { return panel.plot.echogramThemeStops(i) } : null
                onActivated: function(index) { if (panel.vs) panel.vs.echoThemeIndex = index }
            }
        }

        KIslandRow {
            label: qsTr("Source data")
            labelColor: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
            verticalPadding: Tokens.spaceSm
            showSeparator: false
            open: panel.vs ? panel.vs.echogramVisible : false

            Row {
                spacing: Tokens.spaceXs

                KCircleIconButton {
                    visible: sourceDataCombo.currentIndex === 2
                    width: Tokens.controlHMd
                    height: Tokens.controlHMd
                    iconSource: "qrc:/icons/ui/settings.svg"
                    iconTintColor: AppPalette.accentBar
                    rounded: false
                    cornerRadius: Tokens.radiusSm
                    fillColor: AppPalette.controlRaised
                    fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
                    borderWidth: 0
                    borderColor: "transparent"
                    toolTipText: qsTr("Open TGC settings")
                    onClicked: if (panel.store) panel.store.openTgcSettings()
                }

                KCombo {
                    id: sourceDataCombo
                    width: panel.comboW
                    model: [qsTr("Raw"), qsTr("Side-Scan"), qsTr("TGC")]
                    currentIndex: panel.vs ? panel.vs.compensationIndex : 0
                    onActivated: function(index) { if (panel.vs) panel.vs.compensationIndex = index }
                }
            }
        }
    }

    // ══ Bottom-Track (instruments > 0) ══════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        visible: panel.instruments > 0 && panel.dataGate(panel.ds && panel.ds.hasChartData)

        SectionLabel { text: qsTr("Bottom-Track") + ":" }

        KIsland {
            KIslandRow {
                label: qsTr("Visibility")
                labelColor: panel.labelInk
                interactive: true
                onClicked: bottomTrackVisibleSwitch.click()

                KSwitch {
                    id: bottomTrackVisibleSwitch
                    flat: true
                    checked: panel.vs ? panel.vs.bottomTrackLine : false
                    onToggled: if (panel.vs) panel.vs.bottomTrackLine = checked
                }
            }

            KIslandRow {
                label: qsTr("Type")
                labelColor: panel.labelInk
                verticalPadding: Tokens.spaceSm
                showSeparator: false
                open: panel.vs ? panel.vs.bottomTrackLine : false

                KCombo {
                    width: panel.comboW
                    model: [qsTr("Line"), qsTr("Points")]
                    currentIndex: panel.vs ? panel.vs.bottomTrackTheme : 0
                    onActivated: function(index) { if (panel.vs) panel.vs.bottomTrackTheme = index }
                }
            }
        }
    }

    // ══ Rangefinder ═════════════════════════════════════════════════════════════
    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        visible: panel.dataGate(panel.ds && panel.ds.hasRangefinderData)

        SectionLabel { text: qsTr("Rangefinder") + ":" }

        KIsland {
            KIslandRow {
                label: qsTr("Visibility")
                labelColor: panel.labelInk
                interactive: true
                onClicked: rangefinderVisibleSwitch.click()

                KSwitch {
                    id: rangefinderVisibleSwitch
                    flat: true
                    checked: panel.vs ? panel.vs.rangefinderLine : false
                    onToggled: if (panel.vs) panel.vs.rangefinderLine = checked
                }
            }

            KIslandRow {
                label: qsTr("Type")
                labelColor: panel.labelInk
                verticalPadding: Tokens.spaceSm
                showSeparator: false
                open: panel.vs ? panel.vs.rangefinderLine : false

                KCombo {
                    width: panel.comboW
                    model: [qsTr("Line"), qsTr("Points")]
                    currentIndex: panel.vs ? panel.vs.rangefinderTheme : 0
                    onActivated: function(index) { if (panel.vs) panel.vs.rangefinderTheme = index }
                }
            }
        }
    }

    SectionLabel {
        text: qsTr("Data") + ":"
        visible: panel.instruments > 1 && (
                     panel.dataGate(panel.ds && panel.ds.hasAttitudeData)
                  || panel.dataGate(panel.ds && panel.ds.hasDopplerBeamData)
                  || panel.dataGate(panel.ds && panel.ds.hasDvlSolutionData)
                  || panel.dataGate(panel.ds && panel.ds.hasUsblData)
                  || panel.dataGate(panel.ds && panel.ds.hasPositionData))
    }

    // ══ Attitude / Doppler / DVL / GNSS (instruments > 1) ═══════════════════════
    KIsland {
        visible: panel.instruments > 1 && (
                     panel.dataGate(panel.ds && panel.ds.hasAttitudeData)
                  || panel.dataGate(panel.ds && panel.ds.hasDopplerBeamData)
                  || panel.dataGate(panel.ds && panel.ds.hasDvlSolutionData)
                  || panel.dataGate(panel.ds && panel.ds.hasUsblData)
                  || panel.dataGate(panel.ds && panel.ds.hasPositionData))

        KIslandRow {
            visible: panel.dataGate(panel.ds && panel.ds.hasAttitudeData)
            label: qsTr("Attitude")
            labelColor: panel.labelInk
            interactive: true
            onClicked: attitudeSwitch.click()

            KSwitch {
                id: attitudeSwitch
                flat: true
                checked: panel.vs ? panel.vs.ahrsVisible : false
                onToggled: if (panel.vs) panel.vs.ahrsVisible = checked
            }
        }

        KIslandRow {
            id: dopplerBeamsRow
            visible: panel.dataGate(panel.ds && panel.ds.hasDopplerBeamData)
            label: qsTr("Doppler Beams")
            labelColor: panel.labelInk
            interactive: true
            onClicked: dopplerBeamsSwitch.click()

            KSwitch {
                id: dopplerBeamsSwitch
                flat: true
                checked: panel.vs ? panel.vs.dopplerBeamVisible : false
                onToggled: if (panel.vs) panel.vs.dopplerBeamVisible = checked
            }
        }

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
            delegate: KIslandRow {
                required property var modelData

                visible: dopplerBeamsRow.visible
                open: panel.vs ? panel.vs.dopplerBeamVisible : false
                showSeparator: false
                verticalPadding: Tokens.spaceSm
                label: modelData.lbl
                labelColor: panel.labelInk
                interactive: true
                onClicked: dopplerBeamSwitch.click()

                KSwitch {
                    id: dopplerBeamSwitch
                    flat: true
                    checked: panel.vs ? panel.vs[modelData.key] : false
                    onToggled: if (panel.vs) panel.vs[modelData.key] = checked
                }
            }
        }

        KIslandRow {
            id: dopplerInstrumentRow
            visible: panel.dataGate(panel.ds && panel.ds.hasDvlSolutionData)
            label: qsTr("Doppler Instrument")
            labelColor: panel.labelInk
            interactive: true
            onClicked: dopplerInstrumentSwitch.click()

            KSwitch {
                id: dopplerInstrumentSwitch
                flat: true
                checked: panel.vs ? panel.vs.dopplerInstrumentVisible : false
                onToggled: if (panel.vs) panel.vs.dopplerInstrumentVisible = checked
            }
        }

        Repeater {
            model: [
                { lbl: "X",                   key: "dopplerInstrumentX" },
                { lbl: "Y",                   key: "dopplerInstrumentY" },
                { lbl: "Z",                   key: "dopplerInstrumentZ" },
                { lbl: qsTr("Abs. Velocity"), key: "dopplerInstrumentA" },
                { lbl: qsTr("Depth"),         key: "dopplerInstrumentDst" }
            ]
            delegate: KIslandRow {
                required property var modelData

                visible: dopplerInstrumentRow.visible
                open: panel.vs ? panel.vs.dopplerInstrumentVisible : false
                showSeparator: false
                verticalPadding: Tokens.spaceSm
                label: modelData.lbl
                labelColor: panel.labelInk
                interactive: true
                onClicked: dopplerInstrumentValueSwitch.click()

                KSwitch {
                    id: dopplerInstrumentValueSwitch
                    flat: true
                    checked: panel.vs ? panel.vs[modelData.key] : false
                    onToggled: if (panel.vs) panel.vs[modelData.key] = checked
                }
            }
        }

        KIslandRow {
            id: dvlLegendRow
            visible: panel.dataGate(panel.ds && panel.ds.hasDvlSolutionData)
            label: qsTr("DVL Legend")
            labelColor: panel.labelInk
            interactive: true
            onClicked: dvlLegendSwitch.click()

            KSwitch {
                id: dvlLegendSwitch
                flat: true
                checked: panel.vs ? panel.vs.dvlLegendVisible : false
                onToggled: if (panel.vs) panel.vs.dvlLegendVisible = checked
            }
        }

        KIslandRow {
            visible: dvlLegendRow.visible
            open: panel.vs ? panel.vs.dvlLegendVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Position")
            labelColor: panel.labelInk

            KCombo {
                width: panel.comboW
                model: [qsTr("Top"), qsTr("Center"), qsTr("Bottom")]
                currentIndex: panel.vs ? panel.vs.dvlLegendPosition : 0
                onActivated: function(index) { if (panel.vs) panel.vs.dvlLegendPosition = index }
            }
        }

        KIslandRow {
            visible: panel.dataGate(panel.ds && panel.ds.hasUsblData)
            label: qsTr("Acoustic angle")
            labelColor: panel.labelInk
            interactive: true
            onClicked: acousticAngleSwitch.click()

            KSwitch {
                id: acousticAngleSwitch
                flat: true
                checked: panel.vs ? panel.vs.acousticAngleVisible : false
                onToggled: if (panel.vs) panel.vs.acousticAngleVisible = checked
            }
        }

        KIslandRow {
            visible: panel.dataGate(panel.ds && panel.ds.hasDopplerBeamData)
            enabled: false
            label: qsTr("Doppler Profiler")
            labelColor: AppPalette.textMuted

            KSwitch { flat: true }
        }

        KIslandRow {
            visible: panel.dataGate(panel.ds && panel.ds.hasPositionData)
            label: qsTr("GNSS data")
            labelColor: panel.labelInk
            interactive: true
            onClicked: gnssSwitch.click()

            KSwitch {
                id: gnssSwitch
                flat: true
                checked: panel.vs ? panel.vs.gnssVisible : false
                onToggled: if (panel.vs) panel.vs.gnssVisible = checked
            }
        }
    }

    SectionLabel { text: qsTr("Display parameters") + ":" }

    // ══ Grid / distance auto range / horizontal mode ════════════════════════════
    KIsland {
        KIslandRow {
            label: qsTr("Grid")
            labelColor: panel.labelInk
            interactive: true
            onClicked: gridSwitch.click()

            KSwitch {
                id: gridSwitch
                flat: true
                checked: panel.vs ? panel.vs.gridVisible : false
                onToggled: if (panel.vs) panel.vs.gridVisible = checked
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.gridVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Fill width")
            labelColor: panel.labelInk
            interactive: true
            onClicked: gridFillSwitch.click()

            KSwitch {
                id: gridFillSwitch
                flat: true
                checked: panel.vs ? panel.vs.gridFill : false
                onToggled: if (panel.vs) panel.vs.gridFill = checked
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.gridVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Invert")
            labelColor: panel.labelInk
            interactive: true
            onClicked: gridInvertSwitch.click()

            KSwitch {
                id: gridInvertSwitch
                flat: true
                checked: panel.vs ? panel.vs.gridInvert : false
                onToggled: if (panel.vs) panel.vs.gridInvert = checked
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.gridVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Vertical lines")
            labelColor: panel.labelInk

            KSpinBox {
                width: panel.comboW
                height: Tokens.controlHMd
                from: 1; to: 24; stepSize: 1
                value: panel.vs ? panel.vs.gridNumber : 5
                onValueModified: function(val) { if (panel.vs) panel.vs.gridNumber = val }
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.gridVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Angle range, °")
            labelColor: panel.labelInk
            interactive: true
            onClicked: angleRangeSwitch.click()

            Item {
                width: angleRangeSlotRow.width
                height: angleRangeSlotRow.height

                // Eats clicks landing in the gaps around the spinbox so they
                // cannot bubble up to the row and flip the toggle.
                MouseArea { anchors.fill: parent }

                Row {
                    id: angleRangeSlotRow
                    spacing: Tokens.spaceSm

                    KSpinBox {
                        width: panel.comboW
                        height: Tokens.controlHMd
                        from: 1; to: 360; stepSize: 1
                        value: panel.vs ? panel.vs.angleRange : 45
                        onValueModified: function(val) { if (panel.vs) panel.vs.angleRange = val }
                    }

                    KSwitch {
                        id: angleRangeSwitch
                        flat: true
                        checked: panel.vs ? panel.vs.angleVisible : false
                        onToggled: if (panel.vs) panel.vs.angleVisible = checked
                    }
                }
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.gridVisible : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Velocity range, m/s")
            labelColor: panel.labelInk
            interactive: true
            onClicked: velocityRangeSwitch.click()

            Item {
                width: velocityRangeSlotRow.width
                height: velocityRangeSlotRow.height

                MouseArea { anchors.fill: parent }

                Row {
                    id: velocityRangeSlotRow
                    spacing: Tokens.spaceSm

                    KSpinBox {
                        width: panel.comboW
                        height: Tokens.controlHMd
                        from: 500; to: 8000; stepSize: 500; divisor: 1000; decimals: 1
                        value: panel.vs ? panel.vs.velocityRange : 5000
                        onValueModified: function(val) { if (panel.vs) panel.vs.velocityRange = val }
                    }

                    KSwitch {
                        id: velocityRangeSwitch
                        flat: true
                        checked: panel.vs ? panel.vs.velocityVisible : false
                        onToggled: if (panel.vs) panel.vs.velocityVisible = checked
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("Distance auto range")
            labelColor: panel.labelInk
            interactive: true
            onClicked: distanceAutoRangeSwitch.click()

            KSwitch {
                id: distanceAutoRangeSwitch
                flat: true
                checked: panel.vs ? panel.vs.distanceAutoRange : false
                onToggled: if (panel.vs) panel.vs.distanceAutoRange = checked
            }
        }

        KIslandRow {
            open: panel.vs ? panel.vs.distanceAutoRange : false
            showSeparator: false
            verticalPadding: Tokens.spaceSm
            label: qsTr("Mode")
            labelColor: panel.labelInk

            KCombo {
                width: panel.comboW
                model: [qsTr("Last data"), qsTr("Last on screen"), qsTr("Max on screen")]
                currentIndex: panel.vs ? panel.vs.distanceAutoRangeIndex : 0
                onActivated: function(index) { if (panel.vs) panel.vs.distanceAutoRangeIndex = index }
            }
        }

        KIslandRow {
            label: qsTr("Horizontal")
            labelColor: panel.labelInk
            interactive: true
            onClicked: horizontalModeSwitch.click()

            KSwitch {
                id: horizontalModeSwitch
                flat: true
                checked: panel.vs ? panel.vs.horizontalMode : false
                onToggled: if (panel.vs) panel.vs.horizontalMode = checked
            }
        }
    }

}

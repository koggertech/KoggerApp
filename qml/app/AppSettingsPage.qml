import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0

Column {
    id: root

    required property var store
    property var targetPlot: null

    readonly property int instruments: theme ? theme.instrumentsGrade : 0
    readonly property real groupWidth: Math.max(0, width)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    // Parameter row card — matches KSwitch's full-width pattern.
    //
    //   [ label                  (interactive area)              [TOGGLE] ]
    //                                  [ optional spinbox slot ]
    //
    // Click anywhere on the card (except the spinbox area) flips the toggle.
    // Hover highlights the whole card. The default property is a content slot
    // sized by `slotWidth` — drop a KSpinBox (or any control) inside it.
    component ParamCard: Rectangle {
        id: pcard

        property string label: ""
        property bool checked: false
        property int slotWidth: 0
        signal toggled(bool val)

        default property alias contentData: pcardSlot.data

        readonly property int _knobMargin: Math.max(2, Math.round(2 * AppPalette.scale))
        readonly property bool _hovered: cardMouseLeft.containsMouse || cardMouseRight.containsMouse

        width: parent ? parent.width : implicitWidth
        height: Math.round(38 * AppPalette.scale)
        radius: Tokens.radiusLg
        color: pcard._hovered ? AppPalette.bgHover : AppPalette.bg
        border.width: 1
        border.color: pcard._hovered ? AppPalette.borderHover : AppPalette.border

        Behavior on color       { ColorAnimation { duration: 110 } }
        Behavior on border.color { ColorAnimation { duration: 110 } }

        function _flip() {
            pcard.checked = !pcard.checked
            pcard.toggled(pcard.checked)
        }

        // ── Click-catchers: two MouseAreas flanking pcardSlot ──────────────
        // Splitting around the slot prevents stray clicks in the slot's
        // un-widget-covered gaps (e.g. between a KSpinBox's text and its +/−
        // buttons) from bubbling up and accidentally toggling the card.
        MouseArea {
            id: cardMouseLeft
            anchors.left: parent.left
            anchors.right: pcardSlot.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            z: -1
            onClicked: pcard._flip()
        }
        MouseArea {
            id: cardMouseRight
            anchors.left: pcardSlot.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            z: -1
            onClicked: pcard._flip()
        }

        // ── Label on the left ──────────────────────────────────────────────
        Text {
            anchors.left: parent.left
            anchors.leftMargin: Tokens.spaceMd
            anchors.right: pcardSlot.left
            anchors.rightMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            text: pcard.label
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontMd
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        // ── Optional content slot (spinbox etc.) between label and toggle ──
        Item {
            id: pcardSlot
            width: pcard.slotWidth
            height: parent.height
            anchors.right: pcardToggle.left
            anchors.rightMargin: pcard.slotWidth > 0 ? Tokens.spaceMd : 0
        }

        // ── Toggle pill on the right (same look as KSwitch indicator) ─────
        Item {
            id: pcardToggle
            width: Math.round(44 * AppPalette.scale)
            height: Math.round(24 * AppPalette.scale)
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: pcard.checked ? AppPalette.accentBg : AppPalette.trackOff
                border.width: 1
                border.color: pcard.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder
                Behavior on color { ColorAnimation { duration: 120 } }

                Rectangle {
                    width: parent.height - 2 * pcard._knobMargin
                    height: width
                    radius: width / 2
                    y: pcard._knobMargin
                    x: pcard.checked ? parent.width - width - pcard._knobMargin : pcard._knobMargin
                    color: AppPalette.knob
                    border.width: 1
                    border.color: "#00000022"
                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }
            }
        }
    }

    // Inline toggle switch for parameter rows — same visual size as KSwitch's
    // indicator so all toggles in the app look identical and are easy to tap.
    component SmallCheck: Item {
        id: sc
        property bool checked: false
        signal toggled(bool val)

        readonly property int _knobMargin: Math.max(2, Math.round(2 * AppPalette.scale))

        width: Math.round(44 * AppPalette.scale)
        height: Math.round(24 * AppPalette.scale)

        Rectangle {
            id: scTrack
            anchors.fill: parent
            radius: height / 2
            color: sc.checked ? AppPalette.accentBg : AppPalette.trackOff
            border.width: 1
            border.color: sc.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder

            Behavior on color { ColorAnimation { duration: 120 } }

            Rectangle {
                width: parent.height - 2 * sc._knobMargin
                height: width
                radius: width / 2
                y: sc._knobMargin
                x: sc.checked ? parent.width - width - sc._knobMargin : sc._knobMargin
                color: AppPalette.knob
                border.width: 1
                border.color: "#00000022"

                Behavior on x {
                    NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { sc.checked = !sc.checked; sc.toggled(sc.checked) }
        }
    }

    // URL helpers (for export/import paths)
    function localPath(value) {
        if (!value) return ""
        if (typeof value === "string") {
            if (value.startsWith("file:///"))
                return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
            if (value.startsWith("file://"))
                return value.slice(7)
            return value
        }
        var lp = value.toLocalFile ? value.toLocalFile() : ""
        return lp.length ? lp : value.toString()
    }

    function displayPath(value) {
        var s = localPath(value)
        if (!s.length) return ""
        try { return decodeURIComponent(s) } catch(e) { return s }
    }

    onTargetPlotChanged: {
        if (root.targetPlot) btGroup.refreshParams()
    }

    // ── Connections ──────────────────────────────────────────────────────────

    ConnectionsSettingsPage {
        width: root.groupWidth
        store: root.store
    }

    // ── Интерфейс ─────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Interface")
        description: qsTr("Language, theme, UI scale and panel visibility.")
        stateStore: root.store
        stateKey: "app.preference"
        collapsedByDefault: false

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Language:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            KTabBar {
                id: langTabBar
                width: parent.width
                options: [
                    { label: qsTr("English"), value: 0 },
                    { label: qsTr("Russian"), value: 1 },
                    { label: qsTr("Polish"),  value: 2 }
                ]
                currentValue: langController ? langController.currentIndex : 0
                onValueSelected: function(v) { if (langController) langController.apply(v) }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Theme:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: appThemeHolder
                width: parent.width
                property int selectedIndex: 0

                readonly property var names: ["Dark","S.Dark","Light","S.Light","OneDark","Monokai","Kimbie","Solar","Desert","Steam 2003"]
                readonly property int gap: Tokens.spaceXs
                readonly property int cellMinW: Math.round(80 * AppPalette.scale)
                readonly property int cols: Tokens.gridColumns(width, cellMinW, gap, 5)
                readonly property int itemH: Tokens.controlHMd
                readonly property real itemW: (width - (cols - 1) * gap) / cols
                readonly property int rows: Math.ceil(10 / cols)
                height: rows * itemH + (rows - 1) * gap

                onSelectedIndexChanged: if (theme) theme.themeID = selectedIndex
                Component.onCompleted: if (theme) theme.themeID = selectedIndex

                Repeater {
                    model: 10
                    delegate: Rectangle {
                        required property int index
                        readonly property bool sel: index === appThemeHolder.selectedIndex
                        x: (index % appThemeHolder.cols) * (appThemeHolder.itemW + appThemeHolder.gap)
                        y: Math.floor(index / appThemeHolder.cols) * (appThemeHolder.itemH + appThemeHolder.gap)
                        width: appThemeHolder.itemW
                        height: appThemeHolder.itemH
                        radius: Tokens.radiusMd
                        color: sel ? AppPalette.accentBg : AppPalette.bg
                        border.width: 1
                        border.color: sel ? AppPalette.accentBorder : AppPalette.border

                        Text {
                            anchors.centerIn: parent
                            text: appThemeHolder.names[index]
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontXs
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appThemeHolder.selectedIndex = index
                        }
                    }
                }

                Settings { property alias appTheme: appThemeHolder.selectedIndex }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Feature level:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: instrumentsGradeHolder
                width: parent.width
                height: gradeTabBar.implicitHeight
                property int selectedIndex: 0

                onSelectedIndexChanged: if (theme) theme.instrumentsGrade = selectedIndex
                Component.onCompleted: if (theme) theme.instrumentsGrade = selectedIndex

                KTabBar {
                    id: gradeTabBar
                    width: parent.width
                    options: [
                        { label: qsTr("Fish Finders"),  value: 0 },
                        { label: qsTr("Bottom Track"),  value: 1 },
                        { label: qsTr("Maximum"),       value: 2 }
                    ]
                    currentValue: instrumentsGradeHolder.selectedIndex
                    onValueSelected: function(v) { instrumentsGradeHolder.selectedIndex = v }
                }

                Settings { property alias instrumentsGradeList: instrumentsGradeHolder.selectedIndex }
            }
        }

        // UI scale — DPI auto-detect + user override. Persisted via theme.manualScale.
        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Row {
                width: parent.width
                spacing: Tokens.spaceMd
                Text {
                    text: qsTr("UI scale:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: theme ? Math.round(theme.manualScale * 100) + "%" : "100%"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KTabBar {
                id: uiScaleTabBar
                width: parent.width
                options: [
                    { label: "75%",  value: 0.75 },
                    { label: "100%", value: 1.00 },
                    { label: "125%", value: 1.25 },
                    { label: "150%", value: 1.50 },
                    { label: "200%", value: 2.00 }
                ]
                currentValue: theme ? theme.manualScale : 1.0
                onValueSelected: function(v) { if (theme) theme.manualScale = v }
            }
        }

        // ── Merged from former "Interface" group ──────────────────────────

        KSwitch {
            id: consoleVisible
            visible: instruments >= 2
            width: parent.width
            text: qsTr("Console")
            checked: theme ? theme.consoleVisible : false
            onToggled: if (theme) theme.consoleVisible = checked

            Connections {
                target: theme
                ignoreUnknownSignals: true
                function onInterfaceChanged() {
                    if (consoleVisible.checked !== theme.consoleVisible)
                        consoleVisible.checked = theme.consoleVisible
                }
            }
        }

        Settings { property alias consoleVisible: consoleVisible.checked }

        KButton {
            visible: Qt.platform.os !== "android"
            width: parent.width
            text: qsTr("Hotkeys")
            onClicked: { hotkeysLoader.active = true; hotkeysLoader.item.open() }
        }

        Loader {
            id: hotkeysLoader
            active: false
            source: "qrc:/qml/settings/HotkeysDialog.qml"
            onLoaded: { if (item) item.store = root.store }
        }
    }

    // ── Датасет ───────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Dataset")
        description: qsTr("Black-stripe smoothing and sonar mount-point offset.")
        stateStore: root.store
        stateKey: "app.dataset"
        collapsedByDefault: true

        Component.onCompleted: {
            core.setFixBlackStripesState(fixBlackStripesCheckButton.checked)
            core.setFixBlackStripesForwardSteps(fixBlackStripesForwardStepsSpinBox.value)
            core.setFixBlackStripesBackwardSteps(fixBlackStripesBackwardStepsSpinBox.value)
            core.setIsAttitudeExpected(sonarOffsetCheckButton.checked)
        }

        // FBS row
        ParamCard {
            id: fixBlackStripesCheckButton
            label: qsTr("FBS forward / backward:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs
            onToggled: function(v) { core.setFixBlackStripesState(v) }

            KSpinBox {
                id: fixBlackStripesForwardStepsSpinBox
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 5
                onValueModified: function(v) { core.setFixBlackStripesForwardSteps(v) }
            }

            KSpinBox {
                id: fixBlackStripesBackwardStepsSpinBox
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: fixBlackStripesForwardStepsSpinBox.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 5
                onValueModified: function(v) { core.setFixBlackStripesBackwardSteps(v) }
            }
        }

        Settings { property alias fixBlackStripesCheckButton: fixBlackStripesCheckButton.checked }
        Settings { property alias fixBlackStripesForwardStepsSpinBox: fixBlackStripesForwardStepsSpinBox.value }
        Settings { property alias fixBlackStripesBackwardStepsSpinBox: fixBlackStripesBackwardStepsSpinBox.value }

        // Sonar offset row
        ParamCard {
            id: sonarOffsetCheckButton
            label: qsTr("S.offset XY, mm:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs
            onToggled: function(v) {
                if (v) dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, sonarOffsetValueY.value * 0.001, 0)
                else   dataset.setSonarOffset(0, 0, 0)
                core.setIsAttitudeExpected(v)
            }

            KSpinBox {
                id: sonarOffsetValueX
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) {
                    if (sonarOffsetCheckButton.checked)
                        dataset.setSonarOffset(v * 0.001, sonarOffsetValueY.value * 0.001, 0)
                }
            }

            KSpinBox {
                id: sonarOffsetValueY
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: sonarOffsetValueX.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) {
                    if (sonarOffsetCheckButton.checked)
                        dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, v * 0.001, 0)
                }
            }
        }

        Settings { property alias sonarOffsetCheckButton: sonarOffsetCheckButton.checked }
        Settings { property alias sonarOffsetValueX: sonarOffsetValueX.value }
        Settings { property alias sonarOffsetValueY: sonarOffsetValueY.value }
    }

    // ── Трек дна ──────────────────────────────────────────────────────────────

    SettingsGroup {
        id: btGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Bottom Track")
        description: qsTr("Bottom detection presets, thresholds and search window.")
        stateStore: root.store
        stateKey: "app.bottomtrack"
        collapsedByDefault: false

        readonly property int spinW: Math.round(115 * AppPalette.scale)

        function refreshParams() {
            if (!root.targetPlot) return
            root.targetPlot.refreshDistParams(
                btPresetHolder.selectedIndex,
                bottomTrackWindow.checked    ? bottomTrackWindowValue.value : 1,
                bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value * 0.01 : 0,
                bottomTrackMinRange.checked  ? bottomTrackMinRangeValue.value / 1000 : 0,
                bottomTrackMaxRange.checked  ? bottomTrackMaxRangeValue.value / 1000 : 1000,
                bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.value / 100  : 1,
                bottomTrackThreshold.checked ? bottomTrackThresholdValue.value / 100  : 0,
                bottomTrackSensorOffset.checked ? btOffX.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffY.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffZ.value * -0.001 : 0
            )
        }

        function doDistProcessing() {
            if (!root.targetPlot) return
            root.targetPlot.doDistProcessing(
                btPresetHolder.selectedIndex,
                bottomTrackWindow.checked    ? bottomTrackWindowValue.value : 1,
                bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value * 0.01 : 0,
                bottomTrackMinRange.checked  ? bottomTrackMinRangeValue.value / 1000 : 0,
                bottomTrackMaxRange.checked  ? bottomTrackMaxRangeValue.value / 1000 : 1000,
                bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.value / 100  : 1,
                bottomTrackThreshold.checked ? bottomTrackThresholdValue.value / 100  : 0,
                bottomTrackSensorOffset.checked ? btOffX.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffY.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffZ.value * -0.001 : 0,
                false
            )
        }

        Component.onCompleted: refreshParams()

        // Preset
        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Preset:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: btPresetHolder
                width: parent.width
                height: presetTabBar.implicitHeight
                property int selectedIndex: 0

                onSelectedIndexChanged: if (root.targetPlot) root.targetPlot.setPreset(selectedIndex)

                KTabBar {
                    id: presetTabBar
                    width: parent.width
                    options: [
                        { label: qsTr("Normal 2D"), value: 0 },
                        { label: qsTr("Narrow 2D"), value: 1 },
                        { label: qsTr("Side-Scan"), value: 2 }
                    ]
                    currentValue: btPresetHolder.selectedIndex
                    onValueSelected: function(v) { btPresetHolder.selectedIndex = v }
                }

                Settings { property alias bottomTrackList: btPresetHolder.selectedIndex }
            }
        }

        // Gain slope
        ParamCard {
            id: bottomTrackGainSlope
            label: qsTr("Gain slope:")
            checked: true
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setGainSlope(bottomTrackGainSlopeValue.value / 100) }

            KSpinBox {
                id: bottomTrackGainSlopeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 300; stepSize: 10; value: 100; divisor: 100; decimals: 2
                onValueModified: function(v) { if (bottomTrackGainSlope.checked && root.targetPlot) root.targetPlot.setGainSlope(v / 100) }
            }
        }
        Settings { property alias bottomTrackGainSlope: bottomTrackGainSlope.checked }
        Settings { property alias bottomTrackGainSlopeValue: bottomTrackGainSlopeValue.value }

        // Threshold
        ParamCard {
            id: bottomTrackThreshold
            label: qsTr("Threshold:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setThreshold(bottomTrackThresholdValue.value / 100) }

            KSpinBox {
                id: bottomTrackThresholdValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200; stepSize: 5; value: 0; divisor: 100; decimals: 2
                onValueModified: function(v) { if (bottomTrackThreshold.checked && root.targetPlot) root.targetPlot.setThreshold(v / 100) }
            }
        }
        Settings { property alias bottomTrackThreshold: bottomTrackThreshold.checked }
        Settings { property alias bottomTrackThresholdValue: bottomTrackThresholdValue.value }

        // Horizontal window
        ParamCard {
            id: bottomTrackWindow
            label: qsTr("Horizontal window:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setWindowSize(bottomTrackWindowValue.value) }

            KSpinBox {
                id: bottomTrackWindowValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 1; to: 100; stepSize: 2; value: 1
                onValueModified: function(v) { if (bottomTrackWindow.checked && root.targetPlot) root.targetPlot.setWindowSize(v) }
            }
        }
        Settings { property alias bottomTrackWindow: bottomTrackWindow.checked }
        Settings { property alias bottomTrackWindowValue: bottomTrackWindowValue.value }

        // Vertical gap
        ParamCard {
            id: bottomTrackVerticalGap
            label: qsTr("Vertical gap, %:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01) }

            KSpinBox {
                id: bottomTrackVerticalGapValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 100; stepSize: 2; value: 10
                onValueModified: function(v) { if (bottomTrackVerticalGap.checked && root.targetPlot) root.targetPlot.setVerticalGap(v * 0.01) }
            }
        }
        Settings { property alias bottomTrackVerticalGap: bottomTrackVerticalGap.checked }
        Settings { property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value }

        // Min range
        ParamCard {
            id: bottomTrackMinRange
            label: qsTr("Min range, m:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMin(bottomTrackMinRangeValue.value / 1000) }

            KSpinBox {
                id: bottomTrackMinRangeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200000; stepSize: 10; value: 0; divisor: 1000; decimals: 2
                onValueModified: function(v) { if (bottomTrackMinRange.checked && root.targetPlot) root.targetPlot.setRangeMin(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMinRange: bottomTrackMinRange.checked }
        Settings { property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value }

        // Max range
        ParamCard {
            id: bottomTrackMaxRange
            label: qsTr("Max range, m:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMax(bottomTrackMaxRangeValue.value / 1000) }

            KSpinBox {
                id: bottomTrackMaxRangeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200000; stepSize: 1000; value: 100000; divisor: 1000; decimals: 2
                onValueModified: function(v) { if (bottomTrackMaxRange.checked && root.targetPlot) root.targetPlot.setRangeMax(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMaxRange: bottomTrackMaxRange.checked }
        Settings { property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value }

        // Sensor offset (label row + values row)
        ParamCard {
            id: bottomTrackSensorOffset
            label: qsTr("Sonar offset XYZ, mm:")
            onToggled: function(v) {
                if (v && root.targetPlot) {
                    root.targetPlot.setOffsetX(btOffX.value *  0.001)
                    root.targetPlot.setOffsetY(btOffY.value *  0.001)
                    root.targetPlot.setOffsetZ(btOffZ.value *  0.001)
                }
            }
        }
        Row {
            visible: bottomTrackSensorOffset.checked
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceXs
            readonly property real sw: (width - 2 * Tokens.spaceXs) / 3

            KSpinBox {
                id: btOffX
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetX(v * 0.001) }
            }
            KSpinBox {
                id: btOffY
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetY(v * 0.001) }
            }
            KSpinBox {
                id: btOffZ
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetZ(v * 0.001) }
            }
        }
        Settings { property alias bottomTrackSensorOffset: bottomTrackSensorOffset.checked }
        Settings { property alias bottomTrackSensorOffsetValueX: btOffX.value }
        Settings { property alias bottomTrackSensorOffsetValueY: btOffY.value }
        Settings { property alias bottomTrackSensorOffsetValueZ: btOffZ.value }

        // Action buttons
        Row {
            width: parent.width; spacing: Tokens.spaceMd
            readonly property real bw: (width - Tokens.spaceMd) / 2

            KButton {
                width: parent.bw
                text: qsTr("Processing")
                onClicked: btGroup.doDistProcessing()
            }

            KButton {
                id: btRealtimeBtn
                width: parent.bw
                text: qsTr("Realtime")
                checkable: true
                checked: false
                onToggled: core.setBottomTrackRealtimeFromSettings(checked)
                Component.onCompleted: core.setBottomTrackRealtimeFromSettings(false)
            }
        }
    }

    // ── TGC ──────────────────────────────────────────────────────────────────

    SettingsGroup {
        id: tgcGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("TGC")
        description: qsTr("Time-varying gain and depth-based amplification curve.")
        stateStore: root.store
        stateKey: "app.tgc"
        collapsedByDefault: true

        readonly property int valueLabelW: Math.round(60 * AppPalette.scale)
        readonly property int labelW: Math.round(92 * AppPalette.scale)

        Component.onCompleted: {
            core.setTgcGainNear(tgcGainNearSlider.value * 0.01)
            core.setTgcGainFar(tgcGainFarSlider.value * 0.01)
            core.setTgcCompensate(tgcCompensateSwitch.checked)
        }

        // Near gain
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Text {
                text: qsTr("Near gain:")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
                width: tgcGroup.labelW
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        tgcGainNearSlider.value = 100
                        core.setTgcGainNear(1.0)
                    }
                }
            }

            KSlider {
                id: tgcGainNearSlider
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 2 * Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 500; stepSize: 1; value: 50
                valueSuffix: "%"
                onValueModified: function(v) { core.setTgcGainNear(v * 0.01) }
            }

            Text {
                width: tgcGroup.valueLabelW
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                text: tgcGainNearSlider.value + "%"
                color: AppPalette.text; font.pixelSize: Tokens.fontMd
            }
        }

        Settings { property alias appTgcGainNear: tgcGainNearSlider.value }

        // Far gain
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Text {
                text: qsTr("Far gain:")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
                width: tgcGroup.labelW
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        tgcGainFarSlider.value = 100
                        core.setTgcGainFar(1.0)
                    }
                }
            }

            KSlider {
                id: tgcGainFarSlider
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 2 * Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 1000; stepSize: 1; value: 250
                valueSuffix: "%"
                onValueModified: function(v) { core.setTgcGainFar(v * 0.01) }
            }

            Text {
                width: tgcGroup.valueLabelW
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                text: tgcGainFarSlider.value + "%"
                color: AppPalette.text; font.pixelSize: Tokens.fontMd
            }
        }

        Settings { property alias appTgcGainFar: tgcGainFarSlider.value }

        // Curve preview
        Canvas {
            id: tgcCurveCanvas
            width: parent.width
            height: Math.round(100 * AppPalette.scale)

            Connections {
                target: tgcGainNearSlider
                function onValueChanged() { tgcCurveCanvas.requestPaint() }
            }
            Connections {
                target: tgcGainFarSlider
                function onValueChanged() { tgcCurveCanvas.requestPaint() }
            }

            onPaint: {
                var ctx = getContext("2d")
                var w = width
                var h = height

                ctx.fillStyle = AppPalette.bg
                ctx.fillRect(0, 0, w, h)

                var gNear = tgcGainNearSlider.value / 100.0
                var gFar  = tgcGainFarSlider.value / 100.0

                var yMax = Math.max(gNear, gFar, 1.0) * 1.15
                if (yMax < 0.5) yMax = 0.5

                function yFor(g) { return h - (g / yMax) * h }

                ctx.strokeStyle = AppPalette.border
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(0, h - 0.5)
                ctx.lineTo(w, h - 0.5)
                ctx.stroke()

                var y100 = yFor(1.0)
                ctx.strokeStyle = AppPalette.text
                ctx.globalAlpha = 0.35
                if (ctx.setLineDash) ctx.setLineDash([3, 3])
                ctx.beginPath()
                ctx.moveTo(0, y100)
                ctx.lineTo(w, y100)
                ctx.stroke()
                if (ctx.setLineDash) ctx.setLineDash([])
                ctx.globalAlpha = 1.0

                ctx.strokeStyle = "#F07000"
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.moveTo(0, yFor(gNear))
                ctx.lineTo(w, yFor(gFar))
                ctx.stroke()

                ctx.fillStyle = AppPalette.text
                ctx.globalAlpha = 0.6
                ctx.font = "10px sans-serif"
                ctx.fillText("100%", 4, Math.max(y100 - 2, 10))
                ctx.globalAlpha = 1.0
            }
        }

        // Compensate
        KSwitch {
            id: tgcCompensateSwitch
            width: parent.width
            text: qsTr("Compensate")
            checked: false
            onToggled: core.setTgcCompensate(checked)
        }

        Settings { property alias appTgcCompensate: tgcCompensateSwitch.checked }
    }

    // ── Экспорт ───────────────────────────────────────────────────────────────

    SettingsGroup {
        id: exportGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Export")
        description: qsTr("Export plot data as XTF, CSV (regular or complex) or USBL.")
        stateStore: root.store
        stateKey: "app.export"
        collapsedByDefault: true

        property var exportFolderUrl: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property string exportFolderSource: ""

        Component.onCompleted: exportPathField.text = root.displayPath(exportFolderSource)

        function currentExportPath() {
            var t = exportPathField.text
            if (!t.length) return ""
            if (exportFolderSource.length && t === root.displayPath(exportFolderSource))
                return root.localPath(exportFolderSource)
            return t
        }

        // Path row
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Rectangle {
                width: parent.width - Math.round(44 * AppPalette.scale) - Tokens.spaceMd
                height: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: 1
                border.color: exportPathField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: exportPathField
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontSm
                    clip: true

                    Text {
                        visible: !exportPathField.text.length
                        text: qsTr("Export path...")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            KButton {
                width: Math.round(44 * AppPalette.scale); height: Tokens.controlHMd; text: "..."
                onClicked: {
                    exportFolderDialog.currentFolder = exportGroup.exportFolderUrl
                    exportFolderDialog.open()
                }
            }

            FolderDialog {
                id: exportFolderDialog
                title: qsTr("Export folder")
                onAccepted: {
                    exportGroup.exportFolderUrl = exportFolderDialog.currentFolder
                    exportGroup.exportFolderSource = root.localPath(exportFolderDialog.selectedFolder)
                    exportPathField.text = root.displayPath(exportGroup.exportFolderSource)
                }
            }
        }

        Settings { property alias exportFolder:     exportGroup.exportFolderUrl }
        Settings { property alias exportFolderText: exportGroup.exportFolderSource }

        // Decimation + CSV
        ParamCard {
            id: exportDecimation
            label: qsTr("Decimation, m:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs

            KSpinBox {
                id: exportDecimationValue
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 10
            }

            KButton {
                width: Math.round(93 * AppPalette.scale); height: Tokens.controlHMd
                anchors.left: exportDecimationValue.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("CSV")
                onClicked: {
                    if (root.targetPlot)
                        core.exportPlotAsCVS(exportGroup.currentExportPath(), root.targetPlot.plotDatasetChannel(),
                                             exportDecimation.checked ? exportDecimationValue.value : 0)
                }
            }
        }

        Settings { property alias exportDecimation:      exportDecimation.checked }
        Settings { property alias exportDecimationValue: exportDecimationValue.value }

        KButton {
            width: parent.width; text: qsTr("Export to XTF")
            onClicked: core.exportPlotAsXTF(exportGroup.currentExportPath())
        }

        KButton {
            width: parent.width; text: qsTr("Complex signal to CSV")
            onClicked: core.exportComplexToCSV(exportGroup.currentExportPath())
        }

        KButton {
            width: parent.width; text: qsTr("USBL to CSV")
            onClicked: core.exportUSBLToCSV(exportGroup.currentExportPath())
        }
    }

    // ── Сохранение UI ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("UI Saving")
        description: qsTr("Save the current workspace layout and settings to a JSON file.")
        stateStore: root.store
        stateKey: "app.uistate"
        collapsedByDefault: true

        // Export row
        Column {
            width: parent.width
            spacing: Tokens.spaceSm

            Text { text: qsTr("Export state:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm }

            Row {
                id: uiExportRow
                width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

                property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
                property string exportPathSource: ""

                Component.onCompleted: uiExportField.text = root.displayPath(exportPathSource)

                function currentPath() {
                    var t = uiExportField.text
                    if (!t.length) return ""
                    if (exportPathSource.length && t === root.displayPath(exportPathSource))
                        return root.localPath(exportPathSource)
                    return t.toLowerCase().endsWith(".json") ? t : t + ".json"
                }

                Rectangle {
                    width: parent.width - Math.round((44 + 80) * AppPalette.scale) - 2 * Tokens.spaceMd
                    height: Tokens.controlHMd; radius: Tokens.radiusMd; color: AppPalette.bg
                    border.width: 1; border.color: uiExportField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                    TextInput {
                        id: uiExportField
                        anchors.fill: parent; anchors.margins: Tokens.spaceMd
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.text; font.pixelSize: Tokens.fontSm; clip: true
                        Text { visible: !uiExportField.text.length; text: qsTr("Path..."); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                KButton {
                    width: Math.round(44 * AppPalette.scale); height: Tokens.controlHMd; text: "..."
                    onClicked: {
                        uiExportDialog.currentFolder = uiExportRow.exportFolder
                        uiExportDialog.open()
                    }
                }

                KButton {
                    width: Math.round(80 * AppPalette.scale); height: Tokens.controlHMd; text: qsTr("Export")
                    onClicked: {
                        var path = parent.currentPath()
                        if (!path.length) return
                        uiStateSerializer.exportToJsonFile(path)
                        parent.exportPathSource = path
                        uiExportField.text = root.displayPath(path)
                    }
                }

                FileDialog {
                    id: uiExportDialog
                    title: qsTr("Export UI state")
                    fileMode: FileDialog.SaveFile
                    nameFilters: ["JSON (*.json)", "All Files (*)"]
                    onAccepted: {
                        var p = uiExportDialog.parent
                        p.exportFolder = uiExportDialog.currentFolder
                        var src = root.localPath(uiExportDialog.selectedFile)
                        p.exportPathSource = src.toLowerCase().endsWith(".json") ? src : src + ".json"
                        uiExportField.text = root.displayPath(p.exportPathSource)
                    }
                }

                Settings { property alias uiStateExportFolder:     uiExportRow.exportFolder }
                Settings { property alias uiStateExportPathSource: uiExportRow.exportPathSource }
            }
        }

        // Import row
        Column {
            width: parent.width
            spacing: Tokens.spaceSm

            Text { text: qsTr("Import state:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm }

            Row {
                id: uiImportRow
                width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

                property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
                property string importPathSource: ""

                Component.onCompleted: uiImportField.text = root.displayPath(importPathSource)

                function currentPath() {
                    var t = uiImportField.text
                    if (!t.length) return ""
                    if (importPathSource.length && t === root.displayPath(importPathSource))
                        return root.localPath(importPathSource)
                    return t
                }

                Rectangle {
                    width: parent.width - Math.round((44 + 80) * AppPalette.scale) - 2 * Tokens.spaceMd
                    height: Tokens.controlHMd; radius: Tokens.radiusMd; color: AppPalette.bg
                    border.width: 1; border.color: uiImportField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                    TextInput {
                        id: uiImportField
                        anchors.fill: parent; anchors.margins: Tokens.spaceMd
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.text; font.pixelSize: Tokens.fontSm; clip: true
                        Text { visible: !uiImportField.text.length; text: qsTr("Path..."); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                KButton {
                    width: Math.round(44 * AppPalette.scale); height: Tokens.controlHMd; text: "..."
                    onClicked: {
                        uiImportDialog.currentFolder = uiImportRow.importFolder
                        uiImportDialog.open()
                    }
                }

                KButton {
                    width: Math.round(80 * AppPalette.scale); height: Tokens.controlHMd; text: qsTr("Import")
                    onClicked: {
                        var path = parent.currentPath()
                        if (!path.length) return
                        uiStateSerializer.importFromJsonFile(path)
                        parent.importPathSource = path
                        uiImportField.text = root.displayPath(path)
                    }
                }

                FileDialog {
                    id: uiImportDialog
                    title: qsTr("Import UI state")
                    fileMode: FileDialog.OpenFile
                    nameFilters: ["JSON (*.json)", "All Files (*)"]
                    onAccepted: {
                        var p = uiImportDialog.parent
                        p.importFolder = uiImportDialog.currentFolder
                        p.importPathSource = root.localPath(uiImportDialog.selectedFile)
                        uiImportField.text = root.displayPath(p.importPathSource)
                    }
                }

                Settings { property alias uiStateImportFolder:     uiImportRow.importFolder }
                Settings { property alias uiStateImportPathSource: uiImportRow.importPathSource }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: text.length > 0
            text: uiStateSerializer ? (uiStateSerializer.lastError.length ? uiStateSerializer.lastError : uiStateSerializer.lastStatus) : ""
            color: uiStateSerializer && uiStateSerializer.lastError.length ? "#FF6B6B" : AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }
    }

    // ── Workspace Layout ──────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Workspace Layout")
        description: qsTr("Pane editing, favorites and ready-made layout presets.")
        stateStore: root.store
        stateKey: "app.layoutPlacement"

        KSwitch {
            width: parent.width
            text: qsTr("Edit")
            checked: root.store.editableMode
            onToggled: { root.store.editableMode = checked }
        }

        KSwitch {
            width: parent.width
            text: qsTr("Workspace shift")
            checked: root.store.settingsPushContent
            onToggled: { root.store.settingsPushContent = checked }
        }

        KSwitch {
            width: parent.width
            text: qsTr("Global pop-up")
            checked: root.store.globalPopupEnabled
            onToggled: { root.store.globalPopupEnabled = checked }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Sidebar position:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            KTabBar {
                width: parent.width
                options: [
                    { label: qsTr("Left"),  value: "left"  },
                    { label: qsTr("Right"), value: "right" }
                ]
                currentValue: root.store.settingsSide
                onValueSelected: function(value) { root.store.settingsSide = value }
            }
        }

        KButton {
            width: parent.width
            text: qsTr("Reset workspace")
            onClicked: root.store.resetWindowConfiguration()
        }

        Row {
            spacing: Tokens.spaceLg

            KButton {
                width: Tokens.controlHLg; height: Tokens.controlHLg
                text: root.store.currentLayoutIsFavorite ? "★" : "☆"
                checkable: true
                checked: root.store.currentLayoutIsFavorite
                fontPixelSize: Tokens.fontXl
                onClicked: root.store.toggleCurrentLayoutFavorite()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.store.currentLayoutIsFavorite ? qsTr("Current layout is in favorites") : qsTr("Add current layout to favorites")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase
            }
        }

        Text { text: qsTr("Favorite layouts"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

        Text {
            visible: root.store.favoriteLayouts.length === 0
            text: qsTr("No favorite layouts yet")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
        }

        Repeater {
            model: root.store.favoriteLayouts.length
            delegate: Item {
                id: favoriteCard
                required property int index
                readonly property int favoriteIndex: index
                readonly property var favoriteEntry: (favoriteIndex >= 0 && favoriteIndex < root.store.favoriteLayouts.length) ? root.store.favoriteLayouts[favoriteIndex] : null
                readonly property var snapshot: favoriteEntry && favoriteEntry.layout ? favoriteEntry.layout : favoriteEntry
                readonly property var popupLinks: favoriteEntry && favoriteEntry.popupLinks ? favoriteEntry.popupLinks : []
                readonly property bool selected: root.store.favoriteLayoutIsCurrent(favoriteIndex)
                width: parent.width; height: favoriteCardView.implicitHeight

                FavoriteLayoutCard {
                    id: favoriteCardView
                    anchors.fill: parent
                    snapshot: favoriteCard.snapshot; popupLinks: favoriteCard.popupLinks
                    favoriteIndex: favoriteCard.favoriteIndex; selected: favoriteCard.selected; showText: true
                    onClicked: root.store.applyFavoriteLayout(favoriteCard.favoriteIndex)
                }

                CircleIconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: Tokens.spaceSm
                    width: Tokens.iconLg; height: Tokens.iconLg; iconSource: ""; glyph: "×"
                    glyphPixelSize: Tokens.iconSm; glyphColor: AppPalette.textSecond; fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover; fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover; showGlyphWithIcon: true
                    toolTipText: qsTr("Remove favorite"); z: 6
                    onClicked: root.store.removeFavoriteLayoutAt(favoriteIndex)
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

        Text { text: qsTr("Layout presets"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

        Repeater {
            model: [
                { presetId: 1, title: qsTr("Preset 1"), subtitle: qsTr("2 top panes, 1 bottom pane") },
                { presetId: 2, title: qsTr("Preset 2"), subtitle: qsTr("2 × 2 grid") },
                { presetId: 3, title: qsTr("Preset 3"), subtitle: qsTr("1 top pane, 2 bottom panes") }
            ]
            delegate: Rectangle {
                required property var modelData
                readonly property var preset: modelData
                readonly property bool hovered: cardMouse.containsMouse
                width: parent.width; height: Math.round(88 * AppPalette.scale); radius: Tokens.radiusLg
                color: hovered ? AppPalette.bg : AppPalette.card; border.width: 1
                border.color: hovered ? AppPalette.borderHover : AppPalette.border

                Row {
                    anchors.fill: parent; anchors.margins: Tokens.spaceMd; spacing: Tokens.spaceLg
                    Rectangle {
                        width: Math.round(84 * AppPalette.scale); height: Math.round(64 * AppPalette.scale); radius: Tokens.radiusMd; color: AppPalette.bgDeep
                        border.width: 1; border.color: AppPalette.border
                        Canvas {
                            anchors.fill: parent; anchors.margins: Tokens.spaceXs
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                var gap = 4, iw = width, ih = height
                                function pane(x, y, w, h) {
                                    ctx.fillStyle = "rgba(0,0,0,0)"; ctx.strokeStyle = "#64748B"
                                    ctx.lineWidth = 1; ctx.fillRect(x,y,w,h); ctx.strokeRect(x+.5,y+.5,w-1,h-1)
                                }
                                if (preset.presetId === 1) {
                                    var tH=ih*.5-gap/2, bH=ih-tH-gap, lW=iw*.5-gap/2, rW=iw-lW-gap
                                    pane(0,0,lW,tH); pane(lW+gap,0,rW,tH); pane(0,tH+gap,iw,bH)
                                } else if (preset.presetId === 2) {
                                    var lW2=iw*.5-gap/2, rW2=iw-lW2-gap, tH2=ih*.5-gap/2, bH2=ih-tH2-gap
                                    pane(0,0,lW2,tH2); pane(lW2+gap,0,rW2,tH2); pane(0,tH2+gap,lW2,bH2); pane(lW2+gap,tH2+gap,rW2,bH2)
                                } else {
                                    var tH3=ih*.5-gap/2, bH3=ih-tH3-gap, lW3=iw*.5-gap/2, rW3=iw-lW3-gap
                                    pane(0,0,iw,tH3); pane(0,tH3+gap,lW3,bH3); pane(lW3+gap,tH3+gap,rW3,bH3)
                                }
                            }
                        }
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, parent.width - Math.round(84 * AppPalette.scale) - Tokens.spaceLg); spacing: Tokens.spaceXs
                        Text { text: preset.title; color: AppPalette.text; font.pixelSize: Tokens.fontBase; font.bold: true }
                        Text { text: preset.subtitle; color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm }
                    }
                }
                MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.store.applyLayoutPreset(preset.presetId) }
            }
        }

        Text {
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("After applying a preset, choose 2D or 3D mode for each pane.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
        }
    }

    // ── Quick action menu ─────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Quick action menu")
        description: qsTr("Quick-action menu contents: favorite layouts and connected device icons.")
        stateStore: root.store
        stateKey: "app.hotkeysWindow"

        KSwitch {
            width: parent.width; text: qsTr("Show favorite layouts")
            checked: root.store.quickActionFavoritesEnabled
            onToggled: {
                root.store.quickActionFavoritesEnabled = checked
                // Skip the reveal animation if there's nothing to flash.
                if (root.store.favoriteLayouts && root.store.favoriteLayouts.length > 0)
                    root.store.requestHotkeysReveal("layouts")
            }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show connected devices")
            checked: root.store.quickActionConnectionStatusEnabled
            onToggled: {
                root.store.quickActionConnectionStatusEnabled = checked
                if (!deviceManagerWrapper || !deviceManagerWrapper.devs)
                    return
                for (var i = 0; i < deviceManagerWrapper.devs.length; ++i) {
                    var d = deviceManagerWrapper.devs[i]
                    if (d && d.devType !== 0) {
                        root.store.requestHotkeysReveal("connections")
                        break
                    }
                }
            }
        }
    }

    // ── Test (developer-only — compiled with MANUAL_TESTING) ─────────────────

    SettingsGroup {
        visible: typeof manualTesting !== "undefined" && manualTesting === true
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Test")
        description: qsTr("Developer knobs — visible only in MANUAL_TESTING builds.")
        stateStore: root.store
        stateKey: "app.test"
        collapsedByDefault: false

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Double-tap tolerance, px:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.doubleTapDistancePx) + " px"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: tapTolerSlider
                width: parent.width
                from: 1; to: 500; stepSize: 1
                value: AppPalette.doubleTapDistancePx
                onValueModified: function(v) { AppPalette.doubleTapDistancePx = v }
            }

            // Persists the chosen value across launches.
            Settings { property alias appDoubleTapDistancePx: tapTolerSlider.value }

            // ── Pane split grab thickness ────────────────────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Split grab thickness, px:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.splitHitSizePx) + " px"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: splitHitSlider
                width: parent.width
                from: 4; to: 200; stepSize: 1
                value: AppPalette.splitHitSizePx
                onValueModified: function(v) { AppPalette.splitHitSizePx = v }
            }

            Settings { property alias appSplitHitSizePx: splitHitSlider.value }

            // ── Sidebar slide animation duration ─────────────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Sidebar slide, ms:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.sidebarAnimMs) + " ms"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: sidebarAnimSlider
                width: parent.width
                from: 0; to: 5000; stepSize: 10
                value: AppPalette.sidebarAnimMs
                onValueModified: function(v) { AppPalette.sidebarAnimMs = v }
            }

            Settings { property alias appSidebarAnimMs: sidebarAnimSlider.value }

            // ── Workspace rubber-band adjustment duration ────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Workspace adjust, ms:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.workspaceAnimMs) + " ms"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: workspaceAnimSlider
                width: parent.width
                from: 0; to: 5000; stepSize: 10
                value: AppPalette.workspaceAnimMs
                onValueModified: function(v) { AppPalette.workspaceAnimMs = v }
            }

            Settings { property alias appWorkspaceAnimMs: workspaceAnimSlider.value }

            Component.onCompleted: {
                AppPalette.doubleTapDistancePx = tapTolerSlider.value
                AppPalette.splitHitSizePx = splitHitSlider.value
                AppPalette.sidebarAnimMs = sidebarAnimSlider.value
                AppPalette.workspaceAnimMs = workspaceAnimSlider.value
            }
        }
    }
}

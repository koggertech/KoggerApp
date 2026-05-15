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
    spacing: 10

    // Small inline checkbox for parameter rows
    component SmallCheck: Item {
        id: sc
        property bool checked: false
        signal toggled(bool val)
        width: 18
        height: 18

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: sc.checked ? AppPalette.accentBg : AppPalette.bg
            border.width: 1
            border.color: sc.checked ? AppPalette.accentBorder : AppPalette.borderHover

            Text {
                anchors.centerIn: parent
                text: "✓"
                color: AppPalette.accentBorder
                font.pixelSize: 11
                font.bold: true
                visible: sc.checked
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

    // ── Предпочтения ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Preferences")
        stateStore: root.store
        stateKey: "app.preference"
        collapsedByDefault: false

        Column {
            width: parent.width
            spacing: 8

            Text { text: qsTr("Language:"); color: AppPalette.textSecond; font.pixelSize: 14 }

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
            spacing: 8

            Text { text: qsTr("Theme:"); color: AppPalette.textSecond; font.pixelSize: 14 }

            Item {
                id: appThemeHolder
                width: parent.width
                property int selectedIndex: 0

                readonly property var names: ["Dark","S.Dark","Light","S.Light","OneDark","Monokai","Kimbie","Solar","Desert","Steam 2003"]
                readonly property int cols: 5
                readonly property int itemH: 28
                readonly property real itemW: (width - (cols - 1) * 4) / cols
                height: 2 * itemH + 4

                onSelectedIndexChanged: if (theme) theme.themeID = selectedIndex
                Component.onCompleted: if (theme) theme.themeID = selectedIndex

                Repeater {
                    model: 10
                    delegate: Rectangle {
                        required property int index
                        readonly property bool sel: index === appThemeHolder.selectedIndex
                        x: (index % 5) * (appThemeHolder.itemW + 4)
                        y: Math.floor(index / 5) * (appThemeHolder.itemH + 4)
                        width: appThemeHolder.itemW
                        height: appThemeHolder.itemH
                        radius: 6
                        color: sel ? AppPalette.accentBg : AppPalette.bg
                        border.width: 1
                        border.color: sel ? AppPalette.accentBorder : AppPalette.border

                        Text {
                            anchors.centerIn: parent
                            text: appThemeHolder.names[index]
                            color: AppPalette.text
                            font.pixelSize: 11
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
            spacing: 8

            Text { text: qsTr("Feature level:"); color: AppPalette.textSecond; font.pixelSize: 14 }

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
    }

    // ── Интерфейс ─────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Interface")
        stateStore: root.store
        stateKey: "app.interface"
        collapsedByDefault: false

        KSwitch {
            id: consoleVisible
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
        }
    }

    // ── График ────────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Chart")
        stateStore: root.store
        stateKey: "app.plot"
        collapsedByDefault: true

        Row {
            width: parent.width
            height: 30
            spacing: 8

            Text {
                text: qsTr("Chart count:")
                color: AppPalette.textSecond
                font.pixelSize: 13
                width: parent.width - numPlotsSpinBox.implicitWidth - 8
                anchors.verticalCenter: parent.verticalCenter
            }

            KSpinBox {
                id: numPlotsSpinBox
                from: 1; to: 2; stepSize: 1; value: 1
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Settings { property alias numPlotsSpinBox: numPlotsSpinBox.value }

        KSwitch {
            id: plotSyncCheckBox
            width: parent.width
            text: qsTr("Synchronization")
            visible: numPlotsSpinBox.value >= 2
        }

        Settings { property alias plotSyncCheckBox: plotSyncCheckBox.checked }
    }

    // ── Датасет ───────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Dataset")
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
        Row {
            width: parent.width
            height: 30
            spacing: 8

            SmallCheck {
                id: fixBlackStripesCheckButton
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { core.setFixBlackStripesState(v) }
            }

            Text {
                text: qsTr("FBS forward / backward:")
                color: AppPalette.textSecond
                font.pixelSize: 13
                width: parent.width - 18 - 93 - 93 - 32
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
            }

            KSpinBox {
                id: fixBlackStripesForwardStepsSpinBox
                width: 93; from: 0; to: 100; stepSize: 1; value: 5
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { core.setFixBlackStripesForwardSteps(v) }
            }

            KSpinBox {
                id: fixBlackStripesBackwardStepsSpinBox
                width: 93; from: 0; to: 100; stepSize: 1; value: 5
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { core.setFixBlackStripesBackwardSteps(v) }
            }
        }

        Settings { property alias fixBlackStripesCheckButton: fixBlackStripesCheckButton.checked }
        Settings { property alias fixBlackStripesForwardStepsSpinBox: fixBlackStripesForwardStepsSpinBox.value }
        Settings { property alias fixBlackStripesBackwardStepsSpinBox: fixBlackStripesBackwardStepsSpinBox.value }

        // Sonar offset row
        Row {
            width: parent.width
            height: 30
            spacing: 8

            SmallCheck {
                id: sonarOffsetCheckButton
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) {
                    if (v) dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, sonarOffsetValueY.value * 0.001, 0)
                    else   dataset.setSonarOffset(0, 0, 0)
                    core.setIsAttitudeExpected(v)
                }
            }

            Text {
                text: qsTr("S.offset XY, mm:")
                color: AppPalette.textSecond
                font.pixelSize: 13
                width: parent.width - 18 - 93 - 93 - 32
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
            }

            KSpinBox {
                id: sonarOffsetValueX
                width: 93; from: -9999; to: 9999; stepSize: 50; value: 0
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) {
                    if (sonarOffsetCheckButton.checked)
                        dataset.setSonarOffset(v * 0.001, sonarOffsetValueY.value * 0.001, 0)
                }
            }

            KSpinBox {
                id: sonarOffsetValueY
                width: 93; from: -9999; to: 9999; stepSize: 50; value: 0
                anchors.verticalCenter: parent.verticalCenter
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
        stateStore: root.store
        stateKey: "app.bottomtrack"
        collapsedByDefault: false

        readonly property int spinW: 115
        readonly property int labelW: parent.width - 18 - spinW - 16

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
            spacing: 8

            Text { text: qsTr("Preset:"); color: AppPalette.textSecond; font.pixelSize: 14 }

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
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackGainSlope; checked: true
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setGainSlope(bottomTrackGainSlopeValue.value / 100) }
            }
            Text {
                text: qsTr("Gain slope:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackGainSlopeValue
                from: 0; to: 300; stepSize: 10; value: 100; divisor: 100; decimals: 2
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackGainSlope.checked && root.targetPlot) root.targetPlot.setGainSlope(v / 100) }
            }
        }
        Settings { property alias bottomTrackGainSlope: bottomTrackGainSlope.checked }
        Settings { property alias bottomTrackGainSlopeValue: bottomTrackGainSlopeValue.value }

        // Threshold
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackThreshold
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setThreshold(bottomTrackThresholdValue.value / 100) }
            }
            Text {
                text: qsTr("Threshold:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackThresholdValue
                from: 0; to: 200; stepSize: 5; value: 0; divisor: 100; decimals: 2
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackThreshold.checked && root.targetPlot) root.targetPlot.setThreshold(v / 100) }
            }
        }
        Settings { property alias bottomTrackThreshold: bottomTrackThreshold.checked }
        Settings { property alias bottomTrackThresholdValue: bottomTrackThresholdValue.value }

        // Horizontal window
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackWindow
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setWindowSize(bottomTrackWindowValue.value) }
            }
            Text {
                text: qsTr("Horizontal window:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackWindowValue
                from: 1; to: 100; stepSize: 2; value: 1
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackWindow.checked && root.targetPlot) root.targetPlot.setWindowSize(v) }
            }
        }
        Settings { property alias bottomTrackWindow: bottomTrackWindow.checked }
        Settings { property alias bottomTrackWindowValue: bottomTrackWindowValue.value }

        // Vertical gap
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackVerticalGap
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01) }
            }
            Text {
                text: qsTr("Vertical gap, %:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackVerticalGapValue
                from: 0; to: 100; stepSize: 2; value: 10
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackVerticalGap.checked && root.targetPlot) root.targetPlot.setVerticalGap(v * 0.01) }
            }
        }
        Settings { property alias bottomTrackVerticalGap: bottomTrackVerticalGap.checked }
        Settings { property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value }

        // Min range
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackMinRange
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMin(bottomTrackMinRangeValue.value / 1000) }
            }
            Text {
                text: qsTr("Min range, m:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackMinRangeValue
                from: 0; to: 200000; stepSize: 10; value: 0; divisor: 1000; decimals: 2
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackMinRange.checked && root.targetPlot) root.targetPlot.setRangeMin(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMinRange: bottomTrackMinRange.checked }
        Settings { property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value }

        // Max range
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackMaxRange
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMax(bottomTrackMaxRangeValue.value / 1000) }
            }
            Text {
                text: qsTr("Max range, m:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: btGroup.labelW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            KSpinBox {
                id: bottomTrackMaxRangeValue
                from: 0; to: 200000; stepSize: 1000; value: 100000; divisor: 1000; decimals: 2
                anchors.verticalCenter: parent.verticalCenter
                onValueModified: function(v) { if (bottomTrackMaxRange.checked && root.targetPlot) root.targetPlot.setRangeMax(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMaxRange: bottomTrackMaxRange.checked }
        Settings { property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value }

        // Sensor offset (label row + values row)
        Row {
            width: parent.width; height: 30; spacing: 8
            SmallCheck {
                id: bottomTrackSensorOffset
                anchors.verticalCenter: parent.verticalCenter
                onToggled: function(v) {
                    if (v && root.targetPlot) {
                        root.targetPlot.setOffsetX(btOffX.value *  0.001)
                        root.targetPlot.setOffsetY(btOffY.value *  0.001)
                        root.targetPlot.setOffsetZ(btOffZ.value *  0.001)
                    }
                }
            }
            Text {
                text: qsTr("Sonar offset XYZ, mm:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: parent.width - 18 - 8; height: 30
                verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
            }
        }
        Row {
            visible: bottomTrackSensorOffset.checked
            width: parent.width; height: 30; spacing: 4
            readonly property real sw: (width - 2 * 4) / 3

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
            width: parent.width; spacing: 8
            readonly property real bw: (width - 8) / 2

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
        stateStore: root.store
        stateKey: "app.tgc"
        collapsedByDefault: true

        readonly property int valueLabelW: 60
        readonly property int labelW: 92

        Component.onCompleted: {
            core.setTgcGainNear(tgcGainNearSlider.value * 0.01)
            core.setTgcGainFar(tgcGainFarSlider.value * 0.01)
            core.setTgcCompensate(tgcCompensateSwitch.checked)
        }

        // Near gain
        Row {
            width: parent.width; height: 30; spacing: 8

            Text {
                text: qsTr("Near gain:")
                color: AppPalette.textSecond; font.pixelSize: 13
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
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 16
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
                color: AppPalette.text; font.pixelSize: 13
            }
        }

        Settings { property alias appTgcGainNear: tgcGainNearSlider.value }

        // Far gain
        Row {
            width: parent.width; height: 30; spacing: 8

            Text {
                text: qsTr("Far gain:")
                color: AppPalette.textSecond; font.pixelSize: 13
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
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 16
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
                color: AppPalette.text; font.pixelSize: 13
            }
        }

        Settings { property alias appTgcGainFar: tgcGainFarSlider.value }

        // Curve preview
        Canvas {
            id: tgcCurveCanvas
            width: parent.width
            height: 100

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

    // ── Mosaic ───────────────────────────────────────────────────────────────

    SettingsGroup {
        id: mosaicGroup
        property bool fakeCoordsActive: core ? core.posZeroing : false
        visible: fakeCoordsActive
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Mosaic")
        stateStore: root.store
        stateKey: "app.mosaic"
        collapsedByDefault: true

        Component.onCompleted: fakeCoordsActive = core ? core.posZeroing : false

        Connections {
            target: core
            ignoreUnknownSignals: true
            function onPosZeroingChanged() {
                mosaicGroup.fakeCoordsActive = core.posZeroing
            }
        }

        readonly property int valueLabelW: 60
        readonly property int labelW: 120

        // Calc last N epochs (only meaningful with fake coords / pos zeroing)
        Row {
            width: parent.width; height: 30; spacing: 8

            Text {
                text: qsTr("Calc last N epochs:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: mosaicGroup.labelW
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
            }

            KSlider {
                id: fakeCoordsLastNSlider
                width: parent.width - mosaicGroup.labelW - mosaicGroup.valueLabelW - 16
                anchors.verticalCenter: parent.verticalCenter
                from: 10; to: 3000; stepSize: 10; value: 500
                // Crank slider all the way right → no limit (process every epoch).
                readonly property int effectiveN: value >= to ? 0 : value
                toolTipText: effectiveN === 0 ? qsTr("All") : String(effectiveN)

                onEffectiveNChanged: core.setMosaicFakeCoordsLastN(effectiveN)
                Component.onCompleted: core.setMosaicFakeCoordsLastN(effectiveN)
            }

            Text {
                width: mosaicGroup.valueLabelW
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                text: fakeCoordsLastNSlider.value >= fakeCoordsLastNSlider.to
                      ? qsTr("All") : fakeCoordsLastNSlider.value
                color: AppPalette.text; font.pixelSize: 13
            }
        }

        Settings { property alias appMosaicFakeCoordsLastN: fakeCoordsLastNSlider.value }
    }

    // ── Экспорт ───────────────────────────────────────────────────────────────

    SettingsGroup {
        id: exportGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Export")
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
            width: parent.width; height: 30; spacing: 8

            Rectangle {
                width: parent.width - 44 - 8
                height: 30
                radius: 6
                color: AppPalette.bg
                border.width: 1
                border.color: exportPathField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: exportPathField
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: 12
                    clip: true

                    Text {
                        visible: !exportPathField.text.length
                        text: qsTr("Export path...")
                        color: AppPalette.textMuted
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            KButton {
                width: 44; height: 30; text: "..."
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
        Row {
            width: parent.width; height: 30; spacing: 8

            SmallCheck {
                id: exportDecimation
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("Decimation, m:")
                color: AppPalette.textSecond; font.pixelSize: 13
                width: parent.width - 18 - 93 - 93 - 32
                anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }

            KSpinBox {
                id: exportDecimationValue
                width: 93; from: 0; to: 100; stepSize: 1; value: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            KButton {
                width: 93; height: 30; text: qsTr("CSV")
                anchors.verticalCenter: parent.verticalCenter
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
        stateStore: root.store
        stateKey: "app.uistate"
        collapsedByDefault: true

        // Export row
        Column {
            width: parent.width
            spacing: 6

            Text { text: qsTr("Export state:"); color: AppPalette.textMuted; font.pixelSize: 12 }

            Row {
                id: uiExportRow
                width: parent.width; height: 30; spacing: 8

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
                    width: parent.width - 44 - 80 - 16
                    height: 30; radius: 6; color: AppPalette.bg
                    border.width: 1; border.color: uiExportField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                    TextInput {
                        id: uiExportField
                        anchors.fill: parent; anchors.margins: 8
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.text; font.pixelSize: 12; clip: true
                        Text { visible: !uiExportField.text.length; text: qsTr("Path..."); color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                KButton {
                    width: 44; height: 30; text: "..."
                    onClicked: {
                        uiExportDialog.currentFolder = uiExportRow.exportFolder
                        uiExportDialog.open()
                    }
                }

                KButton {
                    width: 80; height: 30; text: qsTr("Export")
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
            spacing: 6

            Text { text: qsTr("Import state:"); color: AppPalette.textMuted; font.pixelSize: 12 }

            Row {
                id: uiImportRow
                width: parent.width; height: 30; spacing: 8

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
                    width: parent.width - 44 - 80 - 16
                    height: 30; radius: 6; color: AppPalette.bg
                    border.width: 1; border.color: uiImportField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                    TextInput {
                        id: uiImportField
                        anchors.fill: parent; anchors.margins: 8
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.text; font.pixelSize: 12; clip: true
                        Text { visible: !uiImportField.text.length; text: qsTr("Path..."); color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                KButton {
                    width: 44; height: 30; text: "..."
                    onClicked: {
                        uiImportDialog.currentFolder = uiImportRow.importFolder
                        uiImportDialog.open()
                    }
                }

                KButton {
                    width: 80; height: 30; text: qsTr("Import")
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
            font.pixelSize: 12
        }
    }

    // ── Workspace Layout ──────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Workspace Layout")
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
            spacing: 8

            Text { text: qsTr("Sidebar position:"); color: AppPalette.textSecond; font.pixelSize: 14 }

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
            spacing: 10

            KButton {
                width: 36; height: 36
                text: root.store.currentLayoutIsFavorite ? "★" : "☆"
                checkable: true
                checked: root.store.currentLayoutIsFavorite
                fontPixelSize: 20
                onClicked: root.store.toggleCurrentLayoutFavorite()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.store.currentLayoutIsFavorite ? qsTr("Current layout is in favorites") : qsTr("Add current layout to favorites")
                color: AppPalette.textSecond; font.pixelSize: 14
            }
        }

        Text { text: qsTr("Favorite layouts"); color: AppPalette.text; font.pixelSize: 15; font.bold: true }

        Text {
            visible: root.store.favoriteLayouts.length === 0
            text: qsTr("No favorite layouts yet")
            color: AppPalette.textMuted; font.pixelSize: 12
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
                    anchors.top: parent.top; anchors.right: parent.right
                    anchors.topMargin: 6; anchors.rightMargin: 6
                    width: 24; height: 24; iconSource: ""; glyph: "×"
                    glyphPixelSize: 16; glyphColor: AppPalette.textSecond; fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover; fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover; showGlyphWithIcon: true
                    toolTipText: qsTr("Remove favorite"); z: 6
                    onClicked: root.store.removeFavoriteLayoutAt(favoriteIndex)
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

        Text { text: qsTr("Layout presets"); color: AppPalette.text; font.pixelSize: 15; font.bold: true }

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
                width: parent.width; height: 88; radius: 8
                color: hovered ? AppPalette.bg : AppPalette.card; border.width: 1
                border.color: hovered ? AppPalette.borderHover : AppPalette.border

                Row {
                    anchors.fill: parent; anchors.margins: 8; spacing: 10
                    Rectangle {
                        width: 84; height: 64; radius: 6; color: AppPalette.bgDeep
                        border.width: 1; border.color: AppPalette.border
                        Canvas {
                            anchors.fill: parent; anchors.margins: 4
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
                        width: Math.max(0, parent.width - 84 - 10); spacing: 4
                        Text { text: preset.title; color: AppPalette.text; font.pixelSize: 14; font.bold: true }
                        Text { text: preset.subtitle; color: AppPalette.textMuted; font.pixelSize: 12 }
                    }
                }
                MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.store.applyLayoutPreset(preset.presetId) }
            }
        }

        Text {
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("After applying a preset, choose 2D or 3D mode for each pane.")
            color: AppPalette.textMuted; font.pixelSize: 12
        }
    }

    // ── Hotkeys Window ────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Hotkeys Window")
        stateStore: root.store
        stateKey: "app.hotkeysWindow"

        KSwitch {
            width: parent.width; text: qsTr("Show favorite layouts")
            checked: root.store.quickActionFavoritesEnabled
            highlighted: root.store.hotkeysRevealKey === "layouts"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: { root.store.quickActionFavoritesEnabled = checked; root.store.requestHotkeysReveal("layouts") }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show marker tool")
            checked: root.store.quickActionMarkerEnabled
            highlighted: root.store.hotkeysRevealKey === "marker"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: { root.store.quickActionMarkerEnabled = checked; root.store.requestHotkeysReveal("marker") }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show connection status")
            checked: root.store.quickActionConnectionStatusEnabled
            highlighted: root.store.hotkeysRevealKey === "connections"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: { root.store.quickActionConnectionStatusEnabled = checked; root.store.requestHotkeysReveal("connections") }
        }
    }
}

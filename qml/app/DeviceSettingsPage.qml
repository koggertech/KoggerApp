import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0

Column {
    id: root

    property var dev: null
    property var store: null

    readonly property real groupWidth: Math.max(0, width)
    readonly property real spinW: Math.round(115 * AppPalette.scale)
    // Spinbox label width — account for SettingsGroup's content card padding
    // (Tokens.spaceMd on each side) plus row spacing + small safety margin.
    readonly property real lblW: Math.max(0, groupWidth - 2 * Tokens.spaceMd - spinW - Tokens.spaceMd - Tokens.spaceSm)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    // KSpinBox wrapper that syncs with a C++ device property
    component DevSpin: Item {
        id: ds
        property int devValue: 0
        property int from: 0
        property int to: 100
        property int stepSize: 1
        property real divisor: 1.0
        property int decimals: 0
        property var writeBack: null  // function(v) called on user interaction

        implicitWidth: Math.round(115 * AppPalette.scale); implicitHeight: Tokens.controlHMd
        property bool _in: false

        onDevValueChanged: {
            if (spin.value !== devValue) {
                _in = true; spin.value = devValue; _in = false
            }
        }
        Component.onCompleted: { _in = true; spin.value = devValue; _in = false }

        KSpinBox {
            id: spin
            anchors.fill: parent
            from: ds.from; to: ds.to; stepSize: ds.stepSize
            divisor: ds.divisor; decimals: ds.decimals
            onValueModified: function(v) { if (!ds._in && ds.writeBack) ds.writeBack(v) }
        }
    }

    // Section heading above the per-area device settings groups.
    Text {
        text: qsTr("Settings:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    // ── Эхограмма ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Echogram"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.echogram"; collapsedByDefault: true
        visible: !!(dev && dev.isChartSupport)
        confirmed: !(dev && dev.chartSetupState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Resolution, mm:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 10; to: 100; stepSize: 10; devValue: dev ? (dev.chartResolution || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartResolution = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Sample count:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 100; to: 15000; stepSize: 100; devValue: dev ? (dev.chartSamples || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartSamples = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Offset:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 10000; stepSize: 100; devValue: dev ? (dev.chartOffset || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartOffset = v } }
        }
    }

    // ── Дальномер ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Rangefinder"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.rangefinder"; collapsedByDefault: true
        visible: !!(dev && dev.isDistSupport)
        confirmed: !(dev && dev.distSetupState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Max distance, mm:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 1000; devValue: dev ? (dev.distMax || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distMax = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Dead zone, mm:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 100; devValue: dev ? (dev.distDeadZone || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distDeadZone = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Confidence threshold, %:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 100; stepSize: 1; devValue: dev ? (dev.distConfidence || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distConfidence = v } }
        }
    }

    // ── Преобразователь ───────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Transducer"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.transducer"; collapsedByDefault: true
        visible: !!(dev && dev.isTransducerSupport)
        confirmed: !(dev && dev.transcState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Pulse count:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 5000; stepSize: 1; devValue: dev ? (dev.transPulse || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.transPulse = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Frequency, kHz:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 40; to: 6000; stepSize: 5; devValue: dev ? (dev.transFreq || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.transFreq = v } }
        }

        KSwitch {
            id: boosterSwitch
            width: parent.width; text: qsTr("Booster")
            property bool wantChecked: !!(dev && dev.transBoost === 1)
            property bool _g: false
            onWantCheckedChanged: { if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false } }
            Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
            onToggled: { if (!_g && dev) dev.transBoost = checked ? 1 : 0 }
        }
    }

    // ── DSP ───────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("DSP"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.dsp"; collapsedByDefault: true
        visible: !!(dev && dev.isDSPSupport)
        confirmed: !(dev && (dev.dspState === false || dev.soundState === false))

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Horizontal smoothing:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 4; stepSize: 1; devValue: dev ? (dev.dspHorSmooth || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.dspHorSmooth = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Sound speed, m/s:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 300; to: 6000; stepSize: 5; devValue: dev ? Math.round((dev.soundSpeed || 0) / 1000) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.soundSpeed = v * 1000 } }
        }
    }

    // ── Датасет ───────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Dataset"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.dataset"; collapsedByDefault: true
        visible: !!(dev && dev.isDatasetSupport)
        confirmed: !(dev && dev.datasetState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Period, ms:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 2000; stepSize: 50; devValue: dev ? (dev.ch1Period || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.ch1Period = v } }
        }

        Column {
            width: parent.width; spacing: Tokens.spaceSm
            Text { text: qsTr("Echogram:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd }
            KTabBar {
                id: datasetChartTab; width: parent.width
                options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("8-bit"), value: 1 }]
                property int chartModel: dev ? (dev.datasetChart === 1 ? 1 : 0) : 0
                property bool _g: false
                onChartModelChanged: { if (currentValue !== chartModel) { _g = true; currentValue = chartModel; _g = false } }
                Component.onCompleted: { _g = true; currentValue = chartModel; _g = false }
                onValueSelected: function(v) { if (!_g && dev) dev.datasetChart = v }
            }
        }

        Column {
            width: parent.width; spacing: Tokens.spaceSm
            Text { text: qsTr("Rangefinder:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd }
            KTabBar {
                id: datasetDistTab; width: parent.width
                options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("On"), value: 1 }, { label: qsTr("NMEA"), value: 2 }]
                property int distModel: dev ? (dev.datasetDist === 1 ? 1 : (dev.datasetSDDBT === 1 ? 2 : 0)) : 0
                property bool _g: false
                onDistModelChanged: { if (currentValue !== distModel) { _g = true; currentValue = distModel; _g = false } }
                Component.onCompleted: { _g = true; currentValue = distModel; _g = false }
                onValueSelected: function(v) {
                    if (_g || !dev) return
                    if (v === 1)      { dev.datasetDist = 1 }
                    else if (v === 2) { dev.datasetSDDBT = 1 }
                    else              { dev.datasetDist = 0; dev.datasetSDDBT = 0 }
                }
            }
        }

        KSwitch {
            id: ahrsSwitch; width: parent.width; text: qsTr("AHRS")
            property bool wantChecked: !!(dev && (dev.datasetEuler & 1))
            property bool _g: false
            onWantCheckedChanged: { if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false } }
            Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
            onToggled: { if (!_g && dev) dev.datasetEuler = checked ? 1 : 0 }
        }

        KSwitch {
            id: tempSwitch; width: parent.width; text: qsTr("Temperature")
            property bool wantChecked: !!(dev && (dev.datasetTemp & 1))
            property bool _g: false
            onWantCheckedChanged: { if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false } }
            Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
            onToggled: { if (!_g && dev) dev.datasetTemp = checked ? 1 : 0 }
        }

        KSwitch {
            id: tsSwitch; width: parent.width; text: qsTr("Timestamp")
            property bool wantChecked: !!(dev && (dev.datasetTimestamp & 1))
            property bool _g: false
            onWantCheckedChanged: { if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false } }
            Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
            onToggled: { if (!_g && dev) dev.datasetTimestamp = checked ? 1 : 0 }
        }
    }

    SettingsGroup {
        id: devActionsGroup
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Actions"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.actions"; collapsedByDefault: true
        confirmed: !(dev && dev.uartState === false)

        readonly property var baudrateOptions: [9600, 19200, 38400, 57600, 115200,
                                                230400, 460800, 921600, 1200000, 2000000]

        Row {
            width: parent.width; spacing: Tokens.spaceSm
            readonly property real bw: (width - 2 * Tokens.spaceSm) / 3
            KButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Flash settings")
                onClicked: { if (dev) { dev.flashSettings(); notifications.info(qsTr("Settings written to device: %1").arg(dev.devName)) } }
            }
            KButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Erase settings"); danger: true
                onClicked: { if (dev) { dev.resetSettings(); notifications.info(qsTr("Settings erased on device: %1").arg(dev.devName)) } }
            }
            KButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Reboot")
                onClicked: { if (dev) { dev.reboot(); notifications.info(qsTr("Reboot command sent: %1").arg(dev.devName)) } }
            }
        }
        Row {
            width: parent.width; spacing: Tokens.spaceSm
            readonly property real setW: Math.round(120 * AppPalette.scale)
            KCombo {
                id: baudrateCombo
                width: parent.width - parent.setW - Tokens.spaceSm
                height: Tokens.controlHMd
                model: devActionsGroup.baudrateOptions
                currentIndex: devActionsGroup.baudrateOptions.indexOf(115200)
            }
            KButton {
                width: parent.setW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Set baudrate")
                onClicked: {
                    if (dev) {
                        var b = devActionsGroup.baudrateOptions[baudrateCombo.currentIndex]
                        dev.baudrate = b
                        notifications.info(qsTr("Baudrate set: %1").arg(b))
                    }
                }
            }
        }
    }

    SettingsGroup {
        id: devSettingsGroup
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Settings"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.settingsFile"; collapsedByDefault: true

        property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        Settings {
            property alias devImportFolder: devSettingsGroup.importFolder
            property alias devExportFolder: devSettingsGroup.exportFolder
        }

        function _localPath(u) {
            if (!u) return ""
            if (typeof u !== "string" && u.toLocalFile) {
                var lp = u.toLocalFile()
                if (lp && lp.length) return lp
            }
            var s = String(u)
            if (s.indexOf("file:///") === 0)
                return Qt.platform.os === "windows" ? s.slice(8) : s.slice(7)
            if (s.indexOf("file://") === 0)
                return s.slice(7)
            return s
        }

        FileDialog {
            id: importXmlDialog
            title: qsTr("Open settings file")
            fileMode: FileDialog.OpenFile
            nameFilters: ["XML files (*.xml)"]
            onCurrentFolderChanged: devSettingsGroup.importFolder = currentFolder
            onAccepted: {
                devSettingsGroup.importFolder = importXmlDialog.currentFolder
                var lp = devSettingsGroup._localPath(importXmlDialog.selectedFile)
                if (dev && lp.length) dev.importSettingsFromXML(lp)
            }
        }

        FileDialog {
            id: exportXmlDialog
            title: qsTr("Save settings file")
            fileMode: FileDialog.SaveFile
            nameFilters: ["XML files (*.xml)"]
            defaultSuffix: "xml"
            onCurrentFolderChanged: devSettingsGroup.exportFolder = currentFolder
            onAccepted: {
                devSettingsGroup.exportFolder = exportXmlDialog.currentFolder
                var lp = devSettingsGroup._localPath(exportXmlDialog.selectedFile)
                if (dev && lp.length) dev.exportSettingsToXML(lp)
            }
        }

        Row {
            width: parent.width; spacing: Tokens.spaceSm
            readonly property real bw: (width - Tokens.spaceSm) / 2
            KButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Import XML")
                onClicked: { importXmlDialog.currentFolder = devSettingsGroup.importFolder; importXmlDialog.open() }
            }
            KButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Export XML")
                onClicked: { exportXmlDialog.currentFolder = devSettingsGroup.exportFolder; exportXmlDialog.open() }
            }
        }
    }

    SettingsGroup {
        id: devUpgradeGroup
        visible: !!(dev && dev.isUpgradeSupport)
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Upgrade"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.upgrade"; collapsedByDefault: true

        property var upgradeFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property string selectedUpgradePathSource: ""

        Settings { property alias devUpgradeFolder: devUpgradeGroup.upgradeFolder }

        function _src(value) {
            if (!value) return ""
            if (typeof value === "string") {
                if (value.startsWith("file:///")) return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
                if (value.startsWith("file://")) return value.slice(7)
                return value
            }
            var lp = value.toLocalFile ? value.toLocalFile() : ""
            return lp && lp.length ? lp : value.toString()
        }
        function _disp(value) {
            var s = devUpgradeGroup._src(value)
            if (!s.length) return ""
            try { return decodeURIComponent(s) } catch (e) { return s }
        }
        function currentUpgradePath() {
            var d = upgradePathInput.text
            if (!d || !d.length) return ""
            if (devUpgradeGroup.selectedUpgradePathSource
                    && d === devUpgradeGroup._disp(devUpgradeGroup.selectedUpgradePathSource))
                return devUpgradeGroup.selectedUpgradePathSource
            return d
        }
        function setUpgradePath(path) {
            devUpgradeGroup.selectedUpgradePathSource = devUpgradeGroup._src(path)
            upgradePathInput.text = devUpgradeGroup._disp(devUpgradeGroup.selectedUpgradePathSource)
        }

        readonly property int _fwOk: 101
        property string _activeTag: ""
        property string _activeLabel: ""
        property string _activeFw: ""
        function _devLabel() {
            if (!dev) return ""
            var n = dev.devName ? dev.devName : ""
            return dev.devSN ? (n + " (SN " + dev.devSN + ")") : n
        }
        function _baseName(p) {
            if (!p) return ""
            var s = String(p).replace(/\\/g, "/")
            var i = s.lastIndexOf("/")
            return i >= 0 ? s.slice(i + 1) : s
        }

        FileDialog {
            id: upgradeFileDialog
            title: qsTr("Please choose a file")
            currentFolder: devUpgradeGroup.upgradeFolder
            nameFilters: ["Upgrade files (*.ufw)"]
            onCurrentFolderChanged: devUpgradeGroup.upgradeFolder = currentFolder
            onAccepted: {
                devUpgradeGroup.upgradeFolder = upgradeFileDialog.currentFolder
                devUpgradeGroup.setUpgradePath(upgradeFileDialog.selectedFile)
            }
        }

        // Прогресс прошивки (0..100).
        Rectangle {
            width: parent.width; height: Math.round(4 * AppPalette.scale); radius: height / 2
            color: AppPalette.trackOff
            readonly property int pct: dev && dev.upgradeFWStatus !== undefined
                                       ? Math.max(0, Math.min(100, dev.upgradeFWStatus)) : 0
            visible: pct > 0 && pct < 100
            Rectangle {
                height: parent.height; radius: parent.radius; color: AppPalette.accentBar
                width: parent.width * parent.pct / 100
            }
        }

        Row {
            width: parent.width; spacing: Tokens.spaceSm
            readonly property real browseW: Math.round(44 * AppPalette.scale)
            Rectangle {
                width: parent.width - parent.browseW - Tokens.spaceSm
                height: Tokens.controlHMd; radius: Tokens.radiusMd
                color: AppPalette.bg; border.width: 1
                border.color: upgradePathInput.activeFocus ? AppPalette.accentBorder : AppPalette.border
                TextInput {
                    id: upgradePathInput
                    activeFocusOnTab: true
                    anchors.fill: parent; anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text; font.pixelSize: Tokens.fontSm; clip: true
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: upgradePathInput.selectAll() }
                    Text {
                        visible: !upgradePathInput.text.length; text: qsTr("Enter path")
                        color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            KButton {
                width: parent.browseW; height: Tokens.controlHMd; text: "..."; fontPixelSize: Tokens.fontMd
                onClicked: { upgradeFileDialog.currentFolder = devUpgradeGroup.upgradeFolder; upgradeFileDialog.open() }
            }
        }
        KButton {
            width: parent.width; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
            text: qsTr("UPGRADE")
            visible: upgradePathInput.text !== ""
            onClicked: {
                if (!dev) return
                var path = devUpgradeGroup.currentUpgradePath()
                var fw = devUpgradeGroup._baseName(path)
                var label = devUpgradeGroup._devLabel()
                var tag = "fw-upgrade-" + (dev.devSN || 0)
                if (core.upgradeFW(path, dev)) {
                    devUpgradeGroup._activeTag = tag
                    devUpgradeGroup._activeLabel = label
                    devUpgradeGroup._activeFw = fw
                    notifications.warning(qsTr("Flashing device %1 with file %2").arg(label).arg(fw), tag)
                } else {
                    notifications.warning(qsTr("Failed to open firmware file: %1").arg(fw))
                }
            }
        }

        Connections {
            target: dev
            ignoreUnknownSignals: true
            function onUpgradingFirmwareDone() {
                if (!devUpgradeGroup._activeTag.length) return
                notifications.dismiss(devUpgradeGroup._activeTag)
                if (dev && dev.upgradeFWStatus === devUpgradeGroup._fwOk)
                    notifications.info(qsTr("Device %1 successfully flashed with file %2")
                                       .arg(devUpgradeGroup._activeLabel).arg(devUpgradeGroup._activeFw))
                else
                    notifications.warning(qsTr("Failed to flash device %1 with file %2 (error code %3)")
                                          .arg(devUpgradeGroup._activeLabel).arg(devUpgradeGroup._activeFw)
                                          .arg(dev ? dev.upgradeFWStatus : -1))
                devUpgradeGroup._activeTag = ""
            }
        }
    }
}

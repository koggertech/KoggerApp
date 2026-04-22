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
    readonly property real spinW: 115
    readonly property real lblW: groupWidth - spinW - 8

    width: parent ? parent.width : implicitWidth
    spacing: 10

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

        implicitWidth: 115; implicitHeight: 30
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

    // ── Device header ──────────────────────────────────────────────────────

    Rectangle {
        width: root.groupWidth; height: 34; radius: 8
        color: AppPalette.bg; border.width: 1; border.color: AppPalette.border

        Row {
            anchors.fill: parent; anchors.margins: 10; spacing: 8

            Rectangle {
                width: 8; height: 8; radius: 4; color: "#10B981"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: dev ? (dev.devName + "  " + dev.fwVersion + "  [SN: " + dev.devSN + "]") : ""
                color: AppPalette.textSecond; font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight; width: parent.width - 20
            }
        }
    }

    // ── Эхограмма ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Echogram"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.echogram"; collapsedByDefault: false

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Resolution, mm:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 10; to: 100; stepSize: 10; devValue: dev ? (dev.chartResolution || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartResolution = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Sample count:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 100; to: 15000; stepSize: 100; devValue: dev ? (dev.chartSamples || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartSamples = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Offset:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 10000; stepSize: 100; devValue: dev ? (dev.chartOffset || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartOffset = v } }
        }
    }

    // ── Дальномер ─────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Rangefinder"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.rangefinder"; collapsedByDefault: true

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Max distance, mm:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 1000; devValue: dev ? (dev.distMax || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distMax = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Dead zone, mm:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 100; devValue: dev ? (dev.distDeadZone || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distDeadZone = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Confidence threshold, %:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 100; stepSize: 1; devValue: dev ? (dev.distConfidence || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distConfidence = v } }
        }
    }

    // ── Преобразователь ───────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Transducer"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.transducer"; collapsedByDefault: true

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Pulse count:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 5000; stepSize: 1; devValue: dev ? (dev.transPulse || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.transPulse = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Frequency, kHz:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 40; to: 6000; stepSize: 5; devValue: dev ? (dev.transFreq || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.transFreq = v } }
        }

        KSwitch {
            id: boosterSwitch
            width: parent.width; text: qsTr("Booster")
            checked: dev ? (dev.transBoost === 1) : false
            onToggled: { if (dev) dev.transBoost = checked ? 1 : 0 }
            Connections {
                target: dev; ignoreUnknownSignals: true
                function onTransBoostChanged() {
                    var v = dev.transBoost === 1
                    if (boosterSwitch.checked !== v) boosterSwitch.checked = v
                }
            }
        }
    }

    // ── DSP ───────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("DSP"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.dsp"; collapsedByDefault: true

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Horizontal smoothing:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 4; stepSize: 1; devValue: dev ? (dev.dspHorSmooth || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.dspHorSmooth = v } }
        }

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Sound speed, m/s:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 300; to: 6000; stepSize: 5; devValue: dev ? Math.round((dev.soundSpeed || 0) / 1000) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.soundSpeed = v * 1000 } }
        }
    }

    // ── Датасет ───────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Dataset"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.dataset"; collapsedByDefault: true

        Row {
            width: parent.width; height: 30; spacing: 8
            Text { text: qsTr("Period, ms:"); color: AppPalette.textSecond; font.pixelSize: 13; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 2000; stepSize: 50; devValue: dev ? (dev.ch1Period || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.ch1Period = v } }
        }

        Column {
            width: parent.width; spacing: 6
            Text { text: qsTr("Echogram:"); color: AppPalette.textSecond; font.pixelSize: 13 }
            KTabBar {
                id: datasetChartTab; width: parent.width
                options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("8-bit"), value: 1 }]
                currentValue: dev ? (dev.datasetChart === 1 ? 1 : 0) : 0
                onValueSelected: function(v) { if (dev) dev.datasetChart = v }
                Connections {
                    target: dev; ignoreUnknownSignals: true
                    function onDatasetChartChanged() { datasetChartTab.currentValue = dev.datasetChart === 1 ? 1 : 0 }
                }
            }
        }

        Column {
            width: parent.width; spacing: 6
            Text { text: qsTr("Rangefinder:"); color: AppPalette.textSecond; font.pixelSize: 13 }
            KTabBar {
                id: datasetDistTab; width: parent.width
                options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("On"), value: 1 }, { label: qsTr("NMEA"), value: 2 }]
                currentValue: dev ? (dev.datasetDist === 1 ? 1 : (dev.datasetSDDBT === 1 ? 2 : 0)) : 0
                onValueSelected: function(v) {
                    if (!dev) return
                    if (v === 1)      { dev.datasetDist = 1 }
                    else if (v === 2) { dev.datasetSDDBT = 1 }
                    else              { dev.datasetDist = 0; dev.datasetSDDBT = 0 }
                }
                Connections {
                    target: dev; ignoreUnknownSignals: true
                    function onDatasetDistChanged()  { datasetDistTab.currentValue = dev.datasetDist === 1 ? 1 : (dev.datasetSDDBT === 1 ? 2 : 0) }
                    function onDatasetSDDBTChanged() { datasetDistTab.currentValue = dev.datasetDist === 1 ? 1 : (dev.datasetSDDBT === 1 ? 2 : 0) }
                }
            }
        }

        KSwitch {
            id: ahrsSwitch; width: parent.width; text: qsTr("AHRS")
            checked: dev ? ((dev.datasetEuler & 1) === 1) : false
            onToggled: { if (dev) dev.datasetEuler = checked ? 1 : 0 }
            Connections {
                target: dev; ignoreUnknownSignals: true
                function onDatasetEulerChanged() {
                    var v = (dev.datasetEuler & 1) === 1
                    if (ahrsSwitch.checked !== v) ahrsSwitch.checked = v
                }
            }
        }

        KSwitch {
            id: tempSwitch; width: parent.width; text: qsTr("Temperature")
            checked: dev ? ((dev.datasetTemp & 1) === 1) : false
            onToggled: { if (dev) dev.datasetTemp = checked ? 1 : 0 }
            Connections {
                target: dev; ignoreUnknownSignals: true
                function onDatasetTempChanged() {
                    var v = (dev.datasetTemp & 1) === 1
                    if (tempSwitch.checked !== v) tempSwitch.checked = v
                }
            }
        }

        KSwitch {
            id: tsSwitch; width: parent.width; text: qsTr("Timestamp")
            checked: dev ? ((dev.datasetTimestamp & 1) === 1) : false
            onToggled: { if (dev) dev.datasetTimestamp = checked ? 1 : 0 }
            Connections {
                target: dev; ignoreUnknownSignals: true
                function onDatasetTimestampChanged() {
                    var v = (dev.datasetTimestamp & 1) === 1
                    if (tsSwitch.checked !== v) tsSwitch.checked = v
                }
            }
        }
    }

    // ── Действия ──────────────────────────────────────────────────────────

    SettingsGroup {
        id: devActionsGroup
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Actions"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.actions"; collapsedByDefault: false

        property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        Settings {
            property alias devImportFolder: devActionsGroup.importFolder
            property alias devExportFolder: devActionsGroup.exportFolder
        }

        FileDialog {
            id: importXmlDialog
            title: qsTr("Open settings file")
            fileMode: FileDialog.OpenFile
            nameFilters: ["XML files (*.xml)"]
            onCurrentFolderChanged: devActionsGroup.importFolder = currentFolder
            onAccepted: {
                devActionsGroup.importFolder = importXmlDialog.currentFolder
                var url = importXmlDialog.selectedFile
                if (!url) return
                var lp = url.toLocalFile ? url.toLocalFile() : url.toString()
                if (dev) dev.importSettingsFromXML(lp)
            }
        }

        FileDialog {
            id: exportXmlDialog
            title: qsTr("Save settings file")
            fileMode: FileDialog.SaveFile
            nameFilters: ["XML files (*.xml)"]
            onCurrentFolderChanged: devActionsGroup.exportFolder = currentFolder
            onAccepted: {
                devActionsGroup.exportFolder = exportXmlDialog.currentFolder
                var url = exportXmlDialog.selectedFile
                if (!url || url.toString() === "") return
                var lp = url.toLocalFile ? url.toLocalFile() : url.toString()
                if (dev) dev.exportSettingsToXML(lp)
            }
        }

        // Flash / Erase / Reboot
        Row {
            width: parent.width; spacing: 6
            readonly property real bw: (width - 12) / 3
            KButton { width: parent.bw; height: 30; text: qsTr("Save");     onClicked: { if (dev) dev.flashSettings() } }
            KButton { width: parent.bw; height: 30; text: qsTr("Reset"); danger: true; onClicked: { if (dev) dev.resetSettings() } }
            KButton { width: parent.bw; height: 30; text: qsTr("Reboot"); onClicked: { if (dev) dev.reboot() } }
        }

        // Baudrate
        Column {
            width: parent.width; spacing: 6
            Text { text: qsTr("Baudrate:"); color: AppPalette.textSecond; font.pixelSize: 13 }
            Row {
                width: parent.width; spacing: 6
                KTabBar {
                    id: baudrateTab
                    width: parent.width - setBaudrateBtn.implicitWidth - 6
                    options: [
                        { label: qsTr("115200"), value: 115200 },
                        { label: qsTr("230400"), value: 230400 },
                        { label: qsTr("460800"), value: 460800 },
                        { label: qsTr("921600"), value: 921600 }
                    ]
                    currentValue: 115200
                }
                KButton {
                    id: setBaudrateBtn
                    height: 30; text: qsTr("Set")
                    onClicked: { if (dev) dev.baudrate = baudrateTab.currentValue }
                }
            }
        }

        // Import / Export XML
        Row {
            width: parent.width; spacing: 6
            readonly property real bw: (width - 6) / 2
            KButton { width: parent.bw; height: 30; text: qsTr("Import XML"); onClicked: { importXmlDialog.currentFolder = devActionsGroup.importFolder; importXmlDialog.open() } }
            KButton { width: parent.bw; height: 30; text: qsTr("Export XML"); onClicked: { exportXmlDialog.currentFolder = devActionsGroup.exportFolder; exportXmlDialog.open() } }
        }
    }
}

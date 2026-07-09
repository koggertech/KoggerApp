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

    component DevButton: KButton {
        normalBg: AppPalette.controlRaised
        hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
    }

    // Section heading above the per-area device settings groups.
    Text {
        text: qsTr("Settings:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    // ── Recorder ──────────────────────────────────────────────────────────
    // Status snapshot + log archive with per-log batched download. Data comes from
    // dev.recorder* (ID_RECORDER_STATUS) and deviceManagerWrapper.streamsList; download
    // is driven by deviceManagerWrapper.startStreamDownload(id). See
    // RecorderN/docs/Recorder-Host-Integration-Guide.md.
    DeviceSettingsGroup {
        id: recorderGroup
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Recorder"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.recorder"; collapsedByDefault: false
        visible: !!(dev && dev.isRecorder)

        // real, not int: free/recorded bytes exceed 2^31 (QML int is 32-bit) — e.g.
        // free1m 62642 * 1 MiB ≈ 61 GB would overflow and wrap to ~1.2 GB.
        readonly property real recordedBytes: dev ? (dev.recorderRecordedSize64k || 0) * 65536.0 : 0
        readonly property real freeBytes:     dev ? (dev.recorderFreeSpace1m || 0) * 1048576.0 : 0

        function _cond(v)  { return [qsTr("Fine"), qsTr("Warning"), qsTr("Degraded"), qsTr("Critical")][v] || qsTr("Unknown") }
        function _condColor(v) { return v >= 3 ? AppPalette.dangerBorder : v === 2 ? "#E0803A" : v === 1 ? "#E0A83A" : AppPalette.accentBar }
        function _recState(v) { return [qsTr("Initializing"), qsTr("Idle"), qsTr("Recording"), qsTr("Critical"), qsTr("Critical (disabled)")][v] || qsTr("Unknown") }
        function _degr(v) { var a = []; if (v & 1) a.push(qsTr("LogDrop")); return a.length ? a.join(", ") : qsTr("none") }
        function _crit(v) { var a = []; if (v & 1) a.push(qsTr("StorageUnavailable")); if (v & 2) a.push(qsTr("RecordingBackendError")); return a.length ? a.join(", ") : qsTr("none") }
        function _fmtSize(b) {
            if (!b || b <= 0) return "0 B"
            if (b < 1024) return b + " B"
            if (b < 1048576) return (b / 1024).toFixed(1) + " KB"
            if (b < 1073741824) return (b / 1048576).toFixed(1) + " MB"
            return (b / 1073741824).toFixed(2) + " GB"
        }
        function _dur(s) {
            if (!s || s <= 0) return "0s"
            var h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60
            return (h > 0 ? h + "h " : "") + (m > 0 || h > 0 ? m + "m " : "") + sec + "s"
        }

        component StatRow: Row {
            property string label: ""
            property string value: ""
            property color valueColor: AppPalette.text
            property bool dot: false
            width: parent ? parent.width : 0
            height: Tokens.controlHSm ? Tokens.controlHSm : Math.round(22 * AppPalette.scale)
            spacing: Tokens.spaceSm
            Text {
                text: label; color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
                width: Math.round(150 * AppPalette.scale); elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                visible: dot; width: Math.round(8 * AppPalette.scale); height: width; radius: width / 2
                color: valueColor; anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: value; color: dot ? AppPalette.text : valueColor; font.pixelSize: Tokens.fontSm
                font.bold: true; anchors.verticalCenter: parent.verticalCenter
                width: parent.width - Math.round(150 * AppPalette.scale) - parent.spacing - (dot ? Math.round(8 * AppPalette.scale) + parent.spacing : 0)
                elide: Text.ElideRight
            }
        }

        // Status snapshot
        Column {
            width: parent.width; spacing: Tokens.spaceXxs

            Text {
                visible: !(dev && dev.recorderStatusValid)
                text: qsTr("Waiting for recorder status…")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
            }

            StatRow { label: qsTr("Condition:"); dot: true
                      valueColor: recorderGroup._condColor(dev ? dev.recorderDeviceCondition : 0)
                      value: recorderGroup._cond(dev ? dev.recorderDeviceCondition : 0)
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("State:"); value: recorderGroup._recState(dev ? dev.recorderRecordingState : 0)
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Current log:"); value: dev && dev.recorderCurrentLogId ? ("#" + dev.recorderCurrentLogId) : "—"
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Recorded:"); value: recorderGroup._fmtSize(recorderGroup.recordedBytes)
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Free space:"); value: recorderGroup._fmtSize(recorderGroup.freeBytes)
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Rec. duration:"); value: recorderGroup._dur(dev ? dev.recorderDurationSeconds : 0)
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Last write:")
                      // seconds_since_last_write reads 0 before any write; show "—" until
                      // the current log has actually recorded something.
                      value: (dev && (dev.recorderRecordedSize64k > 0 || dev.recorderDurationSeconds > 0))
                             ? (dev.recorderSecondsSinceLastWrite + qsTr(" s ago")) : "—"
                      visible: !!(dev && dev.recorderStatusValid) }
            StatRow { label: qsTr("Degraded:"); value: recorderGroup._degr(dev ? dev.recorderDegradedFlags : 0)
                      valueColor: (dev && dev.recorderDegradedFlags) ? "#E0A83A" : AppPalette.textMuted
                      visible: !!(dev && dev.recorderStatusValid && dev.recorderDegradedFlags) }
            StatRow { label: qsTr("Critical:"); value: recorderGroup._crit(dev ? dev.recorderCriticalFlags : 0)
                      valueColor: AppPalette.dangerBorder
                      visible: !!(dev && dev.recorderStatusValid && dev.recorderCriticalFlags) }
        }

        // Logs header: count + refresh
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm
            Text {
                text: qsTr("Logs (%1)").arg(logList.count)
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; font.bold: true
                width: parent.width - refreshBtn.width - parent.spacing
                anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
            }
            DevButton {
                id: refreshBtn
                width: Math.round(96 * AppPalette.scale); height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Refresh")
                onClicked: if (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) deviceManagerWrapper.refreshStreamList()
            }
        }

        Text {
            visible: logList.count === 0
            width: parent.width
            text: qsTr("No logs listed yet — tap Refresh to enumerate the recorder's archive.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; wrapMode: Text.WordWrap
        }

        // Log archive — bounded, scrolls internally
        Rectangle {
            width: parent.width
            visible: logList.count > 0
            readonly property int rowH: Math.round(52 * AppPalette.scale)
            height: Math.min(logList.count * rowH, Math.round(360 * AppPalette.scale)) + 2
            color: AppPalette.bg; radius: Tokens.radiusMd
            border.width: Tokens.cardBorderWidth; border.color: AppPalette.border
            clip: true

            ListView {
                id: logList
                anchors.fill: parent; anchors.margins: 1
                clip: true
                model: (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) ? deviceManagerWrapper.streamsList : null
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Item {
                    width: ListView.view ? ListView.view.width : 0
                    height: Math.round(52 * AppPalette.scale)

                    readonly property bool _uploading: uploadState === 3
                    readonly property bool _done: uploadState === 1 && doneSize > 0
                    readonly property real _pct: size > 0 ? Math.max(0, Math.min(1, doneSize / size)) : 0

                    Row {
                        id: infoRow
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                        anchors.topMargin: Tokens.spaceXs
                        height: Tokens.controlHSm ? Tokens.controlHSm : Math.round(24 * AppPalette.scale)
                        spacing: Tokens.spaceSm

                        Text {
                            text: "#" + id; color: AppPalette.text; font.pixelSize: Tokens.fontMd; font.bold: true
                            width: Math.round(52 * AppPalette.scale); anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            text: recorderGroup._fmtSize(size); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
                            width: Math.round(84 * AppPalette.scale); anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: _uploading ? qsTr("Downloading… %1%").arg(Math.round(_pct * 100))
                                             : (_done ? qsTr("Saved") : (recordState === 3 ? qsTr("Recording") : ""))
                            color: _done ? AppPalette.accentBar : AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm
                            width: parent.width - Math.round(52 * AppPalette.scale) - Math.round(84 * AppPalette.scale)
                                   - dlBtn.width - 3 * parent.spacing
                            anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
                        }
                        DevButton {
                            id: dlBtn
                            width: Math.round(104 * AppPalette.scale); height: Math.round(26 * AppPalette.scale)
                            fontPixelSize: Tokens.fontSm
                            danger: _uploading
                            text: _uploading ? qsTr("Cancel") : (_done ? qsTr("Re-download") : qsTr("Download"))
                            anchors.verticalCenter: parent.verticalCenter
                            onClicked: {
                                if (typeof deviceManagerWrapper === "undefined" || !deviceManagerWrapper)
                                    return
                                if (_uploading)
                                    deviceManagerWrapper.cancelStreamDownload(id)
                                else
                                    deviceManagerWrapper.startStreamDownload(id)
                            }
                        }
                    }

                    // Per-log progress bar
                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                        anchors.bottomMargin: Tokens.spaceXs
                        height: Math.round(4 * AppPalette.scale); radius: height / 2
                        color: AppPalette.trackOff
                        visible: _uploading || _done
                        Rectangle {
                            height: parent.height; radius: parent.radius
                            width: parent.width * _pct
                            color: AppPalette.accentBar
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1; color: AppPalette.border; opacity: 0.5
                        visible: index < logList.count - 1
                    }
                }
            }
        }

        Text {
            width: parent.width
            text: qsTr("Downloads are saved to Documents/KoggerApp/recorder/.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; wrapMode: Text.WordWrap
        }
    }

    // ── Эхограмма ─────────────────────────────────────────────────────────

    DeviceSettingsGroup {
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

    DeviceSettingsGroup {
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

    DeviceSettingsGroup {
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

    DeviceSettingsGroup {
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

    DeviceSettingsGroup {
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

    DeviceSettingsGroup {
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
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Flash settings")
                onClicked: { if (dev) { dev.flashSettings(); notifications.info(qsTr("Settings written to device: %1").arg(dev.devName)) } }
            }
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Erase settings"); danger: true
                onClicked: { if (dev) { dev.resetSettings(); notifications.info(qsTr("Settings erased on device: %1").arg(dev.devName)) } }
            }
            DevButton {
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
            DevButton {
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

    DeviceSettingsGroup {
        id: devSettingsGroup
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Settings"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.settingsFile"; collapsedByDefault: true

        property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        Settings {
            category: "main/devices"
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
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Import XML")
                onClicked: { importXmlDialog.currentFolder = devSettingsGroup.importFolder; importXmlDialog.open() }
            }
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Export XML")
                onClicked: { exportXmlDialog.currentFolder = devSettingsGroup.exportFolder; exportXmlDialog.open() }
            }
        }
    }

    DeviceSettingsGroup {
        id: devUpgradeGroup
        visible: !!(dev && dev.isUpgradeSupport)
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Upgrade"); titlePixelSize: 13
        stateStore: root.store; stateKey: "dev.upgrade"; collapsedByDefault: true

        property var upgradeFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property string selectedUpgradePathSource: ""

        Settings { category: "main/devices"; property alias devUpgradeFolder: devUpgradeGroup.upgradeFolder }

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
                color: AppPalette.bg
                border.width: upgradePathInput.activeFocus ? 1 : Tokens.cardBorderWidth
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
            DevButton {
                width: parent.browseW; height: Tokens.controlHMd; text: "..."; fontPixelSize: Tokens.fontMd
                onClicked: { upgradeFileDialog.currentFolder = devUpgradeGroup.upgradeFolder; upgradeFileDialog.open() }
            }
        }
        DevButton {
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

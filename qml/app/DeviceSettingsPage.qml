import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import "RecorderStatus.js" as RecorderStatus

Column {
    id: root

    property var dev: null
    property var store: null

    // Per-device expand memory. This page is one instance whose `dev` changes on switch,
    // so state is kept per device: on switch, snapshot the leaving device's groups and
    // restore the entering device's (all collapsed if unseen). Entries for disconnected
    // devices are pruned — a reconnect is a new object → fresh defaults.
    property var _groupStates: []   // [{ dev, map: { stateKey: bool } }]
    property var _prevDev: null

    onDevChanged: {
        if (_prevDev) {
            var prev = _groupStateFor(_prevDev)
            if (prev) prev.map = _snapshotGroups()
            else _groupStates.push({ dev: _prevDev, map: _snapshotGroups() })
        }
        _pruneGroupStates()
        _applyForCurrentDev()
        _prevDev = dev
    }

    Component.onCompleted: _applyForCurrentDev()

    function _applyForCurrentDev() {
        var cur = dev ? _groupStateFor(dev) : null
        _applyGroups(cur ? cur.map : null)
    }

    function _groupStateFor(d) {
        for (var i = 0; i < _groupStates.length; ++i)
            if (_groupStates[i].dev === d) return _groupStates[i]
        return null
    }

    function _snapshotGroups() {
        var m = {}
        for (var i = 0; i < children.length; ++i) {
            var g = children[i]
            if (g && g.stateKey !== undefined && typeof g.expanded === "boolean")
                m[g.stateKey] = g.expanded
        }
        return m
    }

    function _applyGroups(map) {
        for (var i = 0; i < children.length; ++i) {
            var g = children[i]
            if (!g || g.stateKey === undefined || typeof g.expanded !== "boolean")
                continue
            if (g.bodyAnimated !== undefined)
                g.bodyAnimated = false   // programmatic: snap, no expand/collapse flicker
            g.expanded = (map && map[g.stateKey] !== undefined) ? map[g.stateKey] : true
        }
        Qt.callLater(_reenableGroupAnim)
    }

    function _reenableGroupAnim() {
        for (var i = 0; i < children.length; ++i) {
            var g = children[i]
            if (g && g.bodyAnimated !== undefined)
                g.bodyAnimated = true
        }
    }

    function _pruneGroupStates() {
        var ds = (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) ? deviceManagerWrapper.devs : []
        var kept = []
        for (var i = 0; i < _groupStates.length; ++i) {
            var e = _groupStates[i], present = false
            for (var j = 0; j < ds.length; ++j)
                if (ds[j] === e.dev) { present = true; break }
            if (present)
                kept.push(e)
        }
        _groupStates = kept
    }

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
        dangerBg: AppPalette.controlRaised
        dangerHoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
        borderWidth: danger ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : Tokens.cardBorderWidth
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
        stateKey: "dev.recorder"; collapsedByDefault: false
        visible: !!(dev && dev.isRecorder)

        // Input ports live in the HEADER — visible even when the group is collapsed.
        // Compact: dot + short label (S1/S2/Nav). gray=never · green=live · red=lost.
        headerActions: Row {
            id: portsRow
            readonly property int gap: Tokens.spaceSm
            spacing: gap
            rightPadding: gap
            visible: !!(dev && dev.recorderStatusValid)
            readonly property int dotSize: Math.round(8 * AppPalette.scale)
            readonly property int innerGap: Math.round(6 * AppPalette.scale)
            Repeater {
                model: recorderGroup._sourceChips()
                delegate: Rectangle {
                    readonly property color accent: modelData.state === "live" ? AppPalette.linkOkBorder
                                                  : modelData.state === "lost" ? AppPalette.linkDownBorder
                                                  : AppPalette.textMuted
                    readonly property bool off: modelData.state === "off"
                    implicitWidth: chipContent.implicitWidth + Tokens.spaceLg * 2
                    width: implicitWidth
                    height: recorderGroup.headerActionSize - portsRow.gap * 2; radius: height / 2
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                    color: "transparent"
                    border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                    border.color: off ? AppPalette.border : accent
                    Row {
                        id: chipContent
                        anchors.centerIn: parent; spacing: portsRow.innerGap
                        Rectangle {
                            width: portsRow.dotSize; height: width; radius: width / 2
                            anchors.verticalCenter: parent.verticalCenter
                            color: off ? "transparent" : accent
                            border.width: off ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : 0
                            border.color: AppPalette.textMuted
                        }
                        Text { text: modelData.short; color: AppPalette.textStrong
                               font.pixelSize: Tokens.fontBase; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }

        // real, not int: free/recorded bytes exceed 2^31 (QML int is 32-bit) and would wrap.
        // recorded_size is 64 KiB units (2^16); free_space is 1 MB = 10^6-byte units (matches
        // the firmware's freeSpaceBytes()/1000000).
        readonly property real recordedBytes: dev ? (dev.recorderRecordedSize64k || 0) * 65536.0 : 0
        readonly property real freeBytes:     dev ? (dev.recorderFreeSpace1m || 0) * 1000000.0 : 0

        function _fmtSize(b) { return RecorderStatus.fmtSize(b) }
        function _elapsed(s) { return RecorderStatus.elapsed(s) }
        function _logDuration(s) {
            if (!s || s <= 0) return "0" + qsTr("s")
            var h = Math.floor(s / 3600)
            var m = Math.floor((s % 3600) / 60)
            var sec = Math.floor(s % 60)
            var pad = function(n) { return (n < 10 ? "0" : "") + n }
            if (h > 0)   return h + qsTr("h") + " " + pad(m) + qsTr("m")
            if (m > 0)   return m + qsTr("m") + " " + pad(sec) + qsTr("s")
            return sec + qsTr("s")
        }

        readonly property int kStallS: 10

        // Join source names for flag-word bits 0/1/2 (Sonar1=1, Sonar2=2, Nav=4).
        function _sources(v) {
            var a = []
            if (v & 1) a.push(qsTr("Sonar 1"))
            if (v & 2) a.push(qsTr("Sonar 2"))
            if (v & 4) a.push(qsTr("Nav"))
            if (!a.length) return ""
            return a.length === 1 ? a[0] : a.slice(0, -1).join(", ") + qsTr(" & ") + a[a.length - 1]
        }

        // Semantic severity accent (from the app palette; distinct from the blue accent).
        function _sevColor(s) {
            if (s === "good")  return AppPalette.linkOkBorder
            if (s === "warn")  return AppPalette.linkIdleBorder
            if (s === "crit")  return AppPalette.linkDownBorder
            if (s === "stale") return AppPalette.textMuted
            return AppPalette.accentBar
        }
        function _sevText(s) {
            if (s === "good")  return AppPalette.linkOkText
            if (s === "warn")  return AppPalette.linkIdleText
            if (s === "crit")  return AppPalette.linkDownText
            if (s === "stale") return AppPalette.textMuted
            return AppPalette.accentBar
        }

        // Worst-state-wins hero: { sev, word, sub, pulse }. Mirrors the UX concept: the one
        // line that answers "is my data being saved right now, and if not, what broke?".
        readonly property var hero: _hero()
        function _hero() {
            var sev = (!dev || !dev.recorderStatusValid) ? "idle"
                    : stale ? "stale"
                    : RecorderStatus.severity(dev)
            if (!dev || !dev.recorderStatusValid)
                return { sev: sev, word: qsTr("Connecting…"), sub: qsTr("Waiting for the recorder."), pulse: false }
            if (stale)
                return { sev: sev, word: qsTr("No response"),
                         sub: qsTr("No update for %1 — showing last known.").arg(_elapsed(staleAgeS)), pulse: false }
            var st = dev.recorderRecordingState
            if (st === 3 || dev.recorderCriticalFlags)
                return { sev: sev,
                         word: (dev.recorderCriticalFlags & 1) ? qsTr("Storage lost") : qsTr("Recording failed"),
                         sub: qsTr("Recording stopped — check the recorder."), pulse: false }
            if (st === 4)
                return { sev: sev, word: qsTr("Recording off"), sub: qsTr("Recording is disabled."), pulse: false }
            if (st === 2) {
                var active = dev.recorderStatusFlags || 0
                var silentBits = (dev.recorderDegradedFlags || 0) & 0x7
                // Not writing: no data landed for a while, OR every seen source has gone
                // silent. Still green if another source is feeding (that rides as an alert).
                if (dev.recorderSecondsSinceLastWrite > kStallS || (silentBits && !active)) {
                    var sil = _sources(dev.recorderDegradedFlags || 0)
                    var gap = _elapsed(dev.recorderSecondsSinceLastWrite)
                    return { sev: sev, word: qsTr("Not writing"),
                             sub: sil.length ? qsTr("No data from %1 · nothing recorded for %2").arg(sil).arg(gap)
                                             : qsTr("Nothing recorded for %1 — source silent.").arg(gap),
                             pulse: false }
                }
                var lostBits = silentBits & ~active
                if (lostBits) {
                    var seen = active | lostBits
                    var defs = [{ b: 1, n: qsTr("Sonar 1") }, { b: 2, n: qsTr("Sonar 2") }, { b: 4, n: qsTr("Nav") }]
                    var parts = []
                    for (var i = 0; i < defs.length; i++)
                        if (seen & defs[i].b)
                            parts.push((active & defs[i].b ? qsTr("%1 connected") : qsTr("%1 disconnected")).arg(defs[i].n))
                    var detail = parts.join(", ")
                    var logId2 = dev.recorderCurrentLogId || 0
                    return { sev: sev, word: qsTr("Recording"),
                             sub: logId2 > 0 ? qsTr("Saving to log #%1 — %2").arg(logId2).arg(detail)
                                             : qsTr("Saving — %1").arg(detail),
                             pulse: true }
                }
                var live = _sources(active)
                var liveN = (active & 1 ? 1 : 0) + (active & 2 ? 1 : 0) + (active & 4 ? 1 : 0)
                var logId = dev.recorderCurrentLogId || 0
                var sub
                if (logId > 0)
                    sub = live.length ? (liveN > 1 ? qsTr("Saving to log #%1 — %2 are connected.").arg(logId).arg(live)
                                                   : qsTr("Saving to log #%1 — %2 connected.").arg(logId).arg(live))
                                      : qsTr("Saving to log #%1.").arg(logId)
                else
                    sub = live.length ? (liveN > 1 ? qsTr("Saving — %1 are connected.").arg(live)
                                                   : qsTr("Saving — %1 connected.").arg(live))
                                      : qsTr("Saving.")
                return { sev: sev, word: qsTr("Recording"), sub: sub, pulse: true }
            }
            if (st === 1)
                return { sev: sev, word: qsTr("Idle"), sub: qsTr("Armed — waiting for data."), pulse: false }
            return { sev: sev, word: qsTr("Starting…"), sub: qsTr("Recorder is coming up."), pulse: false }
        }

        // Input ports — ALL THREE always shown. Per-port state: "live" (status_flags,
        // green), "lost" (degraded_flags, red), "off" (neither seen, gray hollow).
        function _sourceChips() {
            if (!dev || !dev.recorderStatusValid) return []
            var sf = dev.recorderStatusFlags || 0, df = dev.recorderDegradedFlags || 0
            var defs = [{ b: 1, n: qsTr("Sonar 1"), s: qsTr("S1") },
                        { b: 2, n: qsTr("Sonar 2"), s: qsTr("S2") },
                        { b: 4, n: qsTr("Nav"), s: qsTr("Nav") }]
            var out = []
            for (var i = 0; i < defs.length; i++)
                out.push({ name: defs[i].n, short: defs[i].s,
                           state: (sf & defs[i].b) ? "live" : ((df & defs[i].b) ? "lost" : "off") })
            return out
        }

        // Severity tint for the banner background (low-alpha sev colour; idle = none).
        function _sevBg(s) {
            if (s === "idle") return "transparent"
            var c = _sevColor(s)
            return Qt.rgba(c.r, c.g, c.b, 0.12)
        }
        // Free-space "error" (blink): storage fault, or free space critically low (<2 GB).
        function _freeError() {
            if (!dev || !dev.recorderStatusValid) return false
            if (dev.recorderCriticalFlags) return true
            return recorderGroup.freeBytes > 0 && recorderGroup.freeBytes < 2000000000
        }

        // A vital-number tile for the banner: label + value. keyBox = green (good), errorBox
        // = red border + blink, alertBox = red static (e.g. dropping frames).
        component StatBox: Rectangle {
            property string blabel: ""
            property string bvalue: ""
            property bool keyBox: false
            property bool errorBox: false
            property bool alertBox: false
            height: Math.round(42 * AppPalette.scale); radius: Tokens.radiusMd
            implicitWidth: sbCol.implicitWidth + Tokens.spaceMd * 2; width: implicitWidth
            color: AppPalette.card
            border.width: 1
            border.color: (errorBox || alertBox) ? AppPalette.linkDownBorder
                        : keyBox ? Qt.rgba(AppPalette.linkOkBorder.r, AppPalette.linkOkBorder.g, AppPalette.linkOkBorder.b, 0.45)
                        : AppPalette.border
            Column {
                id: sbCol
                anchors.centerIn: parent; spacing: Math.round(1 * AppPalette.scale)
                Text { visible: blabel.length > 0; text: blabel; color: AppPalette.textMuted
                       font.pixelSize: Tokens.fontXs }
                Text { text: bvalue; font.pixelSize: Tokens.fontSm; font.bold: true
                       color: (errorBox || alertBox) ? AppPalette.linkDownText
                            : keyBox ? AppPalette.linkOkText : AppPalette.text }
            }
            Rectangle {   // blink glow when errorBox
                anchors.fill: parent; radius: parent.radius; color: "transparent"
                border.width: Math.round(2 * AppPalette.scale); border.color: AppPalette.linkDownBorder
                visible: errorBox
                SequentialAnimation on opacity {
                    running: errorBox; loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.12; duration: 560; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 0.12; to: 1.0; duration: 560; easing.type: Easing.InOutSine }
                }
            }
        }

        // Freshness: recorderStatusChanged fires every poll (~3s). If it stops (e.g. the
        // telem link is pulled — the serial port stays "open", so link state won't notice)
        // the last snapshot is stale and the device's real state is unknown. Track the age
        // of the last status and flag it, so the readout isn't mistaken for live truth.
        property double _lastStatusMs: 0
        property double _nowMs: 0
        readonly property bool stale: _lastStatusMs > 0 && (_nowMs - _lastStatusMs) > 7000
        readonly property int staleAgeS: _lastStatusMs > 0 ? Math.floor((_nowMs - _lastStatusMs) / 1000) : 0

        Timer { interval: 1000; repeat: true; running: true; onTriggered: recorderGroup._nowMs = Date.now() }
        Connections {
            target: dev
            ignoreUnknownSignals: true
            function onRecorderStatusChanged() { recorderGroup._lastStatusMs = Date.now(); recorderGroup._nowMs = Date.now() }
        }

        // ── Banner: severity-tinted; hero + vital boxes. Ports are in the header. ──
        Rectangle {
            id: banner
            width: parent.width; radius: Tokens.radiusMd
            color: recorderGroup._sevBg(recorderGroup.hero.sev)
            border.width: recorderGroup.hero.sev === "idle" ? 1 : 0
            border.color: AppPalette.border
            implicitHeight: bannerCol.implicitHeight + Tokens.spaceMd * 2; height: implicitHeight
            opacity: recorderGroup.stale ? 0.6 : 1.0

            Rectangle {   // left severity stripe
                anchors.left: parent.left; anchors.leftMargin: Math.round(2 * AppPalette.scale)
                anchors.top: parent.top; anchors.bottom: parent.bottom
                anchors.topMargin: parent.radius; anchors.bottomMargin: parent.radius
                width: Math.round(3 * AppPalette.scale); radius: width / 2
                color: recorderGroup._sevColor(recorderGroup.hero.sev)
                visible: recorderGroup.hero.sev !== "idle"
            }

            Column {
                id: bannerCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: Tokens.spaceMd + Math.round(4 * AppPalette.scale)
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.spaceMd

                // hero: disc + word + sub
                Row {
                    width: parent.width; spacing: Tokens.spaceMd

                    Item {
                        id: heroDisc
                width: Math.round(16 * AppPalette.scale); height: width
                anchors.verticalCenter: heroText.verticalCenter
                Rectangle {   // pulse ring while actively writing
                    id: pulseRing
                    anchors.centerIn: parent; width: parent.width; height: parent.height; radius: width / 2
                    color: "transparent"; border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                    border.color: recorderGroup._sevColor(recorderGroup.hero.sev)
                    visible: recorderGroup.hero.pulse
                    ParallelAnimation {
                        running: recorderGroup.hero.pulse; loops: Animation.Infinite
                        NumberAnimation { target: pulseRing; property: "scale"; from: 1.0; to: 2.4; duration: 1500; easing.type: Easing.OutQuad }
                        NumberAnimation { target: pulseRing; property: "opacity"; from: 0.5; to: 0.0; duration: 1500; easing.type: Easing.OutQuad }
                    }
                }
                Rectangle {   // the severity disc
                    anchors.centerIn: parent; width: parent.width; height: parent.height; radius: width / 2
                    color: recorderGroup._sevColor(recorderGroup.hero.sev)
                }
            }
            Column {
                id: heroText
                width: parent.width - heroDisc.width - parent.spacing
                spacing: Math.round(2 * AppPalette.scale)
                Text { text: recorderGroup.hero.word; color: recorderGroup._sevText(recorderGroup.hero.sev)
                       font.pixelSize: Tokens.fontXxl; font.bold: true
                       width: parent.width; elide: Text.ElideRight }
                Text { text: recorderGroup.hero.sub; color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
                       width: parent.width; wrapMode: Text.WordWrap }
            }
        }

                // vital boxes: free space (blinks on error), current log · bytes, dropping frames
                Flow {
                    width: parent.width; spacing: Tokens.spaceSm
                    StatBox {
                        blabel: qsTr("Free space")
                        bvalue: (dev && (dev.recorderCriticalFlags & 1)) ? "—" : recorderGroup._fmtSize(recorderGroup.freeBytes)
                        errorBox: recorderGroup._freeError()
                        keyBox: !recorderGroup._freeError() && (recorderGroup.hero.sev === "good" || recorderGroup.hero.sev === "idle")
                    }
                    StatBox {
                        blabel: qsTr("Current log")
                        bvalue: (dev && dev.recorderCurrentLogId)
                                ? ("#" + dev.recorderCurrentLogId + " · " + recorderGroup._logDuration(dev.recorderDurationSeconds) + " · " + recorderGroup._fmtSize(recorderGroup.recordedBytes))
                                : qsTr("No log yet")
                    }
                    StatBox {
                        visible: !!(dev && dev.recorderStatusValid && (dev.recorderDegradedFlags & 0x8))
                        alertBox: true
                        bvalue: qsTr("⚠ Dropping frames")
                    }
                }
            }
        }

        // Stale note — the banner already shows "No response"; this labels the frozen values.
        Text {
            visible: recorderGroup.stale
            width: parent.width
            text: qsTr("Values may be outdated.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
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
                width: Math.round(112 * AppPalette.scale); height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
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
            readonly property int rowH: Tokens.controlHMd + Tokens.spaceMd * 2
            height: Math.min(logList.count * rowH, Math.round(rowH * 3.5)) + 2
            color: AppPalette.bg; radius: Tokens.radiusMd
            border.width: Tokens.cardBorderWidth; border.color: AppPalette.border
            clip: true

            ListView {
                id: logList
                anchors.fill: parent; anchors.margins: 1
                clip: true
                model: (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) ? deviceManagerWrapper.streamsList : null
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    id: logScroll
                    policy: ScrollBar.AsNeeded
                    implicitWidth: Tokens.spaceLg
                    readonly property int _handleW: Math.max(6, Math.round(7 * AppPalette.scale))
                    leftPadding: (implicitWidth - _handleW) / 2
                    rightPadding: leftPadding
                    topPadding: Math.round(2 * AppPalette.scale)
                    bottomPadding: topPadding

                    property bool _shown: false
                    onActiveChanged: { if (active) { _shown = true; hideTimer.stop() } else hideTimer.restart() }
                    Timer { id: hideTimer; interval: 1100; onTriggered: logScroll._shown = false }

                    contentItem: Rectangle {
                        radius: width / 2
                        color: logScroll.pressed ? AppPalette.borderHover : AppPalette.border
                        opacity: logScroll._shown ? 1.0 : 0.0
                        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }
                    }
                }

                delegate: Item {
                    id: rowItem
                    width: ListView.view ? ListView.view.width : 0
                    height: Tokens.controlHMd + Tokens.spaceMd * 2

                    readonly property bool _uploading: uploadState === 3
                    readonly property bool _done: uploadState === 1 && doneSize > 0
                    readonly property real _pct: size > 0 ? Math.max(0, Math.min(1, doneSize / size)) : 0
                    // active = downloading, or the brief reveal after a user-initiated download
                    // finishes; drives the progress bar + text-lift, then auto-reverts.
                    property bool _initiated: false
                    property bool _revealDone: false
                    readonly property bool _active: _uploading || _revealDone
                    readonly property int _shift: Math.round(7 * AppPalette.scale)

                    on_DoneChanged: {
                        if (_done && _initiated) {
                            _initiated = false
                            _revealDone = true
                            hideTimer.restart()
                            if (typeof notifications !== "undefined" && notifications)
                                notifications.info(qsTr("Saved \"%1\" to \"%2\"").arg("recorder_log_" + id + ".kp2").arg(recorderGroup._downloadDir))
                        }
                    }
                    on_UploadingChanged: if (_uploading) { _revealDone = false; hideTimer.stop() }

                    Timer { id: hideTimer; interval: 4000; onTriggered: rowItem._revealDone = false }

                    DevButton {
                        id: dlBtn
                        anchors.right: parent.right; anchors.rightMargin: Tokens.spaceLg
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(112 * AppPalette.scale); height: Tokens.controlHMd
                        fontPixelSize: Tokens.fontMd
                        danger: rowItem._uploading
                        text: rowItem._uploading ? qsTr("Cancel") : (rowItem._revealDone ? qsTr("Re-download") : qsTr("Download"))
                        onClicked: {
                            if (typeof deviceManagerWrapper === "undefined" || !deviceManagerWrapper)
                                return
                            if (rowItem._uploading)
                                deviceManagerWrapper.cancelStreamDownload(id)
                            else {
                                rowItem._initiated = true
                                deviceManagerWrapper.startStreamDownload(id)
                            }
                        }
                    }

                    Row {
                        id: textRow
                        anchors.left: parent.left; anchors.leftMargin: Tokens.spaceLg
                        anchors.right: dlBtn.left; anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: rowItem._active ? -rowItem._shift : 0
                        Behavior on anchors.verticalCenterOffset { NumberAnimation { duration: Anim.fadeMs; easing.type: Easing.OutCubic } }
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
                            id: statusText
                            text: rowItem._uploading ? qsTr("Downloading… %1%").arg(Math.round(rowItem._pct * 100))
                                             : (rowItem._revealDone ? qsTr("Saved") : (recordState === 3 ? qsTr("Recording") : ""))
                            color: rowItem._revealDone ? AppPalette.accentBar : AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm
                            width: textRow.width - Math.round(52 * AppPalette.scale) - Math.round(84 * AppPalette.scale) - 2 * textRow.spacing
                            anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight

                            readonly property bool _recPulse: recordState === 3 && !rowItem._uploading && !rowItem._revealDone
                            property real _pulseOp: 1.0
                            opacity: recPulseAnim.running ? _pulseOp : 1.0
                            SequentialAnimation {
                                id: recPulseAnim
                                running: statusText._recPulse
                                loops: Animation.Infinite
                                NumberAnimation { target: statusText; property: "_pulseOp"; from: 1.0; to: 0.4; duration: 800; easing.type: Easing.InOutSine }
                                NumberAnimation { target: statusText; property: "_pulseOp"; from: 0.4; to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                            }
                        }
                    }

                    Rectangle {
                        id: progress
                        anchors.left: parent.left; anchors.leftMargin: Tokens.spaceLg
                        anchors.right: dlBtn.left; anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: Math.round(9 * AppPalette.scale)
                        height: Math.round(4 * AppPalette.scale); radius: height / 2
                        color: AppPalette.trackOff
                        opacity: rowItem._active ? 1 : 0
                        visible: opacity > 0.01
                        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }
                        Rectangle {
                            height: parent.height; radius: parent.radius
                            width: parent.width * rowItem._pct
                            color: AppPalette.accentBar
                            Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
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

        // Mirrors stream_list.cpp: QStandardPaths::DocumentsLocation + "/KoggerApp/recorder".
        readonly property string _downloadDir: {
            var s = String(StandardPaths.writableLocation(StandardPaths.DocumentsLocation))
            if (s.indexOf("file:///") === 0)      s = (Qt.platform.os === "windows" ? s.slice(8) : s.slice(7))
            else if (s.indexOf("file://") === 0)  s = s.slice(7)
            try { s = decodeURIComponent(s) } catch (e) {}
            return s + "/KoggerApp/recorder"
        }

        Text {
            width: parent.width
            text: qsTr("Downloads are saved to \"%1\"").arg(recorderGroup._downloadDir)
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; wrapMode: Text.WordWrap
        }
    }

    // ── Эхограмма ─────────────────────────────────────────────────────────

    DeviceSettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Echogram"); titlePixelSize: 13
        stateKey: "dev.echogram"; collapsedByDefault: false
        visible: !!(dev && dev.isChartSupport)
        confirmed: !(dev && dev.chartSetupState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Resolution, mm"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 10; to: 100; stepSize: 10; devValue: dev ? (dev.chartResolution || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartResolution = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Sample count"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 100; to: 15000; stepSize: 100; devValue: dev ? (dev.chartSamples || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartSamples = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Offset"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 10000; stepSize: 100; devValue: dev ? (dev.chartOffset || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.chartOffset = v } }
        }
    }

    // ── Дальномер ─────────────────────────────────────────────────────────

    DeviceSettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Rangefinder"); titlePixelSize: 13
        stateKey: "dev.rangefinder"; collapsedByDefault: false
        visible: !!(dev && dev.isDistSupport)
        confirmed: !(dev && dev.distSetupState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Max distance, mm"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 1000; devValue: dev ? (dev.distMax || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distMax = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Dead zone, mm"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 50000; stepSize: 100; devValue: dev ? (dev.distDeadZone || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distDeadZone = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Confidence threshold, %"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 100; stepSize: 1; devValue: dev ? (dev.distConfidence || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.distConfidence = v } }
        }
    }

    // ── Преобразователь ───────────────────────────────────────────────────

    DeviceSettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Transducer"); titlePixelSize: 13
        stateKey: "dev.transducer"; collapsedByDefault: false
        visible: !!(dev && dev.isTransducerSupport)
        confirmed: !(dev && dev.transcState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Pulse count"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 5000; stepSize: 1; devValue: dev ? (dev.transPulse || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.transPulse = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Frequency, kHz"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
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
        stateKey: "dev.dsp"; collapsedByDefault: false
        visible: !!(dev && dev.isDSPSupport)
        confirmed: !(dev && (dev.dspState === false || dev.soundState === false))

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Horizontal smoothing"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 4; stepSize: 1; devValue: dev ? (dev.dspHorSmooth || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.dspHorSmooth = v } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Sound speed, m/s"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 300; to: 6000; stepSize: 5; devValue: dev ? Math.round((dev.soundSpeed || 0) / 1000) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.soundSpeed = v * 1000 } }
        }
    }

    // ── Датасет ───────────────────────────────────────────────────────────

    DeviceSettingsGroup {
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Dataset"); titlePixelSize: 13
        stateKey: "dev.dataset"; collapsedByDefault: false
        visible: !!(dev && dev.isDatasetSupport)
        confirmed: !(dev && dev.datasetState === false)

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
            Text { text: qsTr("Period, ms"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg; width: root.lblW; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
            DevSpin { from: 0; to: 2000; stepSize: 50; devValue: dev ? (dev.ch1Period || 0) : 0; anchors.verticalCenter: parent.verticalCenter; writeBack: function(v) { if (dev) dev.ch1Period = v } }
        }

        Column {
            width: parent.width; spacing: Tokens.spaceSm
            Text { text: qsTr("Echogram"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg }
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
            Text { text: qsTr("Rangefinder"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg }
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
        stateKey: "dev.actions"; collapsedByDefault: false
        confirmed: !(dev && dev.uartState === false)

        readonly property var baudrateOptions: [9600, 19200, 38400, 57600, 115200,
                                                230400, 460800, 921600, 1200000, 2000000]

        Row {
            width: parent.width; spacing: Tokens.spaceSm
            readonly property real bw: (width - 2 * Tokens.spaceSm) / 3
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Flash settings")
                toolTipText: qsTr("Write current settings to device memory")
                onClicked: { if (dev) { dev.flashSettings(); notifications.info(qsTr("Settings written to device: %1").arg(dev.devName)) } }
            }
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Erase settings"); danger: true
                toolTipText: qsTr("Erase device settings (reset)")
                onClicked: { if (dev) { dev.resetSettings(); notifications.info(qsTr("Settings erased on device: %1").arg(dev.devName)) } }
            }
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Reboot")
                toolTipText: qsTr("Reboot the device")
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
                toolTipText: qsTr("Apply the selected baud rate")
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
        title: qsTr("Settings file"); titlePixelSize: 13
        stateKey: "dev.settingsFile"; collapsedByDefault: false

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
                toolTipText: qsTr("Load all sonar settings from an XML file")
                onClicked: { importXmlDialog.currentFolder = devSettingsGroup.importFolder; importXmlDialog.open() }
            }
            DevButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Export XML")
                toolTipText: qsTr("Save all sonar settings to an XML file")
                onClicked: { exportXmlDialog.currentFolder = devSettingsGroup.exportFolder; exportXmlDialog.open() }
            }
        }
    }

    DeviceSettingsGroup {
        id: devUpgradeGroup
        visible: !!(dev && dev.isUpgradeSupport)
        width: root.groupWidth; preferredWidth: root.groupWidth
        title: qsTr("Upgrade"); titlePixelSize: 13
        stateKey: "dev.upgrade"; collapsedByDefault: false

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
            readonly property real browseW: Tokens.controlHMd
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
                width: Tokens.controlHMd; height: Tokens.controlHMd; text: "..."
                fontPixelSize: Tokens.fontLg; bold: false
                horizontalPadding: 0; verticalPadding: 0
                toolTipText: qsTr("Choose firmware")
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

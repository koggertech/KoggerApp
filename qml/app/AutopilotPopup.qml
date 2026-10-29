import QtQuick 2.15
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store

    readonly property real _s: AppPalette.appScale
    readonly property int _controlH: Math.round(36 * _s)
    readonly property int _sidePad: Math.round(3 * _s)
    readonly property int _gap: Math.round(6 * _s)
    readonly property int _pad: Math.round(12 * _s)
    readonly property int _armConfirmMinMs: 400
    readonly property int _armConfirmMs: 3000
    property real _armCountdown: 0
    readonly property int _severityCritical: 2
    readonly property int _severityWarning: 4
    readonly property int _severityInfo: 6

    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property bool _online: _dmw ? _dmw.autopilotOnline : false
    readonly property int _arm: _dmw ? _dmw.pilotArmState : -1
    readonly property int _mode: _dmw ? _dmw.pilotModeState : -1
    readonly property string _modeName: _dmw ? _dmw.autopilotModeName : ""
    readonly property real _voltage: _dmw ? _dmw.vruVoltage : NaN
    readonly property real _speed: _dmw ? _dmw.vruVelocityH : NaN
    readonly property real _current: _dmw ? _dmw.vruCurrent : NaN
    readonly property int _batteryPct: _dmw ? _dmw.vruBatteryPercent : -1
    readonly property int _linkQuality: _dmw ? _dmw.autopilotLinkQuality : -1
    readonly property bool _rssiValid: _dmw ? _dmw.radioRssiValid : false
    readonly property int _rssi: _dmw ? _dmw.radioRssi : 0
    readonly property bool _armed: _arm > 0
    readonly property int _gpsFix: _dmw ? _dmw.gpsFixType : -1
    readonly property int _gpsSats: _dmw ? _dmw.gpsSatellites : -1
    readonly property real _gpsHdop: _dmw ? _dmw.gpsHdop : NaN
    readonly property int _gpsFix3d: 3
    readonly property int _gpsMinSatellites: 6
    readonly property bool _gpsPoor: _gpsFix >= 0 && (_gpsFix < _gpsFix3d || (_gpsSats >= 0 && _gpsSats < _gpsMinSatellites))

    function _gpsFixName(fix) {
        switch (fix) {
        case 0: return qsTr("No GPS")
        case 1: return qsTr("No fix")
        case 2: return "2D"
        case 3: return "3D"
        case 4: return "DGPS"
        case 5: return qsTr("RTK float")
        case 6: return qsTr("RTK fixed")
        case 7: return qsTr("Static")
        case 8: return "PPP"
        }
        return "—"
    }

    readonly property var _run: (typeof missionRun !== "undefined") ? missionRun : null
    readonly property int _missionStatePaused: 4
    readonly property int _cmdSetMissionCurrent: 224
    readonly property int _messagesShown: 5
    readonly property real _earthRadiusM: 6371000.0
    readonly property bool _paused: !!_dmw && _dmw.missionState === _missionStatePaused
    readonly property bool _missionKnown: _online && !!_run && _run.known && !_run.vehicleEmpty
    readonly property bool _missionRowVisible: _missionKnown && (_run.running || _paused || (_run.complete && _armed))
    readonly property var _helm: (typeof helm !== "undefined") ? helm : null
    readonly property var _gamepad: (typeof gamepadMonitor !== "undefined") ? gamepadMonitor : null
    readonly property bool _helmActive: !!_helm && _helm.active
    readonly property bool _helmRowVisible: !!_helm && (_helm.available || _helm.active)
    readonly property int _helmConfirmation: _helm ? _helm.confirmation : 0
    property bool expanded: false
    readonly property bool _trayVisible: _online && (expanded || _missionRowVisible || _helmRowVisible)
    readonly property int _rowPad: _trayVisible ? Math.round(6 * _s) : _sidePad
    readonly property int _nextSeq: (_run && _run.running && _run.actionable && _run.targetSeq >= 0) ? _run.nextTargetSeq() : -1
    property bool _dragging: false

    property bool nextConfirmPending: false
    property real _nextFirstTapMs: 0
    property int _pendingJumpSeq: -1
    property int _seqAtSend: -1

    readonly property string _missionRowText: {
        if (!_run) return ""
        if (_run.complete) return qsTr("Mission complete")
        var parts = []
        if (_paused) parts.push(qsTr("Paused"))
        if (_run.currentNav > 0) parts.push(qsTr("Pt %1/%2").arg(_run.currentNav).arg(_run.navCount))
        if (isFinite(_run.remainingDistance)) parts.push(_fmtDist(_run.remainingDistance))
        if (isFinite(_run.remainingSeconds)) parts.push("~" + _fmtTime(_run.remainingSeconds))
        return parts.join(" · ")
    }

    readonly property bool _sourceIsPlan: !!_run && _run.known && !_run.stale && !_run.reading && !_run.vehicleEmpty && _run.matchesPlan
    readonly property string _missionSourceText: {
        if (!_run || !_online) return ""
        if (_run.reading) return qsTr("Reading the vehicle mission…")
        if (_run.known && _run.stale) return qsTr("The mission on the vehicle has changed; read it again")
        if (!_run.known) return qsTr("The vehicle mission has not been read")
        if (_run.vehicleEmpty) return qsTr("No mission on the vehicle")
        return _run.matchesPlan ? qsTr("This plan is on the vehicle") : qsTr("The vehicle has another mission than the open plan")
    }

    readonly property real _homeDistance: {
        if (!_dmw) return NaN
        var lat1 = _dmw.vehicleLat, lon1 = _dmw.vehicleLon, lat2 = _dmw.vehicleHomeLat, lon2 = _dmw.vehicleHomeLon
        if (!isFinite(lat1) || !isFinite(lon1) || !isFinite(lat2) || !isFinite(lon2)) return NaN
        var r = Math.PI / 180
        var dLat = (lat2 - lat1) * r
        var dLon = (lon2 - lon1) * r
        var a = Math.pow(Math.sin(dLat / 2), 2) + Math.cos(lat1 * r) * Math.cos(lat2 * r) * Math.pow(Math.sin(dLon / 2), 2)
        return 2 * _earthRadiusM * Math.asin(Math.min(1, Math.sqrt(a)))
    }

    function _fmtDist(m) {
        if (!isFinite(m)) return "—"
        return m < 1000 ? qsTr("%1 m").arg(Math.round(m)) : qsTr("%1 km").arg((m / 1000).toFixed(1))
    }

    function _fmtTime(sec) {
        if (!isFinite(sec)) return "—"
        if (sec < 60) return qsTr("<1 min")
        var minutes = Math.round(sec / 60)
        if (minutes < 60) return qsTr("%1 min").arg(minutes)
        return qsTr("%1 h %2 min").arg(Math.floor(minutes / 60)).arg(minutes % 60)
    }

    readonly property int _levelWarnPct: 30
    readonly property int _levelCriticalPct: 10

    function _levelColor(percent) {
        if (percent < 0) return AppPalette.textSecond
        if (percent <= _levelCriticalPct) return AppPalette.dangerText
        if (percent <= _levelWarnPct) return AppPalette.linkIdleText
        return AppPalette.textSecond
    }

    function _severityColor(severity) {
        if (severity <= 3) return AppPalette.dangerText
        if (severity === root._severityWarning) return AppPalette.linkIdleText
        return AppPalette.textSecond
    }

    function _jumpReached(current) {
        return _pendingJumpSeq >= 0 && current !== _seqAtSend && (current === _pendingJumpSeq || current === _pendingJumpSeq + 1)
    }

    function _cancelNextConfirm() {
        nextConfirmPending = false
        nextConfirmTimer.stop()
    }

    function requestNextPoint() {
        if (!_dmw || !_online || !_run || !_run.actionable || _nextSeq < 0) return
        if (!nextConfirmPending) {
            nextConfirmPending = true
            _nextFirstTapMs = Date.now()
            nextConfirmTimer.restart()
            return
        }
        if (Date.now() - _nextFirstTapMs < _armConfirmMinMs) return
        _cancelNextConfirm()
        var seq = _run.jumpSeqFor(_nextSeq)
        if (seq < 0) return
        _pendingJumpSeq = seq
        _seqAtSend = _dmw.missionCurrentSeq
        _send(_cmdSetMissionCurrent, function() { root._dmw.autopilotSetMissionCurrent(seq) })
        _log(_severityInfo, qsTr("Going to %1").arg(_run.labelForSeq(_nextSeq)))
    }

    readonly property var _modeNamesForWidth: ["Manual", "Acro", "Hold", "Loiter", "Auto", "RTL"]
    readonly property real _titleSlotW: Math.max(modeNameProbe.width, titleText.implicitWidth)
    readonly property var _statTemplates: ["100 %", "88.8 V", "88.8 A", "88.8 " + qsTr("m/s"), "88"]
    readonly property real _statValueW: Math.ceil(statValueProbe.width)
    readonly property real _statLastValueW: Math.ceil(statLastValueProbe.width)

    readonly property var modes: [
        { mode: 0,  group: 0, icon: "qrc:/icons/ui/mode-manual.svg", tip: qsTr("Manual") },
        { mode: 1,  group: 0, icon: "qrc:/icons/ui/mode-course-hold.svg", tip: qsTr("Course hold (Acro)"), offMode: 0 },
        { mode: 4,  group: 1, icon: "qrc:/icons/ui/mode-hold.svg",   tip: qsTr("Hold"), stop: true },
        { mode: 5,  group: 1, icon: "qrc:/icons/ui/mode-loiter.svg", tip: qsTr("Loiter") },
        { mode: 10, group: 2, icon: "qrc:/icons/ui/mode-auto.svg",   tip: qsTr("Auto") },
        { mode: 11, group: 2, icon: "qrc:/icons/ui/mode-rtl.svg",    tip: qsTr("RTL") }
    ]

    property bool armConfirmPending: false
    property real _armFirstTapMs: 0
    property int _armFlash: 0
    property int pendingCommand: -1

    readonly property real _pillW: _pad + statusDot.width + infoColumn.anchors.leftMargin + infoColumn.width
                                   + buttonRow.anchors.leftMargin + buttonRow.width + _sidePad
    readonly property real _pillH: _controlH + _rowPad * 2
    readonly property real _wantW: _pillW + contentPadding * 2
    readonly property real _wantH: headerHeight + _pillH + (_trayVisible ? tray.height : 0) + contentPadding

    popupVisible: store.autopilotPopupOpen
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    ghostFollowsContent: true
    ghostRadius: _pillH / 2
    headerDragBarLength: 0
    siblingSnapAlignTop: true
    snapEdgeCenters: true

    function _applySize() {
        expandedWidth = _wantW
        expandedHeight = _wantH
    }

    function _restorePlacement() {
        if (_synced && !_dragging && dockTargetId === "")
            syncFromStore()
    }

    property bool _synced: false

    function syncFromStore() {
        if (!popupVisible)
            return
        suspendSignals = true
        var p = store.autopilotPopupPosition(popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        suspendSignals = false
        _synced = true
    }

    function resultText(result) {
        switch (result) {
        case -1: return qsTr("no autopilot link")
        case -2: return qsTr("no response")
        case 1:  return qsTr("temporarily rejected")
        case 2:  return qsTr("denied")
        case 3:  return qsTr("unsupported")
        case 4:  return qsTr("failed")
        case 6:  return qsTr("cancelled")
        }
        return String(result)
    }

    function commandText(command) {
        switch (command) {
        case 400: return qsTr("Arm/disarm")
        case 176: return qsTr("Mode change")
        case 300: return qsTr("Mission start")
        case 224: return qsTr("Go to point")
        }
        return qsTr("Command %1").arg(command)
    }

    function _cancelArmConfirm() {
        armConfirmPending = false
        confirmTimer.stop()
    }

    function _send(command, call) {
        pendingCommand = command
        ackTimer.restart()
        call()
    }

    function requestArm() {
        if (!_dmw || !_online) return
        if (_armed) {
            _cancelArmConfirm()
            _send(400, function() { root._dmw.autopilotArm(false) })
            return
        }
        if (_helmActive && _helm.throttleHeld()) {
            _cancelArmConfirm()
            _log(_severityWarning, qsTr("Release the throttle before arming"))
            return
        }
        if (!armConfirmPending) {
            armConfirmPending = true
            _armFirstTapMs = Date.now()
            confirmTimer.restart()
            return
        }
        if (Date.now() - _armFirstTapMs < _armConfirmMinMs)
            return
        _cancelArmConfirm()
        _send(400, function() { root._dmw.autopilotArm(true) })
    }

    function requestMode(mode) {
        if (!_dmw || !_online || mode === _mode) return
        _cancelArmConfirm()
        _send(176, function() { root._dmw.autopilotSetMode(mode) })
    }

    function _warnFailure(command, result) {
        _log(_severityWarning, qsTr("%1: %2").arg(commandText(command)).arg(resultText(result)))
    }

    function _log(severity, text) {
        if (_dmw)
            _dmw.logAutopilotEvent(severity, text)
    }

    function _toast(text) {
        if (typeof notifications !== "undefined" && notifications)
            notifications.info(text)
    }


    on_WantWChanged: _applySize()
    on_WantHChanged: { _applySize(); Qt.callLater(_restorePlacement) }
    onInteractionStarted: _dragging = true
    on_OnlineChanged: if (!_online) { _cancelArmConfirm(); _cancelNextConfirm(); expanded = false; _pendingJumpSeq = -1 }
    on_NextSeqChanged: _cancelNextConfirm()
    onExpandedChanged: _cancelNextConfirm()
    on_ArmedChanged: _cancelArmConfirm()
    onArmConfirmPendingChanged: {
        if (armConfirmPending) {
            _armFlash += 1
            armCountdownAnim.restart()
        } else {
            armCountdownAnim.stop()
            _armCountdown = 0
        }
    }

    Component.onCompleted: {
        _applySize()
        syncFromStore()
        Qt.callLater(syncFromStore)
        Qt.callLater(resolveOverlapWithSibling)
    }

    onPopupVisibleChanged: {
        _cancelArmConfirm()
        if (popupVisible) {
            _applySize()
            syncFromStore()
            Qt.callLater(syncFromStore)
            Qt.callLater(resolveOverlapWithSibling)
        }
    }

    onPositionCommitted: function(x, y, w, h) {
        _dragging = false
        if (_synced)
            store.setAutopilotPopupPosition(x, y, w, h)
    }

    dockState: store ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    onCloseRequested: store.autopilotPopupOpen = false

    Timer {
        id: confirmTimer
        interval: root._armConfirmMs
        onTriggered: root.armConfirmPending = false
    }

    NumberAnimation {
        id: armCountdownAnim
        target: root
        property: "_armCountdown"
        from: 1
        to: 0
        duration: root._armConfirmMs
    }

    Timer {
        id: ackTimer
        interval: 3000
        onTriggered: {
            if (root.pendingCommand < 0) return
            var command = root.pendingCommand
            root.pendingCommand = -1
            var jumped = command === root._cmdSetMissionCurrent && root._dmw && root._jumpReached(root._dmw.missionCurrentSeq)
            root._pendingJumpSeq = -1
            if (!jumped) root._warnFailure(command, -2)
        }
    }

    Timer {
        id: nextConfirmTimer
        interval: 4000
        onTriggered: root.nextConfirmPending = false
    }

    Connections {
        target: root._dmw
        ignoreUnknownSignals: true
        function onAutopilotCommandAcked(command, result) {
            if (command !== root.pendingCommand) return
            if (result === 5) {
                ackTimer.restart()
                return
            }
            root.pendingCommand = -1
            root._pendingJumpSeq = -1
            ackTimer.stop()
            if (result !== 0)
                root._warnFailure(command, result)
        }
        function onVruChanged() {
            if (root.pendingCommand !== root._cmdSetMissionCurrent || !root._jumpReached(root._dmw.missionCurrentSeq)) return
            root.pendingCommand = -1
            root._pendingJumpSeq = -1
            ackTimer.stop()
        }
        function onAutopilotStatusText(severity, text) {
            if (!root.popupVisible && severity <= root._severityCritical)
                root._toast(qsTr("Vehicle: %1").arg(String(text).replace(/</g, "‹").replace(/>/g, "›")))
        }
        function onAutopilotAppMessage(severity, text) {
            if (!root.popupVisible)
                root._toast(text)
        }
    }

    component InfoCell: Column {
        property string key: ""
        property string value: ""
        property color valueColor: AppPalette.textStrong
        spacing: 0
        Text {
            text: parent.key
            color: AppPalette.textMuted
            font.pixelSize: Math.round(10 * root._s)
        }
        Text {
            text: parent.value
            color: parent.valueColor
            font.pixelSize: Math.round(12 * root._s)
            font.bold: true
        }
    }

    component PillButton: KCircleIconButton {
        property bool selected: false
        property bool danger: false
        width: root._controlH
        height: root._controlH
        enabled: root._online
        opacity: enabled ? 1.0 : 0.5
        iconTintColor: danger ? AppPalette.dangerText : AppPalette.text
        fillColor:        danger ? AppPalette.dangerBg : (selected ? AppPalette.accentBgStrong : AppPalette.card)
        fillHoverColor:   danger ? AppPalette.dangerBg : (selected ? AppPalette.accentBorder : AppPalette.cardHover)
        fillPressedColor: AppPalette.bgDeep
        borderColor:      danger ? AppPalette.dangerBorder : (selected ? AppPalette.accentBorder : AppPalette.border)
        borderHoverColor: danger ? AppPalette.dangerBorder : (selected ? AppPalette.accentBorder : AppPalette.borderHover)
        borderWidth: danger || selected ? 1 : Tokens.cardBorderWidth
        toolTipOnHold: true
    }

    component StatCell: Row {
        spacing: Math.round(2 * root._s)
    }

    component GlyphSlot: Item {
        anchors.verticalCenter: parent.verticalCenter
        width: Math.round(14 * root._s)
        height: Math.round(13 * root._s)
    }

    component StatValue: Text {
        anchors.verticalCenter: parent.verticalCenter
        width: root._statValueW
        elide: Text.ElideRight
        color: AppPalette.textSecond
        font.pixelSize: Math.round(11 * root._s)
    }

    component StatIcon: Item {
        id: statIcon
        property url source
        property color tint: AppPalette.textSecond
        width: Math.round(13 * root._s)
        height: width

        Image {
            id: statIconImage
            anchors.fill: parent
            source: statIcon.source
            sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
            sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: false
        }
        ColorOverlay {
            anchors.fill: statIconImage
            source: statIconImage
            color: statIcon.tint
            smooth: true
        }
    }

    component BatteryGlyph: Item {
        id: battery
        property int percent: -1
        readonly property color ink: root._levelColor(percent)
        readonly property int stroke: Math.max(1, Math.round(root._s))
        width: Math.round(14 * root._s)
        height: Math.round(8 * root._s)

        Rectangle {
            id: batteryBody
            width: parent.width - batteryCap.width
            height: parent.height
            radius: Math.round(2 * root._s)
            color: "transparent"
            border.width: battery.stroke
            border.color: battery.ink

            Rectangle {
                readonly property int inset: battery.stroke * 2
                x: inset
                y: inset
                height: parent.height - inset * 2
                width: Math.round((parent.width - inset * 2) * Math.max(0, Math.min(100, battery.percent)) / 100)
                visible: battery.percent > 0
                radius: 1
                color: battery.ink
            }
        }
        Rectangle {
            id: batteryCap
            anchors.left: batteryBody.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(1, Math.round(2 * root._s))
            height: Math.round(parent.height * 0.45)
            radius: 1
            color: battery.ink
        }
    }

    component SignalBars: Row {
        id: signalBars
        property int percent: -1
        readonly property int lit: percent <= 0 ? 0 : Math.ceil(percent / 25)
        readonly property int barH: Math.round(11 * root._s)
        spacing: Math.max(1, Math.round(1 * root._s))

        Repeater {
            model: 4
            delegate: Rectangle {
                required property int index
                anchors.bottom: parent.bottom
                width: Math.max(2, Math.round(2 * root._s))
                height: Math.round(signalBars.barH * (index + 1) / 4)
                radius: 1
                color: index < signalBars.lit ? root._levelColor(signalBars.percent) : AppPalette.border
            }
        }
    }

    component PillSeparator: Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: 1
        height: Math.round(root._controlH * 0.6)
        color: AppPalette.border
    }

    Item {
        id: cardBody
        anchors.fill: parent

        Rectangle {
            width: parent.width
            height: root._pillH + (root._trayVisible ? tray.height : 0)
            radius: root._pillH / 2
            color: AppPalette.bg
        }

        Rectangle {
            id: pill
            width: parent.width
            height: root._pillH
            radius: height / 2
            color: "transparent"
            border.width: 0

            MouseArea {
                anchors.left: parent.left
                anchors.right: buttonRow.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                enabled: root._online
                cursorShape: Qt.PointingHandCursor
                onClicked: root.expanded = !root.expanded
            }

            Item {
                id: statusDot
                x: root._pad
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(20 * root._s)
                height: width

                Image {
                    id: statusIcon
                    anchors.fill: parent
                    source: root._online ? "qrc:/icons/ui/autopilot-link.svg" : "qrc:/icons/ui/autopilot-link-off.svg"
                    sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
                    sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    visible: false
                }
                ColorOverlay {
                    anchors.fill: statusIcon
                    source: statusIcon
                    color: !root._online ? AppPalette.textMuted
                         : root._armed ? AppPalette.dangerBorder : AppPalette.missionRally
                    smooth: true
                }

                MouseArea {
                    id: statusTipArea
                    anchors.fill: parent
                    anchors.margins: -Math.round(6 * root._s)
                    enabled: !root._online
                    hoverEnabled: true
                    onClicked: statusTipTimer.restart()
                }
                Timer {
                    id: statusTipTimer
                    interval: 2000
                }
                KToolTip {
                    text: qsTr("No autopilot")
                    targetItem: statusDot
                    delay: statusTipTimer.running ? 0 : 520
                    shown: !root._online && (statusTipArea.containsMouse || statusTipTimer.running)
                }
            }

            Column {
                id: statValueProbe
                opacity: 0
                enabled: false

                Repeater {
                    model: root._statTemplates
                    delegate: Text {
                        required property string modelData
                        text: modelData
                        font.pixelSize: Math.round(11 * root._s)
                    }
                }
            }

            Text {
                id: statLastValueProbe
                opacity: 0
                enabled: false
                text: "100 %"
                font.pixelSize: Math.round(11 * root._s)
            }

            Column {
                id: modeNameProbe
                opacity: 0
                enabled: false

                Repeater {
                    model: root._modeNamesForWidth
                    delegate: Text {
                        required property string modelData
                        text: modelData
                        font: titleText.font
                    }
                }
            }

            Row {
                id: infoColumn
                anchors.left: statusDot.right
                anchors.leftMargin: root._online ? Math.round(8 * root._s) : 0
                anchors.verticalCenter: parent.verticalCenter
                visible: root._online
                width: root._online ? implicitWidth : 0
                spacing: Math.round(8 * root._s)

                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    width: root._titleSlotW + titleRow.spacing + chevron.width
                    height: titleRow.height

                    Row {
                        id: titleRow
                        spacing: Math.round(6 * root._s)

                        Text {
                            id: titleText
                            anchors.verticalCenter: parent.verticalCenter
                            text: root._modeName
                            color: AppPalette.textStrong
                            font.pixelSize: Math.round(13 * root._s)
                            font.bold: true
                        }
                        KCircleIconButton {
                            id: chevron
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.round(24 * root._s)
                            height: width
                            iconSource: "qrc:/icons/ui/chevron-down.svg"
                            iconRotation: root.expanded ? 180 : 0
                            iconTintColor: AppPalette.text
                            fillColor: AppPalette.card
                            fillHoverColor: AppPalette.cardHover
                            fillPressedColor: AppPalette.bgDeep
                            borderColor: AppPalette.border
                            borderHoverColor: AppPalette.borderHover
                            onClicked: root.expanded = !root.expanded
                        }
                    }
                }

                Grid {
                    anchors.verticalCenter: parent.verticalCenter
                    rows: 2
                    flow: Grid.TopToBottom
                    columnSpacing: Math.round(6 * root._s)
                    rowSpacing: Math.round(3 * root._s)
                    verticalItemAlignment: Grid.AlignVCenter

                    StatCell {
                        GlyphSlot { BatteryGlyph { anchors.centerIn: parent; percent: root._batteryPct } }
                        StatValue {
                            text: root._batteryPct >= 0 ? root._batteryPct + " %" : "—"
                            color: root._levelColor(root._batteryPct)
                        }
                    }
                    StatCell {
                        GlyphSlot { StatIcon { anchors.centerIn: parent; source: "qrc:/icons/ui/stat-speed.svg" } }
                        StatValue {
                            text: isFinite(root._speed) ? root._speed.toFixed(1) + " " + qsTr("m/s") : "—"
                        }
                    }
                    StatCell {
                        GlyphSlot { StatIcon { anchors.centerIn: parent; source: "qrc:/icons/ui/stat-voltage.svg" } }
                        StatValue {
                            text: isFinite(root._voltage) ? root._voltage.toFixed(1) + " V" : "—"
                        }
                    }
                    StatCell {
                        GlyphSlot { StatIcon { anchors.centerIn: parent; source: "qrc:/icons/ui/stat-current.svg" } }
                        StatValue {
                            text: isFinite(root._current) ? root._current.toFixed(1) + " A" : "—"
                        }
                    }
                    StatCell {
                        GlyphSlot { SignalBars { anchors.centerIn: parent; percent: root._linkQuality } }
                        StatValue {
                            width: root._statLastValueW
                            text: root._linkQuality >= 0 ? root._linkQuality + " %" : "—"
                            color: root._levelColor(root._linkQuality)
                        }
                    }
                    StatCell {
                        GlyphSlot {
                            StatIcon {
                                anchors.centerIn: parent
                                source: "qrc:/icons/ui/satellite.svg"
                                tint: root._gpsPoor ? AppPalette.linkIdleText : AppPalette.textSecond
                            }
                        }
                        StatValue {
                            width: root._statLastValueW
                            text: root._gpsSats >= 0 ? String(root._gpsSats) : "—"
                            color: root._gpsPoor ? AppPalette.linkIdleText : AppPalette.textSecond
                        }
                    }
                }
            }

            Row {
                id: buttonRow
                anchors.left: infoColumn.right
                anchors.leftMargin: Math.round(10 * root._s)
                anchors.verticalCenter: parent.verticalCenter
                spacing: root._gap

                PillButton {
                    id: armBtn
                    danger: root._armed || root.armConfirmPending
                    highlighted: root.armConfirmPending
                    flashToken: root._armFlash
                    highlightBorderColor: AppPalette.dangerBorder
                    autoToolTip: false
                    onClicked: root.requestArm()

                    Text {
                        anchors.centerIn: parent
                        scale: armBtn.contentScale
                        text: root.armConfirmPending ? "ARM\n" + Math.max(1, Math.ceil(root._armCountdown * root._armConfirmMs / 1000))
                                                     : root._armed ? "DIS\nARM" : "ARM"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        lineHeight: 0.85
                        color: armBtn.danger ? AppPalette.dangerText : AppPalette.textStrong
                        font.pixelSize: Math.round((root._armed || root.armConfirmPending ? 10 : 12) * root._s)
                        font.weight: Font.Black
                    }

                    Shape {
                        id: armCountdownRing
                        readonly property real stroke: Math.max(2, Math.round(3 * root._s))
                        anchors.fill: parent
                        scale: armBtn.backgroundScale
                        visible: root.armConfirmPending && root._armCountdown > 0.001
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            strokeColor: AppPalette.dangerText
                            strokeWidth: armCountdownRing.stroke
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap

                            PathAngleArc {
                                centerX: armBtn.width / 2
                                centerY: armBtn.height / 2
                                radiusX: (Math.min(armBtn.width, armBtn.height) - armCountdownRing.stroke) / 2
                                radiusY: radiusX
                                startAngle: -90
                                sweepAngle: 360 * root._armCountdown
                            }
                        }
                    }
                }

                PillSeparator {}

                Repeater {
                    model: root.modes
                    delegate: Row {
                        id: modeCell
                        required property var modelData
                        required property int index
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: root._gap

                        PillSeparator {
                            visible: modeCell.index > 0 && root.modes[modeCell.index - 1].group !== modeCell.modelData.group
                        }

                        PillButton {
                            selected: root._online && root._mode === modeCell.modelData.mode
                            toolTipOnHold: modeCell.modelData.stop !== true
                            iconSource: modeCell.modelData.icon
                            toolTipText: modeCell.modelData.tip
                            onClicked: root.requestMode(modeCell.modelData.offMode !== undefined && root._mode === modeCell.modelData.mode
                                                        ? modeCell.modelData.offMode : modeCell.modelData.mode)
                        }
                    }
                }

                PillSeparator {}

                KCircleIconButton {
                    width: root._controlH
                    height: root._controlH
                    toolTipOnHold: true
                    iconSource: "qrc:/icons/ui/map_route.svg"
                    iconTintColor: AppPalette.text
                    fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover
                    fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border
                    borderHoverColor: AppPalette.borderHover
                    toolTipText: qsTr("Mission editor")
                    onClicked: if (typeof core !== "undefined" && core) core.setMissionEditorActive(true)
                }

                KCircleIconButton {
                    width: root._controlH
                    height: root._controlH
                    iconSource: "qrc:/icons/ui/x.svg"
                    iconTintColor: AppPalette.text
                    fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover
                    fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border
                    borderHoverColor: AppPalette.borderHover
                    onClicked: root.store.autopilotPopupOpen = false
                }
            }
        }

        Rectangle {
            id: tray
            visible: root._trayVisible
            anchors.top: pill.bottom
            width: parent.width
            height: trayColumn.implicitHeight + root._rowPad * 2
            color: "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._pad
                anchors.rightMargin: root._pad
                height: 1
                color: AppPalette.border
            }

            Column {
                id: trayColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: root._pad
                anchors.rightMargin: root._pad
                anchors.topMargin: root._rowPad
                spacing: Math.round(8 * root._s)

                Item {
                    width: parent.width + root._pad - root._sidePad
                    height: root._controlH
                    visible: root._helmRowVisible

                    KGamepadIcon {
                        id: helmPadIcon
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(30 * root._s)
                        height: width
                        family: root._gamepad ? root._gamepad.family : ""
                        color: root._helmActive ? AppPalette.accent : AppPalette.textStrong
                    }

                    Text {
                        anchors.left: helmPadIcon.right
                        anchors.leftMargin: root._gap
                        anchors.right: helmBars.visible ? helmBars.left
                                     : helmSettingsButton.visible ? helmSettingsButton.left : helmButton.left
                        anchors.rightMargin: root._gap
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            if (!root._helm) return ""
                            switch (root._helm.state) {
                            case 1: return qsTr("Checking vehicle parameters…")
                            case 2: return qsTr("Switching to Manual…")
                            case 3: return root._helm.background ? qsTr("Gamepad control · background") : qsTr("Gamepad control")
                            case 4: return qsTr("Holding position…")
                            default: return qsTr("Gamepad: %1").arg(root._gamepad ? root._gamepad.name : "")
                            }
                        }
                        color: AppPalette.textSecond
                        font.pixelSize: Math.round(11 * root._s)
                        elide: Text.ElideRight
                    }

                    Row {
                        id: helmBars
                        anchors.right: helmSettingsButton.visible ? helmSettingsButton.left : helmButton.left
                        anchors.rightMargin: root._gap
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !!root._helm && root._helm.state === 3

                        Grid {
                            anchors.verticalCenter: parent.verticalCenter
                            columns: 2
                            columnSpacing: Math.round(6 * root._s)
                            rowSpacing: Math.round(5 * root._s)
                            verticalItemAlignment: Grid.AlignVCenter

                            Text {
                                text: qsTr("Throttle")
                                color: AppPalette.textSecond
                                font.pixelSize: Math.round(11 * root._s)
                            }
                            HelmBar {
                                width: Math.round(110 * root._s)
                                height: Math.round(9 * root._s)
                                value: root._helm ? root._helm.throttle : 0
                            }
                            Text {
                                text: qsTr("Turn")
                                color: AppPalette.textSecond
                                font.pixelSize: Math.round(11 * root._s)
                            }
                            HelmBar {
                                width: Math.round(110 * root._s)
                                height: Math.round(9 * root._s)
                                value: root._helm ? root._helm.steering : 0
                            }
                        }
                    }

                    PillButton {
                        id: helmSettingsButton
                        anchors.right: helmButton.left
                        anchors.rightMargin: root._gap
                        anchors.verticalCenter: parent.verticalCenter
                        visible: typeof theme !== "undefined" && !!theme && theme.instrumentsGrade >= 1
                        iconSource: "qrc:/icons/ui/settings.svg"
                        toolTipText: qsTr("Gamepad settings")
                        onClicked: if (root.store) root.store.openAppSettingsAtGroup("app.gamepadhelm")
                    }

                    PillButton {
                        id: helmButton
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        danger: root._helmActive
                        toolTipOnHold: false
                        iconSource: root._helmActive ? "qrc:/icons/ui/helm-release.svg" : "qrc:/icons/ui/helm-take.svg"
                        toolTipText: root._helmActive ? qsTr("Release control") : qsTr("Take control")
                        onClicked: {
                            if (!root._helm) return
                            if (root._helmActive) root._helm.disengage()
                            else root._helm.engage()
                        }
                    }
                }

                Column {
                    width: parent.width
                    visible: root._helmRowVisible && root._helmConfirmation !== 0
                    spacing: Math.round(6 * root._s)

                    Text {
                        width: parent.width
                        text: root._helmConfirmation === 1
                              ? qsTr("If the link drops, the transmitter takes over")
                              : qsTr("The boat does not stop when the link drops")
                        color: AppPalette.textStrong
                        font.pixelSize: Math.round(12 * root._s)
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: root._helmConfirmation === 1
                              ? qsTr("%1 s after the last gamepad command the boat follows the transmitter sticks again. Keep the transmitter on.")
                                    .arg(root._helm ? root._helm.overrideSeconds : 3)
                              : qsTr("Without a transmitter the boat would keep the last gamepad command. Turn on stopping on link loss? The boat holds; missions in Auto go on. The setting is saved on the boat.")
                        color: AppPalette.textSecond
                        font.pixelSize: Math.round(12 * root._s)
                        wrapMode: Text.WordWrap
                    }

                    Item {
                        width: parent.width
                        height: Tokens.controlHMd

                        KButton {
                            id: helmConfirmButton
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            height: Tokens.controlHMd
                            enabled: !!root._helm && !root._helm.confirmationBusy
                            text: root._helmConfirmation === 1 ? qsTr("Transmitter is on — start")
                                : (root._helm && root._helm.confirmationBusy) ? qsTr("Saving…")
                                : qsTr("Turn on and start")
                            onClicked: if (root._helm) root._helm.confirm()
                        }

                        KButton {
                            anchors.right: helmConfirmButton.left
                            anchors.rightMargin: root._gap
                            anchors.verticalCenter: parent.verticalCenter
                            height: Tokens.controlHMd
                            text: qsTr("Cancel")
                            onClicked: if (root._helm) root._helm.cancelConfirmation()
                        }
                    }
                }

                Item {
                    width: parent.width
                    height: progressRow.implicitHeight
                    visible: root._missionRowVisible

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.expanded = !root.expanded
                    }

                    Column {
                        id: progressRow
                        width: parent.width
                        spacing: Math.round(4 * root._s)

                        Text {
                            width: parent.width
                            text: root._missionRowText
                            color: AppPalette.textStrong
                            font.pixelSize: Math.round(12 * root._s)
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            width: parent.width
                            height: Math.round(4 * root._s)
                            radius: height / 2
                            color: AppPalette.trackOff
                            visible: !!root._run && root._run.progress >= 0

                            Rectangle {
                                width: parent.width * (root._run ? Math.max(0, Math.min(1, root._run.progress)) : 0)
                                height: parent.height
                                radius: parent.radius
                                color: root._paused ? AppPalette.textMuted : AppPalette.accentBar
                                Behavior on width { NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing } }
                            }
                        }
                    }
                }

                Column {
                    width: parent.width
                    visible: root.expanded
                    spacing: Math.round(6 * root._s)

                    Text {
                        width: parent.width
                        visible: root._missionKnown && !!root._run && root._run.currentLabel.length > 0
                        textFormat: Text.StyledText
                        text: {
                            if (!root._run) return ""
                            var line = "<b>" + root._run.currentLabel + "</b>"
                            if (root._nextSeq >= 0) line += "  →  " + root._run.labelForSeq(root._nextSeq)
                            return line
                        }
                        color: AppPalette.textStrong
                        font.pixelSize: Math.round(12 * root._s)
                        elide: Text.ElideRight
                    }

                    Flow {
                        width: parent.width
                        spacing: Math.round(12 * root._s)

                        InfoCell {
                            visible: root._missionKnown
                            key: qsTr("To point")
                            value: root._dmw && root._dmw.navigationValid ? root._fmtDist(root._dmw.waypointDistance) : "—"
                        }
                        InfoCell {
                            visible: root._missionKnown
                            key: qsTr("Cross-track")
                            value: root._dmw && root._dmw.navigationValid && isFinite(root._dmw.crossTrackError)
                                   ? qsTr("%1 m").arg(Math.abs(root._dmw.crossTrackError).toFixed(1)) : "—"
                        }
                        InfoCell {
                            key: qsTr("GPS")
                            valueColor: root._gpsPoor ? AppPalette.linkIdleText : AppPalette.textStrong
                            value: {
                                if (root._gpsFix < 0) return "—"
                                var parts = [root._gpsFixName(root._gpsFix)]
                                if (root._gpsSats >= 0) parts.push(qsTr("%1 sat.").arg(root._gpsSats))
                                if (isFinite(root._gpsHdop)) parts.push("HDOP " + root._gpsHdop.toFixed(1))
                                return parts.join(" · ")
                            }
                        }
                        InfoCell {
                            key: qsTr("To home")
                            value: root._fmtDist(root._homeDistance)
                        }
                        InfoCell {
                            visible: root._rssiValid
                            key: "RSSI"
                            value: qsTr("%1 dBm").arg(root._rssi)
                        }
                    }

                    Text {
                        width: parent.width
                        visible: text.length > 0 && !root._sourceIsPlan
                        text: root._missionSourceText
                        color: AppPalette.textSecond
                        font.pixelSize: Math.round(11 * root._s)
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        spacing: Math.round(6 * root._s)

                        KButton {
                            visible: root._missionKnown && root._nextSeq >= 0
                            height: Tokens.controlHMd
                            danger: root.nextConfirmPending
                            text: root.nextConfirmPending ? qsTr("Tap again to skip") : qsTr("Next point")
                            onClicked: root.requestNextPoint()
                        }
                        KButton {
                            visible: !!root._run && !root._run.reading
                            height: Tokens.controlHMd
                            text: qsTr("Read again")
                            toolTipText: qsTr("Read the mission from the vehicle again")
                            onClicked: if (root._run) root._run.refresh()
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: Math.round(2 * root._s)

                        Repeater {
                            model: root._dmw ? root._dmw.autopilotMessages.slice(0, root._messagesShown) : []
                            delegate: Text {
                                required property var modelData
                                width: parent.width
                                textFormat: Text.PlainText
                                text: Qt.formatTime(new Date(modelData.time), "hh:mm:ss") + "  " + modelData.text
                                color: root._severityColor(modelData.severity)
                                font.pixelSize: Math.round(11 * root._s)
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }
}

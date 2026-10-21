import QtQuick 2.15
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
    readonly property int _severityWarning: 4

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
    property bool expanded: false
    readonly property bool _trayVisible: _online && (expanded || _missionRowVisible)
    readonly property int _rowPad: _trayVisible ? Math.round(6 * _s) : _sidePad
    readonly property int _trayPad: Math.round(10 * _s)
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
        if (typeof notifications !== "undefined" && notifications)
            notifications.info(qsTr("Going to %1").arg(_run.labelForSeq(_nextSeq)))
    }

    readonly property string _powerText: {
        var parts = []
        if (_batteryPct >= 0) parts.push(qsTr("Bat. %1 %").arg(_batteryPct))
        if (isFinite(_voltage)) parts.push(_voltage.toFixed(1) + " V")
        if (isFinite(_current)) parts.push(_current.toFixed(1) + " A")
        return parts.join(" · ")
    }
    property real _powerW: 0
    readonly property var _modeNamesForWidth: ["Manual", "Hold", "Loiter", "Auto", "RTL", "SmartRTL", "Guided", "Acro", "Steering"]
    readonly property real _modeSlotW: modeNameProbe.width
    readonly property real _titleSlotW: _online ? _modeSlotW : titleText.implicitWidth

    readonly property string _statsText: {
        var parts = []
        if (_linkQuality >= 0) parts.push(qsTr("Link %1 %").arg(_linkQuality))
        if (_gpsFix >= 0) {
            var gps = _gpsSats >= 0 && _gpsFix >= _gpsFix3d ? qsTr("Sat. %1").arg(_gpsSats) : _gpsFixName(_gpsFix)
            parts.push(_gpsPoor ? "<font color=\"" + AppPalette.linkIdleText + "\">" + gps + "</font>" : gps)
        }
        if (isFinite(_speed)) parts.push(_speed.toFixed(1) + " " + qsTr("m/s"))
        if (_rssiValid) parts.push(qsTr("%1 dBm").arg(_rssi))
        return parts.join(" · ")
    }
    property real _statsW: 0

    readonly property var modes: [
        { mode: 0,  icon: "qrc:/icons/ui/mode-manual.svg", tip: qsTr("Manual") },
        { mode: 4,  icon: "qrc:/icons/ui/mode-hold.svg",   tip: qsTr("Hold") },
        { mode: 5,  icon: "qrc:/icons/ui/mode-loiter.svg", tip: qsTr("Loiter") },
        { mode: 10, icon: "qrc:/icons/ui/mode-auto.svg",   tip: qsTr("Auto") },
        { mode: 11, icon: "qrc:/icons/ui/mode-rtl.svg",    tip: qsTr("RTL") }
    ]

    property bool armConfirmPending: false
    property real _armFirstTapMs: 0
    property int _armFlash: 0
    property int pendingCommand: -1

    readonly property real _pillW: _pad + statusDot.width + Math.round(8 * _s) + infoColumn.width + _gap * 2 + buttonRow.width + _sidePad
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
        if (typeof notifications === "undefined" || !notifications) return
        notifications.info(qsTr("%1: %2").arg(commandText(command)).arg(resultText(result)))
    }

    on_WantWChanged: _applySize()
    on_WantHChanged: { _applySize(); Qt.callLater(_restorePlacement) }
    onInteractionStarted: _dragging = true
    on_OnlineChanged: if (!_online) { _cancelArmConfirm(); _cancelNextConfirm(); _statsW = 0; _powerW = 0; expanded = false; _pendingJumpSeq = -1 }
    on_NextSeqChanged: _cancelNextConfirm()
    onExpandedChanged: _cancelNextConfirm()
    on_ArmedChanged: _cancelArmConfirm()
    onArmConfirmPendingChanged: if (armConfirmPending) _armFlash += 1

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
        interval: 4000
        onTriggered: root.armConfirmPending = false
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
            if (severity <= root._severityWarning && typeof notifications !== "undefined" && notifications)
                notifications.info(qsTr("Vehicle: %1").arg(String(text).replace(/</g, "‹").replace(/>/g, "›")))
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

            Rectangle {
                id: statusDot
                x: root._pad
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(8 * root._s)
                height: width
                radius: width / 2
                color: !root._online ? AppPalette.textMuted
                     : root._armed ? AppPalette.dangerBorder : AppPalette.missionRally
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

            Column {
                id: infoColumn
                anchors.left: statusDot.right
                anchors.leftMargin: Math.round(8 * root._s)
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(Math.round(300 * root._s),
                                Math.ceil(Math.max(Math.round(84 * root._s),
                                                   root._titleSlotW
                                                   + (chevron.visible ? titleRow.spacing + chevron.width : 0)
                                                   + (powerText.visible ? titleRow.spacing + powerText.leftPadding + root._powerW : 0),
                                                   root._statsW)))

                Row {
                    id: titleRow
                    width: parent.width
                    spacing: Math.round(4 * root._s)

                    Text {
                        id: titleText
                        width: Math.min(root._titleSlotW, titleRow.width - (chevron.visible ? titleRow.spacing + chevron.width : 0))
                        text: root.armConfirmPending ? qsTr("Tap again to arm")
                            : !root._online ? qsTr("No autopilot")
                            : root._modeName
                        color: root.armConfirmPending ? AppPalette.dangerText : AppPalette.textStrong
                        font.pixelSize: Math.round(13 * root._s)
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Text {
                        id: chevron
                        anchors.verticalCenter: titleText.verticalCenter
                        visible: root._online
                        text: root.expanded ? "▴" : "▾"
                        color: AppPalette.textMuted
                        font.pixelSize: Math.round(11 * root._s)
                    }
                    Text {
                        id: powerText
                        anchors.baseline: titleText.baseline
                        visible: root._online && text.length > 0
                        width: Math.max(0, Math.min(implicitWidth, titleRow.width - titleText.width
                                                    - (chevron.visible ? titleRow.spacing + chevron.width : 0) - titleRow.spacing))
                        leftPadding: Math.round(4 * root._s)
                        text: root._powerText
                        color: AppPalette.textSecond
                        font.pixelSize: Math.round(11 * root._s)
                        elide: Text.ElideRight
                        onImplicitWidthChanged: if (visible) root._powerW = Math.max(root._powerW, implicitWidth - leftPadding)
                        onVisibleChanged: if (visible) root._powerW = Math.max(root._powerW, implicitWidth - leftPadding)
                    }
                }
                Text {
                    width: parent.width
                    visible: root._online && text.length > 0
                    text: root._statsText
                    textFormat: Text.StyledText
                    color: AppPalette.textSecond
                    font.pixelSize: Math.round(11 * root._s)
                    elide: Text.ElideRight
                    onImplicitWidthChanged: if (visible) root._statsW = Math.max(root._statsW, implicitWidth)
                    onVisibleChanged: if (visible) root._statsW = Math.max(root._statsW, implicitWidth)
                }
            }

            Row {
                id: buttonRow
                anchors.left: infoColumn.right
                anchors.leftMargin: root._gap * 2
                anchors.verticalCenter: parent.verticalCenter
                spacing: root._gap

                PillButton {
                    id: armBtn
                    danger: root._armed || root.armConfirmPending
                    highlighted: root.armConfirmPending
                    flashToken: root._armFlash
                    highlightBorderColor: AppPalette.dangerBorder
                    toolTipText: root._armed ? qsTr("Disarm") : qsTr("Arm (tap twice)")
                    onClicked: root.requestArm()

                    Text {
                        anchors.centerIn: parent
                        scale: armBtn.contentScale
                        text: root.armConfirmPending ? "ARM?" : root._armed ? "DIS\nARM" : "ARM"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        lineHeight: 0.85
                        color: armBtn.danger ? AppPalette.dangerText : AppPalette.textStrong
                        font.pixelSize: Math.round((root._armed ? 10 : 12) * root._s)
                        font.weight: Font.Black
                    }
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1
                    height: Math.round(root._controlH * 0.6)
                    color: AppPalette.border
                }

                Repeater {
                    model: root.modes
                    delegate: PillButton {
                        required property var modelData
                        selected: root._online && root._mode === modelData.mode
                        iconSource: modelData.icon
                        toolTipText: modelData.tip
                        onClicked: root.requestMode(modelData.mode)
                    }
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
            height: trayColumn.implicitHeight + root._trayPad * 2
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
                anchors.topMargin: root._trayPad
                spacing: Math.round(8 * root._s)

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

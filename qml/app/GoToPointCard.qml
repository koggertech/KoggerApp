import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var view: null
    property var store: null
    property bool hostsView: false

    readonly property var ctl: view ? view.missionRunController : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property var _core: (typeof core !== "undefined") ? core : null
    readonly property var _params: (typeof autopilotParams !== "undefined") ? autopilotParams : null
    readonly property var _run: (typeof missionRun !== "undefined") ? missionRun : null
    readonly property bool _online: _dmw ? _dmw.autopilotOnline : false
    readonly property bool _armed: _dmw ? _dmw.pilotArmState > 0 : false
    readonly property int _mode: _dmw ? _dmw.pilotModeState : -1
    readonly property bool _guided: _mode === _roverModeGuided
    readonly property bool _auto: _mode === _roverModeAuto
    readonly property bool _hasPick: !!ctl && ctl.goToPickValid
    readonly property bool _vehicleKnown: !!_dmw && isFinite(_dmw.vehicleLat) && isFinite(_dmw.vehicleLon)
                                          && _dmw.gpsFixType >= _gpsFix3d
    readonly property real _distance: (_hasPick && _vehicleKnown)
                                      ? _distanceM(_dmw.vehicleLat, _dmw.vehicleLon, ctl.goToPickLat, ctl.goToPickLon) : NaN
    readonly property real _targetDistance: (!!ctl && ctl.goToTargetValid && _vehicleKnown)
                                            ? _distanceM(_dmw.vehicleLat, _dmw.vehicleLon, ctl.goToTargetLat, ctl.goToTargetLon) : NaN
    readonly property int _ekfPosition: _dmw ? _dmw.ekfPositionState : -1
    readonly property bool _retarget: _guided && !!ctl && ctl.goToTargetValid
    readonly property string _blockReason: {
        if (!_online) return qsTr("No autopilot")
        if (!_armed) return qsTr("Arm the boat first")
        if (!_vehicleKnown) return qsTr("Boat position unknown")
        if (_ekfPosition === 0) return qsTr("No EKF position yet: wait for GPS")
        if (_distance > _maxDistanceM) return qsTr("Too far: limit %1").arg(_fmtDist(_maxDistanceM))
        return ""
    }
    readonly property bool _canSend: _blockReason.length === 0 && !_sending
    readonly property real _storedSpeed: store && store.goToSpeed > 0 ? Math.max(_speedMin, store.goToSpeed) : 0
    readonly property int _arrival: store ? store.goToArrival : _arrivalKeep
    readonly property bool _missionOnBoat: !!_run && _run.known && !_run.vehicleEmpty
    readonly property var _arrivalItems: [
        { value: _arrivalKeep, label: "Guided", enabled: true },
        { value: _arrivalMission, label: "Auto", enabled: _missionOnBoat },
        { value: _arrivalHome, label: "RTL", enabled: true },
        { value: _arrivalDrift, label: "Hold", enabled: true }
    ]
    readonly property bool _arrivalFallback: _arrival === _arrivalMission && !_missionOnBoat
    readonly property real _s: AppPalette.scale

    readonly property int _roverModeHold: 4
    readonly property int _roverModeLoiter: 5
    readonly property int _roverModeAuto: 10
    readonly property int _roverModeRtl: 11
    readonly property int _roverModeGuided: 15
    readonly property int _gpsFix3d: 3
    readonly property int _cmdSetMode: 176
    readonly property int _cmdChangeSpeed: 178
    readonly property int _cmdDoReposition: 192
    readonly property int _resultAccepted: 0
    readonly property int _resultInProgress: 5
    readonly property int _resultFailed: 4
    readonly property int _confirmMinMs: 400
    readonly property int _confirmWindowMs: 4000
    readonly property int _ackTimeoutMs: 3000
    readonly property int _guidedWaitMs: 4000
    readonly property real _maxDistanceM: 1000
    readonly property real _earthRadiusM: 6371008.8
    readonly property real _speedStep: 0.5
    readonly property real _speedMin: typeof missionPlan !== "undefined" && missionPlan ? missionPlan.warnSpeed : 0.8
    readonly property real _speedMax: 10
    readonly property real _speedFallback: 2.0
    readonly property int _arrivalKeep: 0
    readonly property int _arrivalMission: 1
    readonly property int _arrivalHome: 2
    readonly property int _arrivalDrift: 3

    property bool confirmPending: false
    property real _firstTapMs: 0
    property bool _sending: false
    property bool _lateAck: false
    property real _sentLat: NaN
    property real _sentLon: NaN
    property real _sentSpeed: 0
    property int _sentArrival: 0
    property bool _awaitGuided: false
    property real _wpSpeed: NaN
    property int _targetArrival: 0
    property bool _speedPending: false
    property bool _speedOverridden: false
    property int _modePending: -1
    property string _modePendingLabel: ""

    function _distanceM(lat1, lon1, lat2, lon2) {
        var r = Math.PI / 180
        var dLat = (lat2 - lat1) * r
        var dLon = (lon2 - lon1) * r
        var a = Math.sin(dLat / 2) * Math.sin(dLat / 2)
                + Math.cos(lat1 * r) * Math.cos(lat2 * r) * Math.sin(dLon / 2) * Math.sin(dLon / 2)
        return 2 * _earthRadiusM * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))
    }

    function _fmtDist(m) {
        if (!isFinite(m)) return ""
        return m < 1000 ? qsTr("%1 m").arg(Math.round(m)) : qsTr("%1 km").arg((m / 1000).toFixed(2))
    }

    function _fmtSpeed(v) {
        return qsTr("%1 m/s").arg(v.toFixed(1))
    }

    function _arrivalText(action) {
        switch (action) {
        case _arrivalMission: return qsTr("Then the mission continues")
        case _arrivalHome: return qsTr("Then the boat returns home")
        case _arrivalDrift: return qsTr("Then the boat stops and drifts")
        }
        return qsTr("Then the boat holds the point")
    }

    function _arrivalMode(action) {
        switch (action) {
        case _arrivalMission: return _roverModeAuto
        case _arrivalHome: return _roverModeRtl
        case _arrivalDrift: return _roverModeHold
        }
        return -1
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

    function _toast(text) {
        if (typeof notifications !== "undefined" && notifications)
            notifications.info(text)
    }

    function _log(text) {
        if (_dmw)
            _dmw.logAutopilotEvent(6, text)
    }

    function _finishSend() {
        _sending = false
        _lateAck = false
        ackTimer.stop()
    }

    function _refreshWpSpeed() {
        var v = _params ? parseFloat(_params.valueText("WP_SPEED")) : NaN
        _wpSpeed = isFinite(v) && v > 0 ? v : NaN
    }

    function _stepSpeed(direction) {
        if (!store) return
        var base = _storedSpeed > 0 ? _storedSpeed : (isFinite(_wpSpeed) ? _wpSpeed : _speedFallback)
        var grid = base / _speedStep
        var next = (direction > 0 ? Math.floor(grid + 1e-6) + 1 : Math.ceil(grid - 1e-6) - 1) * _speedStep
        store.goToSpeed = Math.max(_speedMin, Math.min(_speedMax, next))
    }

    function _sendMode(mode, label) {
        if (!_dmw || !_online) return
        _modePending = mode
        _modePendingLabel = label
        modeAckTimer.restart()
        _dmw.autopilotSetMode(mode)
    }

    function stopBoat() {
        if (ctl) ctl.clearGoToTargetSelection()
        _targetArrival = _arrivalKeep
        _sendMode(_roverModeLoiter, qsTr("Stop"))
    }

    function tapGo() {
        if (!ctl || !_dmw || !_hasPick || !_canSend) return
        if (!_retarget) {
            var now = Date.now()
            if (!confirmPending) {
                confirmPending = true
                _firstTapMs = now
                confirmTimer.restart()
                return
            }
            if (now - _firstTapMs < _confirmMinMs) return
        }
        confirmPending = false
        confirmTimer.stop()
        _sentLat = ctl.goToPickLat
        _sentLon = ctl.goToPickLon
        _sentSpeed = _storedSpeed
        _sentArrival = (_arrival === _arrivalMission && !_missionOnBoat) ? _arrivalKeep : _arrival
        _lateAck = false
        _sending = true
        ackTimer.restart()
        ctl.clearGoToPick()
        _dmw.autopilotGoTo(_sentLat, _sentLon)
    }

    on_HasPickChanged: {
        confirmPending = false
        if (_hasPick && !isFinite(_wpSpeed)) {
            _refreshWpSpeed()
            if (!isFinite(_wpSpeed) && _params && _online)
                _params.request(["WP_SPEED"])
        }
    }
    onHostsViewChanged: if (!hostsView) { confirmPending = false }
    on_OnlineChanged: {
        if (_online || !hostsView) return
        confirmPending = false
        _finishSend()
        _awaitGuided = false
        guidedTimer.stop()
        _wpSpeed = NaN
        _speedPending = false
        _speedOverridden = false
        _modePending = -1
        modeAckTimer.stop()
        if (_core) _core.setGoToPlacementArmed(false)
    }
    on_ModeChanged: {
        if (!hostsView || !ctl) return
        if (_guided) {
            _awaitGuided = false
            guidedTimer.stop()
            return
        }
        if (_mode >= 0)
            _speedOverridden = false
        if (_mode >= 0 && !_awaitGuided && ctl.goToTargetValid) {
            ctl.clearGoToTarget()
            _targetArrival = _arrivalKeep
        }
    }

    Timer {
        id: confirmTimer
        interval: root._confirmWindowMs
        onTriggered: root.confirmPending = false
    }

    Timer {
        id: ackTimer
        interval: root._ackTimeoutMs
        onTriggered: {
            if (!root._sending) return
            root._sending = false
            root._lateAck = true
            root._toast(qsTr("Go here: %1").arg(root.resultText(-2)))
        }
    }

    Timer {
        id: guidedTimer
        interval: root._guidedWaitMs
        onTriggered: {
            root._awaitGuided = false
            if (root.ctl && !root._guided)
                root.ctl.clearGoToTarget()
        }
    }

    Timer {
        id: modeAckTimer
        interval: root._ackTimeoutMs
        onTriggered: {
            if (root._modePending < 0) return
            root._modePending = -1
            root._toast(qsTr("%1: %2").arg(root._modePendingLabel).arg(root.resultText(-2)))
        }
    }

    Connections {
        target: root._params
        ignoreUnknownSignals: true
        function onListChanged() { root._refreshWpSpeed() }
        function onRequestFinished(missing) { root._refreshWpSpeed() }
        function onWriteConfirmed(name, value) { if (name === "WP_SPEED") root._refreshWpSpeed() }
    }

    Connections {
        target: root._dmw
        ignoreUnknownSignals: true
        function onAutopilotCommandAcked(command, result) {
            if (!root.hostsView) return
            if (command === root._cmdSetMode && root._modePending >= 0) {
                if (result === root._resultInProgress) return
                root._modePending = -1
                modeAckTimer.stop()
                if (result !== root._resultAccepted)
                    root._toast(qsTr("%1: %2").arg(root._modePendingLabel).arg(root.resultText(result)))
                return
            }
            if (command === root._cmdChangeSpeed && root._speedPending) {
                if (result === root._resultInProgress) return
                root._speedPending = false
                if (result !== root._resultAccepted)
                    root._toast(qsTr("Go here speed: %1").arg(root.resultText(result)))
                return
            }
            if (command !== root._cmdDoReposition || (!root._sending && !root._lateAck)) return
            if (result === root._resultInProgress) {
                if (root._sending)
                    ackTimer.restart()
                return
            }
            root._finishSend()
            if (result === root._resultFailed) {
                root._toast(qsTr("Go here failed: the boat could not enter Guided or take the point, usually because the autopilot has no position estimate yet (EKF/GPS)"))
                return
            }
            if (result !== root._resultAccepted) {
                root._toast(qsTr("Go here: %1").arg(root.resultText(result)))
                return
            }
            if (!root._guided) {
                root._awaitGuided = true
                guidedTimer.restart()
            }
            if (root.ctl)
                root.ctl.setGoToTarget(root._sentLat, root._sentLon)
            root._targetArrival = root._sentArrival
            if (root._sentSpeed > 0) {
                root._speedPending = true
                root._speedOverridden = true
                root._dmw.autopilotSetSpeed(root._sentSpeed)
            } else if (root._speedOverridden && isFinite(root._wpSpeed)) {
                root._speedPending = true
                root._speedOverridden = false
                root._dmw.autopilotSetSpeed(root._wpSpeed)
            }
            root._log(qsTr("Going to %1, %2").arg(root._sentLat.toFixed(6)).arg(root._sentLon.toFixed(6)))
        }
        function onAutopilotMissionItemReached(seq) {
            if (!root.hostsView || seq !== 0 || !root._guided || !root.ctl || !root.ctl.goToTargetValid || root.ctl.goToArrived) return
            root.ctl.setGoToArrived(true)
            root._toast(qsTr("The boat reached the point"))
            var mode = root._arrivalMode(root._targetArrival)
            root._targetArrival = root._arrivalKeep
            if (mode >= 0)
                root._sendMode(mode, qsTr("After arrival"))
        }
    }

    component CardFrame: Rectangle {
        property real anchorX: 0
        property real anchorY: 0
        property bool alert: false
        readonly property real gapPx: Math.round(22 * root._s)
        width: Math.round(270 * root._s)
        x: Math.max(Tokens.spaceMd, Math.min(root.width - width - Tokens.spaceMd, anchorX - width / 2))
        y: anchorY - height - gapPx >= Tokens.spaceMd ? anchorY - height - gapPx
                                                      : Math.min(root.height - height - Tokens.spaceMd, anchorY + gapPx)
        radius: Tokens.radiusLg
        color: AppPalette.card
        border.width: 1
        border.color: alert ? AppPalette.dangerBorder : AppPalette.border

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: function(w) { w.accepted = true }
        }
    }

    component CardTitle: Item {
        property alias text: titleLabel.text
        property alias detail: titleDetail.text
        signal closeClicked()
        width: parent ? parent.width : 0
        height: Math.max(titleLabel.implicitHeight, titleClose.height)

        Text {
            id: titleLabel
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, titleClose.x - Tokens.spaceSm)
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            id: titleDetail
            anchors.left: titleLabel.right
            anchors.leftMargin: Tokens.spaceSm
            anchors.right: titleClose.left
            anchors.rightMargin: Tokens.spaceSm
            anchors.baseline: titleLabel.baseline
            visible: text.length > 0
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            elide: Text.ElideRight
        }

        KCircleIconButton {
            id: titleClose
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Tokens.controlHMd
            height: Tokens.controlHMd
            glyph: "×"
            glyphPixelSize: Math.round(14 * root._s)
            fillColor: "transparent"
            fillHoverColor: AppPalette.cardHover
            borderColor: "transparent"
            onClicked: parent.closeClicked()
        }
    }

    component StepButton: KCircleIconButton {
        width: Tokens.controlHMd
        height: Tokens.controlHMd
        glyphPixelSize: Math.round(14 * root._s)
        fillColor: AppPalette.card
        fillHoverColor: AppPalette.cardHover
        borderColor: AppPalette.border
    }

    component ArrivalSegments: Rectangle {
        id: segments
        property var items: []
        property int current: 0
        signal picked(int value)
        readonly property int inset: Math.max(2, Math.round(2 * root._s))
        width: parent ? parent.width : 0
        height: Tokens.controlHMd
        radius: Tokens.radiusMd
        color: AppPalette.card
        border.color: AppPalette.border
        border.width: Tokens.cardBorderWidth

        Row {
            x: segments.inset
            y: segments.inset

            Repeater {
                model: segments.items
                delegate: Rectangle {
                    id: segment
                    required property var modelData
                    readonly property bool selected: modelData.value === segments.current
                    width: (segments.width - segments.inset * 2) / Math.max(1, segments.items.length)
                    height: segments.height - segments.inset * 2
                    radius: Math.max(2, segments.radius - segments.inset)
                    opacity: modelData.enabled || selected ? 1.0 : 0.4
                    color: selected ? AppPalette.accentBgStrong
                         : segmentTap.containsMouse && segmentTap.enabled ? AppPalette.cardHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: segment.modelData.label
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontSm
                        font.bold: true
                    }
                    MouseArea {
                        id: segmentTap
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: segment.modelData.enabled && !segment.selected
                        cursorShape: Qt.PointingHandCursor
                        onClicked: segments.picked(segment.modelData.value)
                    }
                }
            }
        }
    }

    CardFrame {
        id: card
        visible: root.hostsView && root._hasPick && root.ctl.goToPickOnScreen
        anchorX: root.ctl ? root.ctl.goToPickScreen.x : 0
        anchorY: root.ctl ? root.ctl.goToPickScreen.y : 0
        alert: root.confirmPending
        height: content.implicitHeight + Tokens.spaceMd * 2

        Column {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Tokens.spaceMd
            spacing: Tokens.spaceSm

            CardTitle {
                text: qsTr("Go here")
                detail: isFinite(root._distance) ? qsTr("%1 from the boat").arg(root._fmtDist(root._distance)) : ""
                onCloseClicked: if (root.ctl) root.ctl.clearGoToPick()
            }

            Item {
                width: parent.width
                height: Tokens.controlHMd

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Speed")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontSm
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Tokens.spaceSm

                    StepButton {
                        glyph: "−"
                        onClicked: root._stepSpeed(-1)
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(72 * root._s)
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: root._storedSpeed > 0 ? root._fmtSpeed(root._storedSpeed)
                                : (isFinite(root._wpSpeed) ? root._fmtSpeed(root._wpSpeed) : "—")
                            color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontBase
                            font.bold: true
                        }
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            visible: root._storedSpeed <= 0
                            text: qsTr("boat default")
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm
                        }
                    }
                    StepButton {
                        glyph: "+"
                        onClicked: root._stepSpeed(1)
                    }
                    StepButton {
                        glyph: "↺"
                        toolTipOnHold: true
                        enabled: root._storedSpeed > 0
                        opacity: enabled ? 1.0 : 0.4
                        toolTipText: qsTr("Use the boat default speed (WP_SPEED)")
                        onClicked: if (root.store) root.store.goToSpeed = 0
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Tokens.spaceXs

                Text {
                    text: qsTr("On arrival")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontSm
                }

                ArrivalSegments {
                    items: root._arrivalItems
                    current: root._arrival
                    onPicked: function(value) { if (root.store) root.store.goToArrival = value }
                }

                Text {
                    width: parent.width
                    text: root._arrivalFallback ? qsTr("No mission on the boat: it will hold the point")
                                                : root._arrivalText(root._arrival)
                    color: root._arrivalFallback ? AppPalette.linkIdleText : AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                width: parent.width
                visible: root._blockReason.length > 0
                text: root._blockReason
                color: AppPalette.dangerText
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                visible: root._blockReason.length === 0 && root._auto
                text: qsTr("The boat switches to Guided and the mission pauses; Auto continues it")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }

            KButton {
                width: parent.width
                height: Tokens.controlHMd
                enabled: root._canSend
                danger: root.confirmPending
                text: root.confirmPending ? qsTr("Tap again to go") : qsTr("Go here")
                onClicked: root.tapGo()
            }
        }
    }

    CardFrame {
        id: targetCard
        visible: root.hostsView && !!root.ctl && root.ctl.goToTargetSelected && root.ctl.goToTargetOnScreen
        anchorX: root.ctl ? root.ctl.goToTargetScreen.x : 0
        anchorY: root.ctl ? root.ctl.goToTargetScreen.y : 0
        height: targetContent.implicitHeight + Tokens.spaceMd * 2

        Column {
            id: targetContent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Tokens.spaceMd
            spacing: Tokens.spaceSm

            CardTitle {
                text: root.ctl && root.ctl.goToArrived ? qsTr("The boat is at the point") : qsTr("The boat is going here")
                onCloseClicked: if (root.ctl) root.ctl.clearGoToTargetSelection()
            }

            Text {
                width: parent.width
                visible: isFinite(root._targetDistance) && !!root.ctl && !root.ctl.goToArrived
                text: qsTr("%1 to go").arg(root._fmtDist(root._targetDistance))
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: !!root.ctl && !root.ctl.goToArrived
                text: root._arrivalText(root._targetArrival)
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }

            KButton {
                width: parent.width
                height: Tokens.controlHMd
                enabled: root._online
                danger: true
                text: qsTr("Stop")
                toolTipText: qsTr("Loiter: the boat stops and holds its position")
                onClicked: root.stopBoat()
            }
        }
    }
}

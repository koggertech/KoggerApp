import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var view: null
    property bool hostsView: false

    readonly property var ctl: view ? view.missionRunController : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property var _core: (typeof core !== "undefined") ? core : null
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
    readonly property int _ekfPosition: _dmw ? _dmw.ekfPositionState : -1
    readonly property bool _retarget: _guided && !!ctl && ctl.goToTargetValid
    readonly property string _blockReason: {
        if (!_online) return qsTr("No autopilot")
        if (!_armed) return qsTr("Arm the boat first")
        if (!_vehicleKnown) return qsTr("Boat position unknown")
        if (_ekfPosition === 0) return qsTr("The autopilot has no position estimate yet (EKF): wait for GPS")
        if (_distance > _maxDistanceM) return qsTr("Too far: limit %1").arg(_fmtDist(_maxDistanceM))
        return ""
    }
    readonly property bool _canSend: _blockReason.length === 0 && !_sending
    readonly property real _s: AppPalette.scale

    readonly property int _roverModeAuto: 10
    readonly property int _roverModeGuided: 15
    readonly property int _gpsFix3d: 3
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

    property bool confirmPending: false
    property real _firstTapMs: 0
    property bool _sending: false
    property bool _lateAck: false
    property real _sentLat: NaN
    property real _sentLon: NaN
    property bool _awaitGuided: false

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
        _lateAck = false
        _sending = true
        ackTimer.restart()
        ctl.clearGoToPick()
        _dmw.autopilotGoTo(_sentLat, _sentLon)
    }

    on_HasPickChanged: confirmPending = false
    onHostsViewChanged: if (!hostsView) { confirmPending = false }
    on_OnlineChanged: {
        if (_online || !hostsView) return
        confirmPending = false
        _finishSend()
        _awaitGuided = false
        guidedTimer.stop()
        if (_core) _core.setGoToPlacementArmed(false)
    }
    on_ModeChanged: {
        if (!hostsView || !ctl) return
        if (_guided) {
            _awaitGuided = false
            guidedTimer.stop()
            return
        }
        if (_mode >= 0 && !_awaitGuided && ctl.goToTargetValid)
            ctl.clearGoToTarget()
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

    Connections {
        target: root._dmw
        ignoreUnknownSignals: true
        function onAutopilotCommandAcked(command, result) {
            if (!root.hostsView || command !== root._cmdDoReposition || (!root._sending && !root._lateAck)) return
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
            root._log(qsTr("Going to %1, %2").arg(root._sentLat.toFixed(6)).arg(root._sentLon.toFixed(6)))
        }
        function onAutopilotMissionItemReached(seq) {
            if (!root.hostsView || seq !== 0 || !root._guided || !root.ctl || !root.ctl.goToTargetValid || root.ctl.goToArrived) return
            root.ctl.setGoToArrived(true)
            root._toast(qsTr("The boat reached the point"))
        }
    }

    Rectangle {
        id: card

        readonly property real anchorX: root.ctl ? root.ctl.goToPickScreen.x : 0
        readonly property real anchorY: root.ctl ? root.ctl.goToPickScreen.y : 0
        readonly property real gapPx: Math.round(22 * root._s)

        visible: root.hostsView && root._hasPick && root.ctl.goToPickOnScreen
        width: Math.round(250 * root._s)
        height: content.implicitHeight + Tokens.spaceMd * 2
        x: Math.max(Tokens.spaceMd, Math.min(root.width - width - Tokens.spaceMd, anchorX - width / 2))
        y: anchorY - height - gapPx >= Tokens.spaceMd ? anchorY - height - gapPx
                                                      : Math.min(root.height - height - Tokens.spaceMd, anchorY + gapPx)
        radius: Tokens.radiusLg
        color: AppPalette.card
        border.width: 1
        border.color: root.confirmPending ? AppPalette.dangerBorder : AppPalette.border

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: function(w) { w.accepted = true }
        }

        Column {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Tokens.spaceMd
            spacing: Tokens.spaceSm

            Item {
                width: parent.width
                height: Math.max(titleText.implicitHeight, closeButton.height)

                Text {
                    id: titleText
                    anchors.left: parent.left
                    anchors.right: closeButton.left
                    anchors.rightMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Go here")
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                    elide: Text.ElideRight
                }

                KCircleIconButton {
                    id: closeButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Tokens.controlHMd
                    height: Tokens.controlHMd
                    glyph: "×"
                    glyphPixelSize: Math.round(14 * root._s)
                    fillColor: "transparent"
                    fillHoverColor: AppPalette.cardHover
                    borderColor: "transparent"
                    onClicked: if (root.ctl) root.ctl.clearGoToPick()
                }
            }

            Text {
                width: parent.width
                visible: isFinite(root._distance)
                text: qsTr("%1 from the boat").arg(root._fmtDist(root._distance))
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                elide: Text.ElideRight
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
                visible: root._blockReason.length === 0 && !root._guided
                text: root._auto ? qsTr("The boat switches to Guided and the mission pauses; Auto continues it")
                                 : qsTr("The boat switches to Guided and holds the point on arrival")
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
}

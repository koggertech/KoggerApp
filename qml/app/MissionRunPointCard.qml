import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var view: null
    property bool hostsView: false

    readonly property var ctl: view ? view.missionRunController : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property var _run: (typeof missionRun !== "undefined") ? missionRun : null
    readonly property bool _online: _dmw ? _dmw.autopilotOnline : false
    readonly property bool _auto: _dmw ? _dmw.pilotModeState === _roverModeAuto : false
    readonly property int _selectedSeq: ctl ? ctl.selectedSeq : -1
    readonly property bool _isTarget: !!_run && _selectedSeq >= 0 && _run.targetSeq === _selectedSeq && _run.running
    readonly property bool _actionable: !!_run && _run.actionable
    readonly property real _s: AppPalette.scale

    readonly property int _roverModeAuto: 10
    readonly property int _cmdSetMissionCurrent: 224
    readonly property int _resultInProgress: 5
    readonly property int _confirmMinMs: 400
    readonly property int _confirmWindowMs: 4000
    readonly property int _ackTimeoutMs: 3000

    property bool confirmPending: false
    property real _firstTapMs: 0
    property int pendingSeq: -1
    property int _seqAtSend: -1
    property string _pendingLabel: ""

    on_SelectedSeqChanged: confirmPending = false
    on_OnlineChanged: if (!_online) { confirmPending = false; pendingSeq = -1; ackTimer.stop() }

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

    function _warn(result) {
        if (typeof notifications !== "undefined" && notifications)
            notifications.info(qsTr("Go to point: %1").arg(resultText(result)))
    }

    function _reached(current) {
        return pendingSeq >= 0 && current !== _seqAtSend && (current === pendingSeq || current === pendingSeq + 1)
    }

    function tapGo() {
        if (!ctl || !_dmw || !_online || _isTarget || !_actionable || ctl.selectedJumpSeq < 0) return
        var now = Date.now()
        if (!confirmPending) {
            confirmPending = true
            _firstTapMs = now
            confirmTimer.restart()
            return
        }
        if (now - _firstTapMs < _confirmMinMs) return
        confirmPending = false
        confirmTimer.stop()
        pendingSeq = ctl.selectedJumpSeq
        _seqAtSend = _dmw.missionCurrentSeq
        _pendingLabel = ctl.selectedLabel
        ackTimer.restart()
        _dmw.autopilotSetMissionCurrent(pendingSeq)
        if (typeof notifications !== "undefined" && notifications)
            notifications.info(qsTr("Going to %1").arg(_pendingLabel))
        ctl.clearSelection()
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
            if (root.pendingSeq < 0) return
            var reached = root._reached(root._dmw ? root._dmw.missionCurrentSeq : -1)
            root.pendingSeq = -1
            if (!reached) root._warn(-2)
        }
    }

    Connections {
        target: root._dmw
        ignoreUnknownSignals: true
        function onAutopilotCommandAcked(command, result) {
            if (command !== root._cmdSetMissionCurrent || root.pendingSeq < 0 || result === root._resultInProgress) return
            root.pendingSeq = -1
            ackTimer.stop()
            if (result !== 0) root._warn(result)
        }
        function onVruChanged() {
            if (root.pendingSeq < 0) return
            if (root._reached(root._dmw.missionCurrentSeq)) {
                root.pendingSeq = -1
                ackTimer.stop()
            }
        }
    }

    Rectangle {
        id: card

        readonly property real anchorX: root.ctl ? root.ctl.selectedScreen.x : 0
        readonly property real anchorY: root.ctl ? root.ctl.selectedScreen.y : 0
        readonly property real gapPx: Math.round(22 * root._s)

        visible: root.hostsView && !!root.ctl && root._selectedSeq >= 0 && root.ctl.selectedOnScreen
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
                    text: root.ctl ? root.ctl.selectedLabel : ""
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
                    onClicked: if (root.ctl) root.ctl.clearSelection()
                }
            }

            Text {
                width: parent.width
                text: {
                    if (!root.ctl || !root._run) return ""
                    var line = qsTr("Point %1 of %2").arg(root.ctl.selectedNav).arg(root._run.navCount)
                    return root._isTarget ? line + " · " + qsTr("current target") : line
                }
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: !root._auto
                text: qsTr("The mission continues from this point when the boat is switched to Auto")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }

            KButton {
                width: parent.width
                height: Tokens.controlHMd
                enabled: root._online && !root._isTarget && root._actionable
                danger: root.confirmPending
                text: root.confirmPending ? qsTr("Tap again to go")
                    : (root._auto ? qsTr("Go here") : qsTr("Start from here"))
                toolTipText: root._auto ? qsTr("The boat skips to this point at once") : ""
                onClicked: root.tapGo()
            }
        }
    }
}

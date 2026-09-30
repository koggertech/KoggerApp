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

    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property bool _online: _dmw ? _dmw.autopilotOnline : false
    readonly property int _arm: _dmw ? _dmw.pilotArmState : -1
    readonly property int _mode: _dmw ? _dmw.pilotModeState : -1
    readonly property string _modeName: _dmw ? _dmw.autopilotModeName : ""
    readonly property real _voltage: _dmw ? _dmw.vruVoltage : NaN
    readonly property real _speed: _dmw ? _dmw.vruVelocityH : NaN
    readonly property real _current: _dmw ? _dmw.vruCurrent : NaN
    readonly property int _batteryPct: _dmw ? _dmw.vruBatteryPercent : -1
    readonly property int _linkQuality: _dmw ? _dmw.averageChartLosses : -1
    readonly property bool _hasSonar: {
        var list = _dmw ? _dmw.devs : null
        if (!list) return false
        for (var i = 0; i < list.length; ++i)
            if (list[i] && list[i].isBoardInited) return true
        return false
    }
    readonly property bool _armed: _arm > 0

    readonly property string _statsText: {
        var parts = []
        if (_batteryPct >= 0) parts.push(qsTr("Bat. %1 %").arg(_batteryPct))
        if (isFinite(_voltage)) parts.push(_voltage.toFixed(1) + " V")
        if (isFinite(_current)) parts.push(_current.toFixed(1) + " A")
        if (isFinite(_speed)) parts.push(_speed.toFixed(1) + " " + qsTr("m/s"))
        if (_hasSonar && _linkQuality >= 0) parts.push(qsTr("Link %1 %").arg(_linkQuality))
        return parts.join("  ·  ")
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
    readonly property real _pillH: _controlH + _sidePad * 2
    readonly property real _wantW: _pillW + contentPadding * 2
    readonly property real _wantH: headerHeight + _pillH + contentPadding

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
        notifications.warning(qsTr("%1: %2").arg(commandText(command)).arg(resultText(result)))
    }

    on_WantWChanged: _applySize()
    on_WantHChanged: _applySize()
    on_OnlineChanged: if (!_online) { _cancelArmConfirm(); _statsW = 0 }
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
            root._warnFailure(command, -2)
        }
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
            ackTimer.stop()
            if (result !== 0)
                root._warnFailure(command, result)
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

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: AppPalette.bg
        border.width: 0

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
            id: infoColumn
            anchors.left: statusDot.right
            anchors.leftMargin: Math.round(8 * root._s)
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(Math.round(240 * root._s),
                            Math.max(Math.round(84 * root._s), titleText.implicitWidth, root._statsW))

            Text {
                id: titleText
                width: parent.width
                text: root.armConfirmPending ? qsTr("Tap again to arm")
                    : !root._online ? qsTr("No autopilot")
                    : root._modeName
                color: root.armConfirmPending ? AppPalette.dangerText : AppPalette.textStrong
                font.pixelSize: Math.round(13 * root._s)
                font.bold: true
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                visible: root._online && text.length > 0
                text: root._statsText
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
}

import QtQuick 2.15
import kqml_types 1.0

// Operating half of the USBL UI: live solution readout, the remote-node set and the
// interrogation schedule. Configuration lives in UsblPlanGroup.
//
// The readout reads Dataset, not the device: widget panels are global and the solution
// cache is device-agnostic (see docs/KoggerApp-Docs/usbl-protocol.md → QML-facing surface).
DeviceSettingsGroup {
    id: usblGroup

    property var dev: null
    property var plan: null
    // 1 s clock used to age the last fix. lastUsblFixEpochMs is constant per fix, so the
    // ticking dependency has to come from outside — same trick DataFieldCatalog uses for
    // the `time` field.
    property string clockTick: ""

    // VESTIGIAL. The store now REPLACES its whole state on every edit (UsblPlanLogic.js),
    // so `plan.groups`, `plan.trigger()` and friends already change identity and bindings
    // re-evaluate on their own. The few `var _d = _rev` reads left below are harmless
    // no-ops kept to avoid churning working bindings -- do not copy the pattern.
    readonly property int _rev: plan ? plan.rev : 0

    title: qsTr("USBL")
    titlePixelSize: 13
    stateKey: "dev.usbl"
    collapsedByDefault: false
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon))

    readonly property real _w: width
    readonly property bool _hasFix: !!(dataset && dataset.isLastUsblSolutionValid)
    readonly property real _ageS: {
        var _t = clockTick
        if (!_hasFix) return -1
        return Math.max(0, (Date.now() - dataset.lastUsblFixEpochMs) / 1000)
    }
    readonly property bool _stale: _ageS >= 0 && _ageS > 5
    readonly property color _sevColor: !_hasFix ? AppPalette.textMuted
                                     : (_stale ? AppPalette.linkIdleBorder : AppPalette.linkOkBorder)
    readonly property color _sevBg: !_hasFix ? AppPalette.rowRaised
                                  : (_stale ? AppPalette.linkIdleBg : AppPalette.linkOkBg)

    function _fmtAge(s) {
        if (s < 0) return ""
        return (s < 10 ? s.toFixed(1) : Math.round(s)) + qsTr(" s ago")
    }
    function _num(v, digits, suffix) {
        if (v === undefined || v === null || isNaN(v)) return "—"
        return v.toFixed(digits) + (suffix ? suffix : "")
    }
    function _dms(deg, isLat) {
        if (isNaN(deg)) return "—"
        var a = Math.abs(deg)
        var d = Math.floor(a)
        var m = (a - d) * 60
        var hemi = isLat ? (deg < 0 ? "S" : "N") : (deg < 0 ? "W" : "E")
        return d + "°" + (m < 10 ? "0" : "") + m.toFixed(3) + "′ " + hemi
    }

    // KSwitch is a full-width labelled row; a node needs a compact inline toggle, so this
    // reproduces the KSwitch track/knob at chip size using the same palette tokens.
    component MiniSwitch: Rectangle {
        id: ms
        property bool checked: false
        signal toggled()
        implicitWidth: Math.round(36 * AppPalette.scale)
        implicitHeight: Math.round(20 * AppPalette.scale)
        radius: height / 2
        color: ms.checked ? AppPalette.toggleOn : AppPalette.trackOff
        border.width: Math.max(1, Math.round(1 * AppPalette.scale))
        border.color: ms.checked ? AppPalette.toggleOnBorder : AppPalette.trackOffBorder
        Rectangle {
            width: ms.height - Math.round(6 * AppPalette.scale); height: width; radius: width / 2
            y: (ms.height - height) / 2
            x: ms.checked ? ms.width - width - y : y
            color: AppPalette.knob
            Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }
        KTapArea { anchors.fill: parent; onTapped: ms.toggled() }
    }

    // Address chips for the enabled nodes — visible while the group is collapsed.
    headerActions: Row {
        spacing: Tokens.spaceSm
        rightPadding: Tokens.spaceSm
        Repeater {
            model: plan ? plan.nodes.filter(function (n) { return n.active }).slice(0, 4) : []
            delegate: Rectangle {
                id: addrChip
                required property var modelData
                readonly property bool _live: usblGroup._hasFix && !usblGroup._stale
                                             && dataset && dataset.lastUsblAddress === addrChip.modelData.addr
                implicitWidth: _chipRow.implicitWidth + Tokens.spaceLg * 2
                width: implicitWidth
                height: usblGroup.headerActionSize - Tokens.spaceSm * 2
                radius: height / 2
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                color: "transparent"
                border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                border.color: addrChip._live ? AppPalette.linkOkBorder : AppPalette.border
                Row {
                    id: _chipRow
                    anchors.centerIn: parent
                    spacing: Math.round(5 * AppPalette.scale)
                    Rectangle {
                        width: Math.round(6 * AppPalette.scale); height: width; radius: width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: addrChip._live ? AppPalette.linkOkBorder : "transparent"
                        border.width: addrChip._live ? 0 : 1
                        border.color: AppPalette.textMuted
                    }
                    Text {
                        text: String(addrChip.modelData.addr)
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontXs; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    // ── hero: is the beacon being tracked right now, and if not, why ──────
    Rectangle {
        width: parent.width
        radius: Tokens.radiusMd
        color: usblGroup._sevBg
        border.width: Tokens.cardBorderWidth
        border.color: usblGroup._hasFix ? usblGroup._sevColor : AppPalette.border
        implicitHeight: _heroRow.implicitHeight + 2 * Tokens.spaceMd
        Row {
            id: _heroRow
            x: Tokens.spaceLg; width: parent.width - 2 * Tokens.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            spacing: Tokens.spaceMd
            Rectangle {
                width: Math.round(8 * AppPalette.scale); height: width; radius: width / 2
                color: usblGroup._sevColor
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: !usblGroup._hasFix ? qsTr("NO SOLUTION")
                    : (usblGroup._stale ? qsTr("STALE") : qsTr("TRACKING"))
                color: usblGroup._sevColor
                font.pixelSize: Tokens.fontMd; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                width: Math.max(0, parent.width - x - Tokens.spaceMd)
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                text: !usblGroup._hasFix
                      ? (plan && plan.nodes.length ? qsTr("press Start to interrogate") : qsTr("add a remote node"))
                      : qsTr("addr %1 · %2").arg(dataset.lastUsblAddress).arg(usblGroup._fmtAge(usblGroup._ageS))
            }
        }
    }

    // ── live readout ──────────────────────────────────────────────────────
    Grid {
        width: parent.width
        columns: 3
        columnSpacing: Tokens.spaceSm
        rowSpacing: Tokens.spaceSm
        readonly property real cellW: (width - columnSpacing * 2) / 3

        component StatCell: Rectangle {
            property string value: "—"
            property string label: ""
            radius: Tokens.radiusMd
            color: AppPalette.rowRaised
            border.width: Tokens.cardBorderWidth
            border.color: AppPalette.border
            implicitHeight: _sc.implicitHeight + 2 * Tokens.spaceMd
            Column {
                id: _sc
                x: Tokens.spaceMd; y: Tokens.spaceMd
                width: parent.width - 2 * Tokens.spaceMd
                spacing: 0
                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: value; color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontXl; font.bold: true
                }
                Text { text: label; color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs }
            }
        }

        StatCell {
            width: parent.cellW
            value: usblGroup._hasFix ? usblGroup._num(dataset.lastUsblDistance, 1) : "—"
            label: qsTr("Distance, m")
        }
        StatCell {
            width: parent.cellW
            value: usblGroup._hasFix ? usblGroup._num(dataset.lastUsblAzimuth, 1, "°") : "—"
            label: qsTr("Azimuth, °")
        }
        StatCell {
            width: parent.cellW
            value: usblGroup._hasFix ? usblGroup._num(dataset.lastUsblElevation, 1, "°") : "—"
            label: qsTr("Elevation, °")
        }
        StatCell {
            width: parent.cellW
            value: usblGroup._hasFix ? usblGroup._num(dataset.lastUsblSnr, 0) : "—"
            label: qsTr("SNR, dB")
        }
        StatCell {
            width: parent.cellW * 2 + parent.columnSpacing
            value: (dataset && dataset.isLastUsblBeaconCoordinateValid)
                   ? usblGroup._dms(dataset.lastUsblBeaconLatitude, true) + "\n"
                     + usblGroup._dms(dataset.lastUsblBeaconLongitude, false)
                   : "—"
            label: qsTr("Beacon position")
        }
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // ── remote nodes ──────────────────────────────────────────────────────
    Row {
        width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
        Text {
            text: plan && plan.role === "initiator"
                  ? qsTr("Nodes we interrogate")
                  : qsTr("Nodes that interrogate us")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm; font.bold: true
            width: Math.max(0, parent.width - _addNodeBtn.width - parent.spacing)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblButton {
            id: _addNodeBtn
            text: qsTr("+ Add node")
            implicitHeight: Tokens.controlHSm
            fontPixelSize: Tokens.fontXs
            anchors.verticalCenter: parent.verticalCenter
            onClicked: if (plan) plan.addNode()
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceMd

        Text {
            visible: !plan || !plan.nodes.length
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            padding: Tokens.spaceXl
            text: qsTr("No remote nodes. Add one to start interrogating.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.WordWrap
        }

        Repeater {
            // nodesView, not nodes: the view is rebuilt on every `rev` bump, so a step
            // added or removed actually reaches the card. Binding to `nodes` gave each
            // delegate a live node object whose in-place edits QML cannot see.
            model: plan ? plan.nodesView : []
            delegate: Rectangle {
                id: nodeCard
                required property var modelData
                readonly property var _n: modelData
                readonly property bool _isCurrent: usblGroup._curStep && usblGroup._curStep.nodeId === _n.id

                width: parent.width
                radius: Tokens.radiusMd
                color: AppPalette.rowRaised
                border.width: _isCurrent ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : Tokens.cardBorderWidth
                border.color: _isCurrent ? AppPalette.accentBorder : AppPalette.border
                opacity: _n.active ? 1.0 : 0.62
                implicitHeight: _nodeCol.implicitHeight + 2 * Tokens.spaceMd

                Column {
                    id: _nodeCol
                    x: Tokens.spaceMd; y: Tokens.spaceMd
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: Tokens.spaceSm

                    Row {
                        width: parent.width; spacing: Tokens.spaceSm
                        MiniSwitch {
                            id: _nodeSw
                            checked: nodeCard._n.active
                            anchors.verticalCenter: parent.verticalCenter
                            onToggled: if (plan) plan.toggleNode(nodeCard._n.id)
                        }
                        Text {
                            text: qsTr("Address")
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        UsblSpin {
                            id: _addrSpin
                            from: 0; to: 8; stepSize: 1
                            devValue: nodeCard._n.addr
                            implicitWidth: Math.round(78 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                            writeBack: function (v) { if (plan) plan.setNodeAddr(nodeCard._n.id, v) }
                        }
                        Text {
                            text: nodeCard._n.cmdCount
                                  ? qsTr("%1 command(s)").arg(nodeCard._n.cmdCount)
                                  : qsTr("no commands")
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            width: Math.max(0, parent.width - _nodeSw.width - _addrSpin.width
                                            - _stepCombo.width - _delNode.width - parent.spacing * 5
                                            - Math.round(34 * AppPalette.scale))
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item { width: 1; height: 1 }
                        KCombo {
                            id: _stepCombo
                            readonly property var _avail: {
                                var _d = usblGroup._rev
                                return plan
                                    ? plan.groups.filter(function (g) { return plan.schedulable(g) })
                                    : []
                            }
                            model: [qsTr("+ Command")].concat(_avail.map(function (g) {
                                var _d = usblGroup._rev
                                return "cmd " + plan.slotLabel(g)
                            }))
                            currentIndex: 0
                            enabled: _avail.length > 0
                            implicitWidth: Math.round(96 * AppPalette.scale)
                            fontPixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                            onActivated: function (i) {
                                if (i > 0 && plan) plan.addStep(nodeCard._n.id, _avail[i - 1].id)
                                currentIndex = 0
                            }
                        }
                        KCircleIconButton {
                            id: _delNode
                            glyph: "×"
                            width: Tokens.controlHSm; height: Tokens.controlHSm
                            glyphPixelSize: Math.round(14 * AppPalette.scale)
                            borderWidth: Tokens.cardBorderWidth
                            anchors.verticalCenter: parent.verticalCenter
                            toolTipText: qsTr("Remove node")
                            onClicked: if (plan) plan.removeNode(nodeCard._n.id)
                        }
                    }

                    // Step chips — each names one concrete cmd slot from its group's set.
                    Flow {
                        width: parent.width
                        spacing: Tokens.spaceXs
                        visible: nodeCard._n.steps.length > 0
                        Repeater {
                            model: nodeCard._n.steps
                            delegate: Rectangle {
                                // Needs an id: QML property lookup goes own-properties ->
                                // ids in the component -> component root. An ANCESTOR's
                                // properties are not in scope, so a bare `_cur` inside the
                                // Texts below resolves to nothing at runtime.
                                id: _refChip
                                required property var modelData
                                required property int index
                                // Colour comes from the snapshot; groupById()/colorOf() in a
                                // binding would not re-run when the plan changes.
                                readonly property bool _cur: usblGroup._curStep
                                                             && usblGroup._curStep.nodeId === nodeCard._n.id
                                                             && usblGroup._curStep.groupId === modelData.group
                                                             && usblGroup._curStep.cmd === modelData.cmd
                                radius: Tokens.radiusSm
                                color: _cur ? AppPalette.accent : AppPalette.card
                                border.width: Tokens.cardBorderWidth
                                border.color: _cur ? AppPalette.accentBorder
                                                   : _refChip.modelData.color
                                implicitWidth: _chip.implicitWidth + Tokens.spaceMd * 2
                                implicitHeight: Math.round(24 * AppPalette.scale)
                                Row {
                                    id: _chip
                                    anchors.centerIn: parent
                                    spacing: Tokens.spaceXs
                                    Text {
                                        text: "cmd " + modelData.cmd
                                        color: _refChip._cur ? AppPalette.accentText : AppPalette.textStrong
                                        font.pixelSize: Tokens.fontXs; font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "×"
                                        color: _refChip._cur ? AppPalette.accentText : AppPalette.textMuted
                                        font.pixelSize: Tokens.fontMd
                                        anchors.verticalCenter: parent.verticalCenter
                                        KTapArea {
                                            anchors.fill: parent
                                            anchors.margins: -Tokens.spaceXs
                                            onTapped: if (plan) plan.removeStep(nodeCard._n.id, index)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        text: plan && plan.role === "initiator"
              ? qsTr("A node with no commands is interrogated once, with command 0.")
              : qsTr("These are the addresses this device will answer.")
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // ── schedule ──────────────────────────────────────────────────────────
    property var _curStep: null
    property int _stepIndex: -1

    function _advance() {
        if (!plan) return
        var sched = plan.schedule
        if (!sched.length) { _stop(); return }
        _stepIndex = _stepIndex < 0 ? 0 : _stepIndex + 1
        var s = sched[_stepIndex % sched.length]
        _curStep = s
        if (!dev) return
        // Only the schedule is per-node, so only the ping is sent per step.
        var g = s.implicit ? null : plan.groupById(s.groupId)
        var snd = g ? g.ini.send : null
        if (!snd) { dev.acousticPingRequest(s.addr, 0xFFFFFFFF); return }
        var fn = plan.findBy(plan.pingFunctions, snd.fn)
        var payload = (fn.id === "bits") ? snd.payload : ""
        dev.acousticPingRequestEx(s.addr, 0xFFFFFFFF, s.cmd, snd.reply, payload)
    }
    function _stop() { _runTimer.stop() }
    function _start() { _runTimer.start(); _advance() }

    property Timer _runTimer: Timer {
        interval: _dwell.value > 0 ? _dwell.value : 700
        repeat: true
        onTriggered: usblGroup._advance()
    }

    Column {
        id: _schedSect
        width: parent.width
        spacing: Tokens.spaceSm
        visible: !plan || plan.role === "initiator"

        Row {
            width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
            Text {
                text: plan && plan.schedule.length
                      ? qsTr("Schedule · %1 nodes · %2 steps").arg(plan.activeNodeCount).arg(plan.schedule.length)
                      : qsTr("Schedule")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; font.bold: true
                width: Math.max(0, parent.width - _stepBtn.width - _runBtn.width - parent.spacing * 2)
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
            UsblButton {
                id: _stepBtn
                text: qsTr("Step"); implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                anchors.verticalCenter: parent.verticalCenter
                enabled: !!(dev && plan && plan.schedule.length)
                onClicked: { usblGroup._stop(); usblGroup._advance() }
            }
            UsblButton {
                id: _runBtn
                text: usblGroup._runTimer.running ? qsTr("■ Stop") : qsTr("▶ Start")
                implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                checked: usblGroup._runTimer.running
                anchors.verticalCenter: parent.verticalCenter
                enabled: !!(dev && plan && plan.schedule.length)
                onClicked: usblGroup._runTimer.running ? usblGroup._stop() : usblGroup._start()
            }
        }

        // Materialized (addr, cmd) cycle — the iteration rule made visible.
        Flickable {
            width: parent.width
            implicitHeight: _stripRow.implicitHeight
            contentWidth: _stripRow.implicitWidth
            contentHeight: _stripRow.implicitHeight
            flickableDirection: Flickable.HorizontalFlick
            clip: true
            Row {
                id: _stripRow
                spacing: Tokens.spaceXs
                Repeater {
                    model: plan ? plan.schedule : []
                    delegate: Row {
                        required property var modelData
                        required property int index
                        spacing: Tokens.spaceXs
                        Text {
                            visible: index > 0
                            text: "→"; color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Rectangle {
                            // id needed so the child Text can reach _cur: ancestor
                            // properties are not in QML scope, and a bare `_cur` there is a
                            // runtime ReferenceError that no compile step catches.
                            id: _stepChip
                            readonly property bool _cur: plan && plan.schedule.length
                                && usblGroup._stepIndex >= 0
                                && (usblGroup._stepIndex % plan.schedule.length) === index
                            radius: Tokens.radiusSm
                            color: _cur ? AppPalette.accent : AppPalette.card
                            border.width: Tokens.cardBorderWidth
                            border.color: _cur ? AppPalette.accentBorder : AppPalette.border
                            implicitWidth: _stepTxt.implicitWidth + Tokens.spaceMd * 2
                            implicitHeight: Math.round(22 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                id: _stepTxt
                                anchors.centerIn: parent
                                text: qsTr("addr %1 · cmd %2").arg(modelData.addr).arg(modelData.cmd)
                                color: _stepChip._cur ? AppPalette.accentText : AppPalette.text
                                font.pixelSize: Tokens.fontXs
                                font.bold: _stepChip._cur
                            }
                        }
                    }
                }
                Text {
                    visible: !!(plan && plan.schedule.length)
                    text: "⟳"; color: AppPalette.textMuted; font.pixelSize: Tokens.fontMd
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    visible: !plan || !plan.schedule.length
                    text: qsTr("No enabled node — nothing to interrogate.")
                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                }
            }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm
            Text {
                text: qsTr("Dwell"); color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            KSpinBox {
                id: _dwell
                from: 100; to: 5000; stepSize: 100; value: 700
                implicitWidth: Math.round(96 * AppPalette.scale)
                anchors.verticalCenter: parent.verticalCenter
                toolTipText: qsTr("Interval between steps, timed by the app")
            }
            Text {
                text: qsTr("ms between steps")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                width: Math.max(0, parent.width - _dwell.width - Math.round(60 * AppPalette.scale) - parent.spacing * 2)
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}

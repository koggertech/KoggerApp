import QtQuick 2.15
import QtCore
import kqml_types 1.0
import "UsblNodeLogic.js" as Node

// The acoustic nodes. The LIST is this pane -- everything else serves it.
//
// Each row carries its own live numbers and its own state, so there is no separate readout that
// could disagree with the list, and no global band claiming one truth for a set of beacons that
// answer independently. The schedule controls sit above the list because they act on all of it.
//
// THE INTERROGATION CYCLE, which is what the rows report: one request is in flight at a time,
// an answer is due after roughly 2R/c plus the node's turn-around, and the window closes when
// the reply lands, when the next request goes out, or when its budget runs out. The rules and
// the transitions live in UsblNodeLogic.js; this file drives them and translates the codes.
//
// Rows read Dataset, not the device: widget panels are global and the solution cache is
// device-agnostic (see docs/KoggerApp-Docs/usbl-protocol.md -> QML-facing surface).
//
// What each row REPORTS is UsblNodeLogic.js; what it CONFIGURES is UsblPlanGroup.
DeviceSettingsGroup {
    id: usblGroup

    property var dev: null
    property var plan: null
    // 1 s clock, for ages and for the age-based half of the reply axis. epochMs is constant per
    // fix, so the ticking dependency has to come from outside: Date.now() inside a binding is
    // evaluated once and then frozen. Supplied by DeviceSettingsPage.
    //
    // It does NOT drive the window transitions. A window can be shorter than one tick, so
    // sampling at 1 Hz would miss it entirely -- Waiting is opened by the send, closed by the
    // arriving solution or by a timer that fires exactly at the deadline.
    property string clockTick: ""

    // VESTIGIAL. The store now REPLACES its whole state on every edit (UsblPlanLogic.js), so
    // `plan.groups`, `plan.trigger()` and friends already change identity and bindings
    // re-evaluate on their own. The few `var _d = _rev` reads left below are harmless no-ops
    // kept to avoid churning working bindings -- do not copy the pattern.
    readonly property int _rev: plan ? plan.rev : 0

    // "Acoustic nodes", not "…scheduler": after the readout moved into the rows this pane
    // reports as much as it drives, and the scheduler is one control row inside it. stateKey is
    // deliberately unchanged by the rename so a collapsed group stays collapsed.
    title: qsTr("Acoustic nodes")
    titlePixelSize: 13
    stateKey: "dev.usbl"
    collapsedByDefault: false
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon))

    // ── the inputs every row derives from ─────────────────────────────────
    readonly property real _nowMs: { var _t = clockTick; return Date.now() }
    readonly property var _sols: (dataset && dataset.usblSolutions) ? dataset.usblSolutions : ({})
    readonly property var _summary: Node.summary(plan ? plan.nodes : [], _sols, _poll)
    // Where the cycle is. Survives the answer window closing, so the framed row still says which
    // node was last interrogated -- and before anything has been asked, which one Step will take.
    readonly property int _cursorNodeId: Node.cursorNodeId(_curStep, plan ? plan.schedule : [])

    // Codes from UsblNodeLogic, translated here -- the logic module never returns prose.
    //
    // TWO AXES, read together. Whether a node's answer window is open sits beside what the node
    // last did, because "stale" means a fault while we are asking and means nothing at all while
    // we are not. test_usbl_node_logic.mjs asserts these tables name every code the module can
    // return and nothing else.
    //
    // There is no word for emission. It lasts milliseconds and the protocol reports nothing
    // about it, so the row blinks Waiting to say the request is fresh rather than inventing a
    // state nobody can observe.
    readonly property var _opText: ({
        "off":     qsTr("Off"),
        "idle":    qsTr("Idle"),
        "waiting": qsTr("Waiting")
    })
    readonly property var _replyText: ({
        "replied": qsTr("Replied"),
        "stale":   qsTr("Stale"),
        // Never answered. A symbol, not a sentence: there is nothing to translate, and a word
        // here would read as a diagnosis of a beacon nobody has heard from.
        "none":    "—"
    })

    function _fmtAge(ms) {
        if (ms < 0) return ""
        var s = ms / 1000
        return (s < 10 ? s.toFixed(1) : Math.round(s)) + qsTr(" s ago")
    }
    function _num(v, digits, suffix) {
        if (v === undefined || v === null || isNaN(v)) return "—"
        return v.toFixed(digits) + (suffix ? suffix : "")
    }
    // Decimal degrees, 6 dp (~0.1 m) -- the form that pastes into a map or a GIS. Which form the
    // CLIPBOARD gets is still open; this is what the tooltip shows meanwhile.
    function _ll(e) {
        if (!e || !e.coordValid) return ""
        return e.beaconLat.toFixed(6) + ", " + e.beaconLon.toFixed(6)
    }

    // THE COPY SEAM, AND IT IS NOT CONNECTED. This app has no clipboard anywhere -- writing one
    // needs QGuiApplication::clipboard() exposed from C++ -- so the affordance is here and the
    // behaviour is not. One function, so wiring it later is one edit.
    //
    // Until then the button's tooltip is what makes the position readable, and it says the copy
    // is not wired up yet: a button that silently does nothing is worse than one that admits it.
    function _copyCoordinate(addr, text) {
        console.log("USBL: copy position for addr " + addr + " (" + text
                    + ") -- not wired up yet, no clipboard backend")
    }

    // ── the interrogation cycle ───────────────────────────────────────────
    // Which node's answer window is open, and which nodes are known to have missed. Replaced
    // wholesale by the reducer, never edited here.
    property var _poll: Node.initialPoll()

    readonly property bool _anyWaiting: _poll.waitNodeId >= 0

    // Which rows are expanded, by node id. Session state, not plan content: it is how you are
    // looking at the pane right now, and persisting it would restore an inspection nobody asked
    // to resume. Replaced rather than edited, so the delegates' bindings re-evaluate.
    property var _expanded: ({})
    function _toggleExpanded(nodeId) {
        var next = {}
        for (var k in _expanded) if (_expanded[k]) next[k] = true
        if (next[nodeId]) delete next[nodeId]
        else next[nodeId] = true
        _expanded = next
    }

    // The deadline. Non-repeating and restarted on every request: while the schedule runs the
    // next request closes the window first, so this only decides the single-Step case and the
    // last step before Stop. It is a real timer rather than a comparison against the 1 s clock
    // because the window is usually shorter than one tick.
    property Timer _waitTimer: Timer {
        interval: Node.waitMs(usblGroup._dwellMs, Node.GRACE_MS)
        repeat: false
        onTriggered: {
            usblGroup._poll = Node.noteTimeout(usblGroup._poll)
            usblGroup._drainManual()
        }
    }

    // One signal per arriving solution (Dataset::addUsblSolution emits it after updating the
    // per-address cache), which is what lets a window close the instant its reply lands instead
    // of on the next clock tick.
    Connections {
        target: dataset
        function onLastUsblSolutionChanged() {
            if (!plan) return
            var wasOpen = usblGroup._poll.waitNodeId
            usblGroup._poll = Node.noteReplyAddr(usblGroup._poll, plan.nodes,
                                                 dataset.lastUsblAddress)
            if (wasOpen >= 0 && usblGroup._poll.waitNodeId < 0) {
                usblGroup._waitTimer.stop()
                // The window just closed, so anything asked for by hand can go now rather than
                // waiting for a scheduler that may not be running.
                usblGroup._drainManual()
            }
        }
    }

    // The one received payload the app retains. Dataset keeps a solution per address, but the
    // modem payload is not cached at all -- modemLastPayload and friends read straight through to
    // the CURRENT frame's header -- so exactly one command row can show data, the one the newest
    // frame names. Rows say "—" rather than nothing so the answer does not jump between rows of
    // different heights, and the pane says once that only the latest frame is kept.
    //
    // The arrival time is stamped here because the frame carries none; it is one scalar for the
    // one retained payload, not a cache.
    property real _modemAtMs: 0
    Connections {
        target: dev
        enabled: !!dev
        function onModemPayloadChanged() { usblGroup._modemAtMs = Date.now() }
    }
    function _modemMatches(addr, cmd) {
        return !!dev && dev.modemLastBitLength > 0
               && dev.modemLastAddressFrom === addr && dev.modemLastCmdId === cmd
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

    // Both axes render the same way -- a word, a fill, an outline -- so one component owns the
    // code -> colour mapping. It takes the CODE and the already-translated word: the tables
    // above live on the file root, which is not in an inline component's scope.
    //
    // NOTHING HERE FADES. Pulsing `opacity` was tried and is wrong: opacity applies to the
    // border as well as the fill, so every dim phase erased the frame that identifies the
    // badge. What moves instead is the ROW's background colour, below -- one pulsing thing on
    // the pane, and it is the thing that is happening.
    component Badge: Rectangle {
        id: bdg
        property string label: ""
        property string code: ""
        property bool reply: false          // false = the operation axis
        // The age chip. Not a code: it reports how old the numbers are, which is a different
        // question from what the last interrogation did, and it stays OUTLINED rather than
        // filled so it reads as a note about the data instead of a verdict on the beacon.
        property bool aged: false
        readonly property bool _open: !bdg.reply && bdg.code === "waiting"
        readonly property bool _ok:    bdg.reply && bdg.code === "replied"
        readonly property bool _old:   bdg.reply && bdg.code === "stale"
        implicitWidth: _bdgText.implicitWidth + Tokens.spaceMd * 2
        implicitHeight: Math.round(20 * AppPalette.scale)
        radius: Tokens.radiusSm
        color: bdg._open ? AppPalette.accentBg
             : bdg._ok   ? AppPalette.linkOkBg
             : bdg._old  ? AppPalette.linkIdleBg : "transparent"
        border.width: Math.max(1, Math.round(1 * AppPalette.scale))
        border.color: bdg._open ? AppPalette.accentBorder
                    : bdg._ok   ? AppPalette.linkOkBorder
                    : bdg._old  ? AppPalette.linkIdleBorder
                    : bdg.aged  ? AppPalette.linkIdleBorder : AppPalette.border
        Text {
            id: _bdgText
            anchors.centerIn: parent
            text: bdg.label
            color: bdg._open ? AppPalette.accentText
                 : bdg._ok   ? AppPalette.linkOkText
                 : bdg._old  ? AppPalette.linkIdleText
                 : bdg.aged  ? AppPalette.linkIdleText : AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
            font.bold: true
        }
    }

    // A row is read by scanning down a column, so the three numbers keep the same three places
    // whatever is missing. Absence is an em dash in place, never a collapsed cell.
    component MiniStat: Rectangle {
        property string value: "—"
        property string label: ""
        radius: Tokens.radiusSm
        color: AppPalette.card
        border.width: Tokens.cardBorderWidth
        border.color: AppPalette.border
        implicitHeight: _msCol.implicitHeight + 2 * Tokens.spaceSm
        Column {
            id: _msCol
            x: Tokens.spaceSm; y: Tokens.spaceSm
            width: parent.width - 2 * Tokens.spaceSm
            spacing: 0
            Text {
                width: parent.width; elide: Text.ElideRight
                text: value; color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg; font.bold: true
            }
            Text {
                width: parent.width; elide: Text.ElideRight
                text: label; color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            }
        }
    }

    // Collapsed or open, the header answers "is anything answering". It replaced a full-width
    // status band: with every row carrying its own state, a global band restated the same thing
    // less precisely and cost the list a block of height.
    headerActions: Row {
        spacing: Tokens.spaceSm
        rightPadding: Tokens.spaceSm
        Rectangle {
            width: Math.round(7 * AppPalette.scale); height: width; radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            visible: usblGroup._summary.active > 0
            color: usblGroup._summary.replying > 0 ? AppPalette.linkOkBorder : "transparent"
            border.width: usblGroup._summary.replying > 0 ? 0 : Math.max(1, Math.round(1 * AppPalette.scale))
            border.color: AppPalette.textMuted
        }
        Text {
            // Spelled out, not a bare fraction: this is the only pane-level summary left, and
            // "3/4" beside a row of addresses reads as a count of addresses. Room for the word is
            // why the group's title is the short one.
            text: !usblGroup._summary.total ? qsTr("no nodes")
                : !usblGroup._summary.active ? qsTr("all off")
                : qsTr("%1/%2 replying").arg(usblGroup._summary.replying)
                                        .arg(usblGroup._summary.active)
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        // Which addresses, for the collapsed state — carrying THE SAME state language as the rows
        // and the command chips, because this is the same fact seen from further away. It used to
        // be a single boolean (replied, or not), which made a node that has gone stale and one
        // nobody has ever heard from look identical, and never showed a request in flight at all.
        //
        // One mark has to serve two axes here, so it follows the command chips' precedence:
        // waiting outranks the previous verdict.
        Repeater {
            model: plan ? plan.nodes.filter(function (n) { return n.active }).slice(0, 4) : []
            delegate: Rectangle {
                id: addrChip
                required property var modelData
                readonly property string _state:
                    Node.isWaiting(usblGroup._poll, addrChip.modelData.id)
                    ? "waiting"
                    : Node.nodeReplyCode(usblGroup._poll, addrChip.modelData.id,
                                         Node.entryFor(usblGroup._sols, addrChip.modelData.addr))
                readonly property bool _out: _state === "waiting"
                readonly property bool _ok:  _state === "replied"
                readonly property bool _bad: _state === "stale"
                readonly property color _ink: _out ? AppPalette.accentText
                                            : _ok  ? AppPalette.linkOkText
                                            : _bad ? AppPalette.linkIdleText
                                                   : AppPalette.textStrong
                implicitWidth: _chipRow.implicitWidth + Tokens.spaceLg * 2
                width: implicitWidth
                height: usblGroup.headerActionSize - Tokens.spaceSm * 2
                radius: height / 2
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                color: _out ? AppPalette.accentBg
                     : _ok  ? AppPalette.linkOkBg
                     : _bad ? AppPalette.linkIdleBg : "transparent"
                border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                border.color: _out ? AppPalette.accentBorder
                            : _ok  ? AppPalette.linkOkBorder
                            : _bad ? AppPalette.linkIdleBorder : AppPalette.border
                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Row {
                    id: _chipRow
                    anchors.centerIn: parent
                    spacing: Math.round(5 * AppPalette.scale)
                    // Hollow until something is known, then it takes the state's colour: an
                    // unfilled dot means "nothing from this address yet", which is a different
                    // thing from amber.
                    Rectangle {
                        width: Math.round(6 * AppPalette.scale); height: width; radius: width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: (addrChip._out || addrChip._ok || addrChip._bad)
                               ? addrChip._ink : "transparent"
                        border.width: (addrChip._out || addrChip._ok || addrChip._bad) ? 0 : 1
                        border.color: AppPalette.textMuted
                    }
                    Text {
                        text: String(addrChip.modelData.addr)
                        color: addrChip._ink
                        font.pixelSize: Tokens.fontXs; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    // ── the schedule: what drives the list ────────────────────────────────
    // Always visible, and first. The schedule is the host's own interrogation loop -- it sends
    // ID_USBL_CONTROL v1 ping requests and needs nothing applied to the device beforehand.
    property var _curStep: null
    property int _stepIndex: -1

    // Interrogations asked for by hand, waiting their turn. See Node.queueEmit.
    property var _manual: []

    // One send path, so a manual interrogation and a scheduled one are indistinguishable to the
    // device, to the poll state and to the row that reports them.
    function _send(s) {
        _curStep = s
        if (!dev) return
        // The window opens here and not before: with no device nothing was transmitted, and a
        // step must not be blamed for failing to answer a request that never left. Keyed by the
        // step's cmd, so four commands on one node succeed or fail independently.
        _poll = Node.noteSent(_poll, s.nodeId, s.cmd, Date.now())
        _waitTimer.restart()

        // Only the schedule is per-node, so only the ping is sent per step.
        var g = s.implicit ? null : plan.groupById(s.groupId)
        var snd = g ? g.ini.send : null
        if (!snd) { dev.acousticPingRequest(s.addr, 0xFFFFFFFF); return }
        var fn = plan.findBy(plan.pingFunctions, snd.fn)
        var payload = (fn.id === "bits") ? snd.payload : ""
        dev.acousticPingRequestEx(s.addr, 0xFFFFFFFF, s.cmd, snd.reply, payload)
    }

    // Build the step a hand-asked interrogation stands for. A muted step is still sendable --
    // "ask this one thing now without putting it in the cycle" is the whole point of the pairing --
    // so this reads the node's refs rather than the schedule, which has already filtered them out.
    function _stepFor(nodeId, cmd) {
        var n = null, i
        for (i = 0; i < plan.nodes.length; ++i) if (plan.nodes[i].id === nodeId) n = plan.nodes[i]
        if (!n) return null
        for (i = 0; i < n.refs.length; ++i)
            if (n.refs[i].cmd === cmd)
                return { addr: n.addr, cmd: cmd, implicit: false,
                         nodeId: nodeId, groupId: n.refs[i].group }
        return { addr: n.addr, cmd: cmd, implicit: true, nodeId: nodeId, groupId: -1 }
    }

    // Asked for by hand. While the schedule runs it takes its turn at the next tick -- which is
    // when the open window has resolved one way or the other. Stopped, it goes as soon as nothing
    // is outstanding, which is usually at once.
    function requestEmit(nodeId, cmd) {
        if (!plan) return
        _manual = Node.queueEmit(_manual, nodeId, cmd)
        if (!_runTimer.running) _drainManual()
    }
    function _drainManual() {
        if (!plan || !_manual.length || _poll.waitKey !== "") return
        var r = Node.dequeueEmit(_manual)
        _manual = r.queue
        var s = _stepFor(r.step.nodeId, r.step.cmd)
        if (s) _send(s)
    }

    function _advance() {
        if (!plan) return
        // Hand-asked interrogations go first and do NOT move the cycle on: a manual shot is an
        // interruption, not a step, and losing your place in the schedule to take one would make
        // the feature cost more than it gives.
        if (_manual.length) {
            var r = Node.dequeueEmit(_manual)
            _manual = r.queue
            var ms = _stepFor(r.step.nodeId, r.step.cmd)
            if (ms) { _send(ms); return }
        }
        var sched = plan.schedule
        if (!sched.length) { _stop(); return }
        _stepIndex = _stepIndex < 0 ? 0 : _stepIndex + 1
        _send(sched[_stepIndex % sched.length])
    }
    // Stop does NOT close an open window. A reply already in the water can still land, and
    // discarding it would report a miss that did not happen.
    function _stop() { _runTimer.stop() }
    function _start() { _runTimer.start(); _advance() }

    // Dwell outlives the session. It is host loop timing rather than plan content, so it is
    // persisted here instead of in the plan blob -- which would need a schema bump and a
    // migration for a number the device never sees.
    property Settings schedPersisted: Settings {
        id: schedSettings
        category: "main/usblSchedule"
        property int dwellMs: 700
    }
    // Stored in MILLISECONDS and shown in seconds: the timers and the answer budget are integer
    // ms, and rounding a float seconds value back into them at every read is a way to be wrong
    // slowly. Clamped on the way out so a value persisted before the floor existed cannot leave
    // the loop running faster than the control can express.
    readonly property int _dwellMs: Math.max(_dwellMinMs, schedSettings.dwellMs)
    // 0.4 s is about 300 m of two-way travel plus turn-around, which is the shortest window that
    // can contain a real answer. Below it every interrogation would time out by construction.
    readonly property int _dwellMinMs: 400

    property Timer _runTimer: Timer {
        interval: usblGroup._dwellMs
        repeat: true
        onTriggered: usblGroup._advance()
    }

    // Step, Start and Dwell on ONE row, because they are one thought: what to send, when to
    // start, how far apart. On two rows the interval sat somewhere you were not looking while
    // deciding whether to press Start.
    //
    // The buttons keep the right edge — where they have been all along, and where the node rows
    // put their own actions. Dwell takes the space the schedule counts used to occupy, and the
    // unit text between them absorbs a narrowing pane so the buttons are never pushed off it.
    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm
        Text {
            id: _dwellLbl
            text: qsTr("Dwell"); color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        KSpinBox {
            id: _dwell
            // Seconds on screen, milliseconds underneath. `to` is an hour because a spin box
            // cannot be unbounded and nothing acoustic comes near it: even a second of dwell is
            // 750 m of two-way travel.
            from: usblGroup._dwellMinMs; to: 3600000; stepSize: 100
            divisor: 1000; decimals: 1
            value: usblGroup._dwellMs
            implicitWidth: Math.round(96 * AppPalette.scale)
            anchors.verticalCenter: parent.verticalCenter
            toolTipText: qsTr("Interval between steps, and the answer window each step is "
                            + "allowed — a reply still counts for %1 s past it")
                         .arg((Node.GRACE_MS / 1000).toFixed(2))
            onValueModified: function (v) { schedSettings.dwellMs = v }
        }
        // Also the spacer: it takes whatever is left, which is what pins the buttons right.
        Text {
            text: qsTr("s between steps")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            width: Math.max(0, parent.width - _dwellLbl.width - _dwell.width
                               - _stepBtn.width - _runBtn.width - parent.spacing * 4)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblButton {
            id: _stepBtn
            text: qsTr("Step"); implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
            anchors.verticalCenter: parent.verticalCenter
            enabled: !!(dev && plan && plan.schedule.length)
            toolTipText: qsTr("Send the next interrogation and stop there")
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

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // The schedule counts head the LIST, not the controls: they describe what is IN the list —
    // how many of these rows get interrogated, and how many interrogations that comes to — so
    // they belong against it rather than against the buttons that start it.
    //
    // "Schedule" alone said nothing when there was nothing to run, so the empty case says why.
    Text {
        width: parent.width
        elide: Text.ElideRight
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm; font.bold: true
        text: (plan && plan.schedule.length)
              ? qsTr("Schedule · %1 nodes · %2 steps").arg(plan.activeNodeCount)
                                                      .arg(plan.schedule.length)
              : qsTr("Schedule · nothing to interrogate")
    }

    // ── the nodes ─────────────────────────────────────────────────────────
    // No sub-heading above the list: the group's own title names it, and a second title would
    // just cost a row. The add affordance ends the list instead.
    Column {
        width: parent.width
        spacing: Tokens.spaceMd

        Text {
            visible: !plan || !plan.nodes.length
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            padding: Tokens.spaceXl
            text: qsTr("No nodes yet. Add one to start interrogating.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.WordWrap
        }

        Repeater {
            // nodesView, not nodes: the view is rebuilt whenever the plan state is replaced, so a
            // step added or removed actually reaches the card. Binding to `nodes` gave each
            // delegate a live node object whose in-place edits QML cannot see.
            model: plan ? plan.nodesView : []
            delegate: Rectangle {
                id: nodeCard
                required property var modelData
                readonly property var _n: modelData

                // The two axes. Arguments are read in the binding expression itself, so every one
                // of them is a tracked dependency -- which is why they are not folded into a
                // single helper that reads them internally.
                //
                // NOTE what `_waiting` is NOT keyed on: whether the run timer is going. One press
                // of Step opens a window with the loop stopped, and keying on the timer is
                // precisely why a single Step used to show Idle throughout.
                readonly property bool _waiting: Node.isWaiting(usblGroup._poll, nodeCard._n.id)
                                                 && nodeCard._n.active
                // Where the cycle is, which outlives the window being open.
                readonly property bool _isCursor: usblGroup._cursorNodeId === nodeCard._n.id
                readonly property string _op: Node.operationCode(nodeCard._n.active,
                                                                 nodeCard._waiting)
                readonly property var _e: Node.entryFor(usblGroup._sols, nodeCard._n.addr)
                // No clock in here. The badge reports what this node's LAST interrogation did,
                // whichever command that was -- so it must not change while nothing is being
                // interrogated, and a command that failed earlier does not keep condemning a node
                // that is answering now. The chips carry the per-command detail.
                readonly property string _reply: Node.nodeReplyCode(
                        usblGroup._poll, nodeCard._n.id, nodeCard._e)
                // Compact for scanning, extended for diagnosing. Per row, so you open the one node
                // you are investigating; opening one never closes another, because a row that
                // shuts by itself is worse than scrolling.
                readonly property bool _open: !!usblGroup._expanded[nodeCard._n.id]
                readonly property real _ageMs: Node.ageMs(nodeCard._e, usblGroup._nowMs)
                readonly property bool _aged: Node.isAged(nodeCard._e, usblGroup._nowMs,
                                                          Node.AGE_WARN_MS)

                width: parent.width
                radius: Tokens.radiusMd
                // TWO MARKS, TWO FACTS.
                //
                // The FRAME follows the cursor: the node last interrogated, or the one the next
                // Step will take. It stays after the answer window closes, because with the cycle
                // strip gone this is the only thing that says where in the schedule you are — and
                // a frame that comes and goes is a frame you cannot use to find the row.
                //
                // The BACKGROUND is a plain light blue while a request is out, and it does not
                // move. Two earlier versions animated it -- opacity, then an alternating fill --
                // and both were noise: with a sub-second window the eye has nothing to track, and
                // the transition in and out already says when it changed.
                color: nodeCard._waiting
                       ? Qt.rgba(AppPalette.accent.r, AppPalette.accent.g, AppPalette.accent.b, 0.14)
                       : AppPalette.rowRaised
                border.width: nodeCard._isCursor ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                                                 : Tokens.cardBorderWidth
                border.color: nodeCard._isCursor ? AppPalette.accentBorder : AppPalette.border
                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                // Switched off still shows its numbers, dimmed: the last thing a node said is
                // worth keeping on screen after you stop asking.
                opacity: _n.active ? 1.0 : 0.62
                implicitHeight: _nodeCol.implicitHeight + 2 * Tokens.spaceMd

                Column {
                    id: _nodeCol
                    x: Tokens.spaceMd; y: Tokens.spaceMd
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: Tokens.spaceSm

                    // identity and state
                    Item {
                        width: parent.width
                        implicitHeight: Tokens.controlHMd

                        Row {
                            id: _idLeft
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Tokens.spaceSm
                            // A chevron, not a gear: this expands, and every settings group in the
                            // app already signals expansion this way. A gear would promise a
                            // settings sheet.
                            Item {
                                width: Math.round(18 * AppPalette.scale)
                                height: width
                                anchors.verticalCenter: parent.verticalCenter
                                DisclosureIndicator {
                                    anchors.centerIn: parent
                                    width: Math.round(10 * AppPalette.scale)
                                    height: width
                                    expanded: nodeCard._open
                                    indicatorColor: AppPalette.textSecond
                                }
                                KTapArea {
                                    anchors.fill: parent
                                    anchors.margins: -Tokens.spaceXs
                                    onTapped: usblGroup._toggleExpanded(nodeCard._n.id)
                                }
                            }
                            MiniSwitch {
                                checked: nodeCard._n.active
                                anchors.verticalCenter: parent.verticalCenter
                                onToggled: if (plan) plan.toggleNode(nodeCard._n.id)
                            }
                            // The header SHOWS the address; editing it lives in the extended view.
                            // A spin box in a scanning list is a mis-click that silently re-points
                            // an interrogation at a different beacon, and an address is set-up-once
                            // data that does not deserve a permanent control.
                            Text {
                                text: qsTr("addr %1").arg(nodeCard._n.addr)
                                color: AppPalette.textStrong
                                font.pixelSize: Tokens.fontSm; font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Badge {
                                code: nodeCard._op
                                label: usblGroup._opText[nodeCard._op] || nodeCard._op
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Badge {
                                reply: true
                                code: nodeCard._reply
                                label: usblGroup._replyText[nodeCard._reply] || nodeCard._reply
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            // How old the numbers are, as a control rather than a caption: the
                            // same chip shape as the badges, outlined in amber once it passes
                            // AGE_WARN_MS. Age lives HERE now — the reply badge deliberately
                            // knows nothing about the clock, so this is the only thing on the row
                            // that changes while nothing is being interrogated.
                            //
                            // In the row rather than anchored across the gap: three chips of one
                            // shape read as one strip, and a positioner cannot overlap the buttons
                            // on the right the way a floating item can.
                            Badge {
                                visible: nodeCard._ageMs >= 0
                                aged: nodeCard._aged
                                label: usblGroup._fmtAge(nodeCard._ageMs)
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Row {
                            id: _idRight
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Tokens.spaceXs
                            KCircleIconButton {
                                glyph: "⧉"
                                width: Tokens.controlHSm; height: Tokens.controlHSm
                                glyphPixelSize: Math.round(13 * AppPalette.scale)
                                borderWidth: Tokens.cardBorderWidth
                                anchors.verticalCenter: parent.verticalCenter
                                enabled: !!(nodeCard._e && nodeCard._e.coordValid)
                                // Depth and elevation live here rather than in the row: three
                                // fixed columns is what makes a list scannable, and these two are
                                // read when you go looking, not while scanning.
                                toolTipText: qsTr("Last known position — copy is not wired up yet")
                                              + "\n" + usblGroup._ll(nodeCard._e)
                                              + "\n" + qsTr("depth %1 %2 · elevation %3")
                                                       .arg(usblGroup._num(nodeCard._e ? nodeCard._e.beaconDepth : NaN, 2))
                                                       .arg(qsTr("m"))
                                                       .arg(usblGroup._num(nodeCard._e ? nodeCard._e.elevation : NaN, 1, "°"))
                                onClicked: usblGroup._copyCoordinate(nodeCard._n.addr,
                                                                     usblGroup._ll(nodeCard._e))
                            }
                            KCircleIconButton {
                                glyph: "×"
                                width: Tokens.controlHSm; height: Tokens.controlHSm
                                glyphPixelSize: Math.round(14 * AppPalette.scale)
                                borderWidth: Tokens.cardBorderWidth
                                anchors.verticalCenter: parent.verticalCenter
                                toolTipText: qsTr("Remove node")
                                onClicked: if (plan) plan.removeNode(nodeCard._n.id)
                            }
                        }

                    }

                    // what this node last reported
                    Row {
                        width: parent.width
                        spacing: Tokens.spaceXs
                        readonly property real cellW: (width - spacing * 2) / 3
                        MiniStat {
                            width: parent.cellW
                            value: usblGroup._num(nodeCard._e ? nodeCard._e.distance : NaN, 1)
                            label: qsTr("Distance, m")
                        }
                        MiniStat {
                            width: parent.cellW
                            value: usblGroup._num(nodeCard._e ? nodeCard._e.azimuth : NaN, 1, "°")
                            label: qsTr("Azimuth")
                        }
                        MiniStat {
                            width: parent.cellW
                            value: usblGroup._num(nodeCard._e ? nodeCard._e.snr : NaN, 0)
                            label: qsTr("SNR, dB")
                        }
                    }

                    // ── what gets sent to it ──────────────────────────────
                    // COMPACT: read and fire. Chips show each command's own fate, a dot shows
                    // whether the schedule includes it, and a tap interrogates it now. No delete
                    // here on purpose -- a delete target adjacent to a chip that transmits is a
                    // mis-click that costs you a step, and deletion is not something you do while
                    // scanning. It lives in the extended view, with adding.
                    Flow {
                        width: parent.width
                        spacing: Tokens.spaceXs
                        visible: !nodeCard._open

                        Text {
                            visible: !nodeCard._n.steps.length
                            text: qsTr("no commands — interrogated once with cmd 0")
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                        }

                        Repeater {
                            model: nodeCard._n.steps
                            delegate: Rectangle {
                                // Needs an id: QML property lookup goes own-properties -> ids in
                                // the component -> component root. An ANCESTOR's properties are
                                // not in scope, so a bare `_cur` inside the Texts below resolves
                                // to nothing at runtime.
                                id: _refChip
                                required property var modelData
                                required property int index

                                // A chip carries its OWN interrogation's fate, because that is the
                                // question the row cannot answer: a node goes stale and only the
                                // chip says which command went unanswered.
                                //
                                // Four marks, none competing for the same property:
                                //   frame       -- this is the step the cycle is on
                                //   blue fill   -- its request is out right now
                                //   green/amber -- how its last interrogation ended
                                //   dot         -- whether the schedule includes it at all
                                // So a framed chip still shows its result, and a chip being
                                // re-asked still shows the previous one until this resolves.
                                readonly property bool _cur: Node.isCursorStep(
                                        usblGroup._curStep, plan ? plan.schedule : [],
                                        nodeCard._n.id, modelData.cmd)
                                readonly property string _state: Node.stepCode(
                                        usblGroup._poll, nodeCard._n.id, modelData.cmd)
                                readonly property bool _out: _state === "waiting"
                                readonly property bool _ok:  _state === "replied"
                                readonly property bool _bad: _state === "stale"
                                readonly property bool _queued: Node.isQueued(
                                        usblGroup._manual, nodeCard._n.id, modelData.cmd)

                                radius: Tokens.radiusSm
                                color: _out ? AppPalette.accentBg
                                     : _ok  ? AppPalette.linkOkBg
                                     : _bad ? AppPalette.linkIdleBg
                                     : _chipTap.containsMouse ? AppPalette.chipRaisedHover
                                                              : AppPalette.card
                                // The frame outranks the result's own outline: it marks position,
                                // which the result must not be able to hide.
                                border.width: _cur ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                                                   : Tokens.cardBorderWidth
                                border.color: _cur ? AppPalette.accentBorder
                                            : _out ? AppPalette.accentBorder
                                            : _ok  ? AppPalette.linkOkBorder
                                            : _bad ? AppPalette.linkIdleBorder : AppPalette.border
                                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                implicitWidth: _chip.implicitWidth + Tokens.spaceMd * 2
                                implicitHeight: Math.round(24 * AppPalette.scale)

                                Row {
                                    id: _chip
                                    anchors.centerIn: parent
                                    spacing: Tokens.spaceXs
                                    // Filled = in the schedule, hollow = muted. A dot rather than
                                    // dimming: dimming would fight both the state fill and the
                                    // row's own switched-off dimming.
                                    Rectangle {
                                        width: Math.round(5 * AppPalette.scale); height: width
                                        radius: width / 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: modelData.on ? (_refChip._out ? AppPalette.accentText
                                                                             : AppPalette.textStrong)
                                                            : "transparent"
                                        border.width: modelData.on ? 0 : 1
                                        border.color: _refChip._out ? AppPalette.accentText
                                                                    : AppPalette.textMuted
                                    }
                                    Text {
                                        text: "cmd " + modelData.cmd
                                        color: _refChip._out ? AppPalette.accentText
                                             : _refChip._ok  ? AppPalette.linkOkText
                                             : _refChip._bad ? AppPalette.linkIdleText
                                                             : AppPalette.textStrong
                                        font.pixelSize: Tokens.fontXs; font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                // The whole chip fires it. A muted command is still fireable --
                                // "ask this one thing now without putting it in the cycle" is the
                                // point of pairing the two.
                                MouseArea {
                                    id: _chipTap
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    enabled: !!dev
                                    onClicked: usblGroup.requestEmit(nodeCard._n.id, modelData.cmd)
                                }
                                KToolTip {
                                    text: _refChip._queued
                                          ? qsTr("queued — goes out when the current answer window closes")
                                          : qsTr("Interrogate cmd %1 now").arg(modelData.cmd)
                                    targetItem: _refChip
                                    shown: _chipTap.containsMouse && !!dev
                                }
                            }
                        }
                    }

                    // EXTENDED: one command per line, because that is where a command has room to
                    // say what it is doing and to be acted on individually.
                    Column {
                        width: parent.width
                        spacing: Tokens.spaceXs
                        visible: nodeCard._open

                        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

                        // Editing the address lives here, not in the header.
                        Row {
                            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm
                            Text {
                                text: qsTr("Address")
                                color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            UsblSpin {
                                from: 0; to: 8; stepSize: 1
                                devValue: nodeCard._n.addr
                                implicitWidth: Math.round(78 * AppPalette.scale)
                                anchors.verticalCenter: parent.verticalCenter
                                writeBack: function (v) { if (plan) plan.setNodeAddr(nodeCard._n.id, v) }
                            }
                            Text {
                                text: qsTr("Commands")
                                color: AppPalette.textMuted
                                font.pixelSize: Tokens.fontSm; font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            visible: !nodeCard._n.steps.length
                            width: parent.width
                            text: qsTr("No commands. This node is interrogated once with cmd 0.")
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            wrapMode: Text.WordWrap
                        }

                        Repeater {
                            model: nodeCard._n.steps
                            delegate: Rectangle {
                                id: _cmdRow
                                required property var modelData
                                required property int index

                                readonly property bool _cur: Node.isCursorStep(
                                        usblGroup._curStep, plan ? plan.schedule : [],
                                        nodeCard._n.id, modelData.cmd)
                                readonly property string _state: Node.stepCode(
                                        usblGroup._poll, nodeCard._n.id, modelData.cmd)
                                readonly property bool _out: _state === "waiting"
                                readonly property bool _ok:  _state === "replied"
                                readonly property bool _bad: _state === "stale"
                                readonly property bool _queued: Node.isQueued(
                                        usblGroup._manual, nodeCard._n.id, modelData.cmd)
                                // The one retained payload, and only if it names THIS command.
                                readonly property bool _hasData: usblGroup._modemMatches(
                                        nodeCard._n.addr, modelData.cmd)

                                width: parent.width
                                radius: Tokens.radiusSm
                                // Same state language as the chips, so the two views cannot be
                                // read differently.
                                color: _out ? AppPalette.accentBg
                                     : _ok  ? AppPalette.linkOkBg
                                     : _bad ? AppPalette.linkIdleBg : AppPalette.card
                                border.width: _cur ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                                                   : Tokens.cardBorderWidth
                                border.color: _cur ? AppPalette.accentBorder
                                            : _out ? AppPalette.accentBorder
                                            : _ok  ? AppPalette.linkOkBorder
                                            : _bad ? AppPalette.linkIdleBorder : AppPalette.border
                                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                implicitHeight: Math.round(30 * AppPalette.scale)

                                Row {
                                    id: _cmdLeft
                                    anchors.left: parent.left
                                    anchors.leftMargin: Tokens.spaceSm
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: Tokens.spaceSm
                                    // In the schedule or not. An icon rather than a switch: a
                                    // switch in a row that already has the node's own switch reads
                                    // as a second copy of it, and this is a different thing --
                                    // the node's switch stops the node, this mutes one command and
                                    // keeps its place in the cycle.
                                    //
                                    // The cycle glyph says what membership means; colour and
                                    // opacity say which way it is set.
                                    KCircleIconButton {
                                        glyph: "⟳"
                                        width: Tokens.controlHSm; height: Tokens.controlHSm
                                        glyphPixelSize: Math.round(13 * AppPalette.scale)
                                        glyphColor: _cmdRow.modelData.on ? AppPalette.accentBorder
                                                                         : AppPalette.textMuted
                                        borderWidth: _cmdRow.modelData.on
                                                     ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                                                     : Tokens.cardBorderWidth
                                        borderColor: _cmdRow.modelData.on ? AppPalette.accentBorder
                                                                          : AppPalette.border
                                        opacity: _cmdRow.modelData.on ? 1.0 : 0.55
                                        anchors.verticalCenter: parent.verticalCenter
                                        toolTipText: _cmdRow.modelData.on
                                            ? qsTr("In the schedule — click to mute this command")
                                            : qsTr("Muted — the schedule skips it. Click to include it again")
                                        onClicked: if (plan) plan.toggleStep(nodeCard._n.id,
                                                                            _cmdRow.index)
                                    }
                                    Text {
                                        text: "cmd " + _cmdRow.modelData.cmd
                                        color: _cmdRow._out ? AppPalette.accentText
                                             : _cmdRow._ok  ? AppPalette.linkOkText
                                             : _cmdRow._bad ? AppPalette.linkIdleText
                                                            : AppPalette.textStrong
                                        font.pixelSize: Tokens.fontSm; font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                Row {
                                    id: _cmdRight
                                    anchors.right: parent.right
                                    anchors.rightMargin: Tokens.spaceSm
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: Tokens.spaceXs
                                    KCircleIconButton {
                                        glyph: "▶"
                                        width: Tokens.controlHSm; height: Tokens.controlHSm
                                        glyphPixelSize: Math.round(10 * AppPalette.scale)
                                        borderWidth: _cmdRow._queued
                                                     ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                                                     : Tokens.cardBorderWidth
                                        borderColor: _cmdRow._queued ? AppPalette.accentBorder
                                                                     : AppPalette.border
                                        anchors.verticalCenter: parent.verticalCenter
                                        enabled: !!dev
                                        toolTipText: _cmdRow._queued
                                            ? qsTr("queued — goes out when the current answer window closes")
                                            : qsTr("Interrogate this command now")
                                        onClicked: usblGroup.requestEmit(nodeCard._n.id,
                                                                        _cmdRow.modelData.cmd)
                                    }
                                    KCircleIconButton {
                                        glyph: "×"
                                        width: Tokens.controlHSm; height: Tokens.controlHSm
                                        glyphPixelSize: Math.round(13 * AppPalette.scale)
                                        borderWidth: Tokens.cardBorderWidth
                                        anchors.verticalCenter: parent.verticalCenter
                                        toolTipText: qsTr("Remove this command from the node")
                                        onClicked: if (plan) plan.removeStep(nodeCard._n.id,
                                                                            _cmdRow.index)
                                    }
                                }

                                // What came back. See usblGroup._modemMatches: the app keeps ONE
                                // modem payload, so at most one row in the pane can fill this in.
                                // The line is reserved on every row so the answer does not move
                                // rows of different heights around as frames arrive.
                                Text {
                                    anchors.left: _cmdLeft.right
                                    anchors.leftMargin: Tokens.spaceMd
                                    anchors.right: _cmdRight.left
                                    anchors.rightMargin: Tokens.spaceMd
                                    anchors.verticalCenter: parent.verticalCenter
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideMiddle
                                    font.pixelSize: Tokens.fontXs
                                    font.family: "monospace"
                                    color: _cmdRow._hasData ? AppPalette.textStrong
                                                            : AppPalette.textMuted
                                    text: {
                                        var _t = usblGroup.clockTick
                                        if (!_cmdRow._hasData) return "—"
                                        var age = usblGroup._modemAtMs > 0
                                                  ? usblGroup._fmtAge(Math.max(0, Date.now() - usblGroup._modemAtMs))
                                                  : ""
                                        return dev.modemLastPayload
                                               + qsTr(" · %1 bit").arg(dev.modemLastBitLength)
                                               + (age.length ? " · " + age : "")
                                    }
                                }
                            }
                        }

                        // Adding a command ends the list, like adding a node ends the node list.
                        Item {
                            width: parent.width
                            implicitHeight: _addCmd.height + Tokens.spaceXs
                            KCombo {
                                id: _addCmd
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.topMargin: Tokens.spaceXs
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
                                implicitWidth: Math.round(120 * AppPalette.scale)
                                fontPixelSize: Tokens.fontXs
                                onActivated: function (i) {
                                    if (i > 0 && plan) plan.addStep(nodeCard._n.id, _avail[i - 1].id)
                                    currentIndex = 0
                                }
                            }
                            Text {
                                anchors.left: _addCmd.right
                                anchors.leftMargin: Tokens.spaceSm
                                anchors.right: parent.right
                                anchors.verticalCenter: _addCmd.verticalCenter
                                text: qsTr("only the newest received payload is kept, so it shows on one command at a time")
                                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        // Ends the list rather than heading it: this pane's subject IS the list, so the add
        // affordance belongs where the list runs out.
        Rectangle {
            width: parent.width
            implicitHeight: Tokens.controlHMd
            radius: Tokens.radiusMd
            color: "transparent"
            border.width: Math.max(1, Math.round(1 * AppPalette.scale))
            border.color: AppPalette.border
            Text {
                anchors.centerIn: parent
                text: qsTr("+ Add node")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm; font.bold: true
            }
            KTapArea { anchors.fill: parent; onTapped: if (plan) plan.addNode() }
        }
    }
}

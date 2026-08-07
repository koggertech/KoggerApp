import QtQuick 2.15
import QtCore
import "UsblNodeLogic.js" as Node

// The host's acoustic interrogation loop, and the state it produces.
//
// WHY THIS IS NOT IN UsblGroup ANY MORE. It used to be, and UsblGroup lives in
// DeviceSettingsPage, which is a settings SUB-PAGE: the loader swaps its component out the
// moment the operator navigates anywhere else (MainWindow.qml -> subPage, SettingsSidebarBase
// -> subLoader). Leaving the page therefore destroyed the schedule timer, the poll state and
// the plan store together -- interrogation stopped, silently, and nothing said so. That was
// tolerable while the only thing reading them was the pane itself. It is not tolerable now
// that a widget panel on the scene reports them, because the scene is exactly what the
// operator is looking at when the settings panel is closed.
//
// So this is declared in MainWindow, beside WorkspaceStore, and lives for the session. The
// pane became a view over it.
//
// WHAT IS NOT PERSISTED: whether the loop is running. A survey session starts stopped. A
// schedule that resumed transmitting on launch, with no USBL UI on screen to say so, is a
// head talking to the water because of a setting nobody remembers making.
//
// The rules -- when a window opens, what closes it, what an unanswered one means -- are all
// in UsblNodeLogic.js and asserted by tools/qml_test/test_usbl_node_logic.mjs. This file
// drives them; it decides nothing.
QtObject {
    id: engine

    property var plan: null

    // The device the pane has selected, when there is a pane and it selected a USBL one.
    // Bound from WorkspaceStore.activeDevice, which is store state and outlives the page.
    property var preferredDev: null

    // WHICH DEVICE THE LOOP TALKS TO. The selection if it is acoustic, otherwise the first
    // acoustic device on the bus -- because with the settings page closed there is no
    // selection to speak of, and "the head that is plugged in" is the only honest answer.
    // With one USBL head, which is the normal case, both branches name it.
    readonly property var dev: {
        if (preferredDev && (preferredDev.isUSBL || preferredDev.isUSBLBeacon))
            return preferredDev
        var ds = (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper)
                 ? deviceManagerWrapper.devs : []
        for (var i = 0; i < ds.length; ++i)
            if (ds[i] && (ds[i].isUSBL || ds[i].isUSBLBeacon)) return ds[i]
        return null
    }
    readonly property bool hasDevice: !!dev

    // ── the interrogation cycle ───────────────────────────────────────────
    // Which step's window is open, what every step last did, and which command was last asked
    // of each node. Replaced wholesale by the reducer, never edited here.
    property var poll: Node.initialPoll()
    property var manual: []
    property var curStep: null
    property int stepIndex: -1

    readonly property bool running: _runTimer.running

    // One row per node, composed by the logic module so the pane and the on-scene panel cannot
    // describe the same beacon differently. Every input is read in this expression, which is
    // what makes it a tracked dependency -- folding them into a helper that reads them
    // internally would freeze the binding.
    readonly property var _sols: (typeof dataset !== "undefined" && dataset && dataset.usblSolutions)
                                 ? dataset.usblSolutions : ({})
    readonly property var rows: Node.panelRows(plan ? plan.nodes : [], _sols, poll, nowMs,
                                               curStep, plan ? plan.schedule : [])

    // 1 s clock, for ages only. epochMs is constant per fix, so the ticking dependency has to
    // come from outside: Date.now() inside a binding is evaluated once and then frozen.
    //
    // It does NOT drive the window transitions. A window can be shorter than one tick, so
    // sampling at 1 Hz would miss it entirely.
    property string clockTick: ""
    readonly property real nowMs: { var _t = clockTick; return Date.now() }

    property Timer _clockTimer: Timer {
        interval: 1000
        repeat: true
        running: engine.hasDevice
        onTriggered: engine.clockTick = String(Date.now())
    }

    // The deadline. Non-repeating and restarted on every request: while the schedule runs the
    // next request closes the window first, so this only decides the single-Step case and the
    // last step before Stop.
    property Timer _waitTimer: Timer {
        interval: Node.waitMs(engine.dwellMs, Node.GRACE_MS)
        repeat: false
        onTriggered: {
            engine.poll = Node.noteTimeout(engine.poll)
            engine.drainManual()
        }
    }

    // One signal per arriving solution (Dataset::addUsblSolution emits it after updating the
    // per-address cache), which is what lets a window close the instant its reply lands
    // instead of on the next clock tick.
    property Connections _solutions: Connections {
        target: (typeof dataset !== "undefined") ? dataset : null
        function onLastUsblSolutionChanged() {
            if (!engine.plan) return
            var wasOpen = engine.poll.waitNodeId
            // The clock goes in here because this is where a reply is ATTRIBUTED to a node, and
            // that is the instant the row's age counts from -- not when the solution landed in
            // Dataset, which happens for late replies too.
            engine.poll = Node.noteReplyAddr(engine.poll, engine.plan.nodes,
                                             dataset.lastUsblAddress, Date.now())
            if (wasOpen >= 0 && engine.poll.waitNodeId < 0) {
                engine._waitTimer.stop()
                // The window just closed, so anything asked for by hand can go now rather than
                // waiting for a scheduler that may not be running.
                engine.drainManual()
            }
        }
    }

    // A head that has gone away cannot answer, and a loop still ticking at one would mark every
    // step stale for as long as it is unplugged.
    onHasDeviceChanged: if (!hasDevice) stop()

    // ── the schedule ──────────────────────────────────────────────────────
    // Dwell outlives the session. It is host loop timing rather than plan content, so it is
    // persisted here instead of in the plan blob -- which would need a schema bump and a
    // migration for a number the device never sees.
    property Settings _schedPersisted: Settings {
        id: schedSettings
        category: "main/usblSchedule"
        property int dwellMs: 700
    }
    // Stored in MILLISECONDS and shown in seconds: the timers and the answer budget are integer
    // ms, and rounding a float seconds value back into them at every read is a way to be wrong
    // slowly. Clamped on the way out so a value persisted before the floor existed cannot leave
    // the loop running faster than the control can express.
    readonly property int dwellMs: Math.max(dwellMinMs, schedSettings.dwellMs)
    // 0.4 s is about 300 m of two-way travel plus turn-around, which is the shortest window that
    // can contain a real answer. Below it every interrogation would time out by construction.
    readonly property int dwellMinMs: 400
    function setDwellMs(v) { schedSettings.dwellMs = v }

    property Timer _runTimer: Timer {
        interval: engine.dwellMs
        repeat: true
        onTriggered: engine.advance()
    }

    readonly property bool canRun: !!(dev && plan && plan.schedule.length)

    // ── sending ───────────────────────────────────────────────────────────
    // One send path, so a manual interrogation and a scheduled one are indistinguishable to the
    // device, to the poll state and to the row that reports them.
    function _send(s) {
        curStep = s
        if (!dev) return
        // The window opens here and not before: with no device nothing was transmitted, and a
        // step must not be blamed for failing to answer a request that never left. Keyed by the
        // step's cmd, so four commands on one node succeed or fail independently.
        poll = Node.noteSent(poll, s.nodeId, s.cmd, Date.now())
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
    // "ask this one thing now without putting it in the cycle" is the whole point of the
    // pairing -- so this reads the node's refs rather than the schedule, which has already
    // filtered them out.
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
    // when the open window has resolved one way or the other. Stopped, it goes as soon as
    // nothing is outstanding, which is usually at once.
    function requestEmit(nodeId, cmd) {
        if (!plan) return
        manual = Node.queueEmit(manual, nodeId, cmd)
        if (!_runTimer.running) drainManual()
    }
    function drainManual() {
        if (!plan || !manual.length || poll.waitKey !== "") return
        var r = Node.dequeueEmit(manual)
        manual = r.queue
        var s = _stepFor(r.step.nodeId, r.step.cmd)
        if (s) _send(s)
    }

    function advance() {
        if (!plan) return
        // Hand-asked interrogations go first and do NOT move the cycle on: a manual shot is an
        // interruption, not a step, and losing your place in the schedule to take one would make
        // the feature cost more than it gives.
        if (manual.length) {
            var r = Node.dequeueEmit(manual)
            manual = r.queue
            var ms = _stepFor(r.step.nodeId, r.step.cmd)
            if (ms) { _send(ms); return }
        }
        var sched = plan.schedule
        if (!sched.length) { stop(); return }
        stepIndex = stepIndex < 0 ? 0 : stepIndex + 1
        _send(sched[stepIndex % sched.length])
    }

    // Stop does NOT close an open window. A reply already in the water can still land, and
    // discarding it would report a miss that did not happen.
    function stop()  { _runTimer.stop() }
    function start() { _runTimer.start(); advance() }
    function step()  { stop(); advance() }
    function toggleRun() { running ? stop() : start() }
}

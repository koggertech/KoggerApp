import QtQuick 2.15
import QtQml
import "UsblNodeLogic.js" as Node
import "NodesCompile.js" as Compile
import "NodesPayload.js" as Payload

QtObject {
    id: engine

    property var store: null
    property var exclusiveWith: null

    // -1 runs every head's steps; a lifeline id runs only that head's.
    property int scope: -1
    property var poll: Node.initialPoll()
    property var manual: []
    property var curStep: null
    property int stepIndex: -1
    property bool _running: false
    readonly property bool running: _running

    // stepId -> { state: waiting|replied|missed|skipped, sentAt, atMs, count, replies, and from the last reply:
    //   distance, azimuth, elevation, snr (RSRQ), sync { val, snr, firstVal, firstSnr, rms, leadMs }, beacon { lat, lon, depth } }
    property var stepLog: ({})

    // "exchangeId/leg/lifelineId" -> the last ModemSolution attributed to that data leg:
    // { lifelineId, stepId, exchangeId, leg, hex, bits, event, addr, cmd, atMs, text,
    //   verdict: match|differs, expectedHex, expectedBits, expectedText, differing (-1: length differs) }
    property var rxLog: ({})
    // lifelineId -> the last ModemSolution its device reported, attributed or not (verdict "unattributed").
    property var rxLast: ({})

    readonly property var _all: store ? Compile.stepsOf(store.compiled) : []
    readonly property var runnable: {
        var out = []
        for (var i = 0; i < _all.length; ++i) {
            var s = _all[i]
            if (scope >= 0 && s.initiatorId !== scope) continue
            if (s.addr < 0) continue
            if (store && store.deviceFor(s.initiatorId)) out.push(s)
        }
        return out
    }
    function runnableFor(lifelineId) {
        var out = []
        for (var i = 0; i < _all.length; ++i)
            if (_all[i].initiatorId === lifelineId && _all[i].addr >= 0 && store && store.deviceFor(lifelineId)) out.push(_all[i])
        return out
    }
    readonly property bool canRun: runnable.length > 0

    readonly property var dev: {
        if (!store) return null
        if (curStep) { var d = store.deviceFor(curStep.initiatorId); if (d) return d }
        for (var i = 0; i < runnable.length; ++i) { var x = store.deviceFor(runnable[i].initiatorId); if (x) return x }
        return null
    }
    readonly property bool hasDevice: !!dev

    readonly property var targets: store ? store.compiled.targets : []
    readonly property var _schedule: runnable.map(function (s) { return { nodeId: s.nodeId, cmd: s.cmd } })
    readonly property var _sols: (typeof dataset !== "undefined" && dataset && dataset.usblSolutions) ? dataset.usblSolutions : ({})
    readonly property var rows: Node.panelRows(targets, _sols, poll, nowMs, curStep, _schedule)

    property string clockTick: ""
    readonly property real nowMs: { var _t = clockTick; return Date.now() }
    readonly property int cycleMs: {
        var t = 0
        for (var i = 0; i < runnable.length; ++i) t += runnable[i].durationMs
        return t
    }

    property Timer _clock: Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: engine.clockTick = String(Date.now())
    }

    property Timer _stepTimer: Timer {
        repeat: false
        onTriggered: engine.advance()
    }

    property Timer _waitTimer: Timer {
        repeat: false
        onTriggered: {
            var s = engine.curStep
            engine.poll = Node.noteTimeout(engine.poll)
            if (s) engine._log(s.stepId, { state: "missed", atMs: Date.now() })
            engine._drainManual()
        }
    }

    property Connections _solutions: Connections {
        target: (typeof dataset !== "undefined") ? dataset : null
        function onLastUsblSolutionChanged() {
            var wasOpen = engine.poll.waitNodeId
            var s = engine.curStep
            engine.poll = Node.noteReplyAddr(engine.poll, engine.targets, dataset.lastUsblAddress, Date.now())
            if (wasOpen !== -1 && engine.poll.waitNodeId === -1) {
                engine._waitTimer.stop()
                var sols = dataset.usblSolutions || {}
                var e = sols[String(dataset.lastUsblAddress)] || {}
                if (s) engine._log(s.stepId, { state: "replied", atMs: Date.now(),
                                               distance: dataset.lastUsblDistance,
                                               azimuth: dataset.lastUsblAzimuth,
                                               elevation: dataset.lastUsblElevation,
                                               snr: dataset.lastUsblSnr,
                                               sync: { val: e.syncVal, snr: e.syncSnr, firstVal: e.syncFirstVal,
                                                       firstSnr: e.syncFirstSnr, rms: e.syncRms, leadMs: e.syncFirstMainMaxMs },
                                               beacon: e.coordValid ? { lat: e.beaconLat, lon: e.beaconLon, depth: e.beaconDepth } : null,
                                               replied: true })
                engine._drainManual()
            }
        }
    }

    property Instantiator _modemWatch: Instantiator {
        model: engine.store ? engine.store.devices : []
        delegate: Connections {
            required property var modelData
            target: modelData
            ignoreUnknownSignals: true
            function onModemPayloadChanged() { engine._onModem(modelData) }
        }
    }

    property Connections _networkWatch: Connections {
        target: engine.store
        ignoreUnknownSignals: true
        function onNetworkReplaced() {
            engine.stepLog = {}
            engine.rxLog = {}
            engine.rxLast = {}
        }
    }

    onHasDeviceChanged: if (!hasDevice && _running) stop()

    function _with(map, key, value) {
        var n = {}
        for (var k in map) n[k] = map[k]
        n[key] = value
        return n
    }

    function _expected(leg) {
        var s = curStep
        if (leg.leg === "request" && s && s.exchangeId === leg.exchangeId) {
            var hex = Payload.normalizeHex(s.payloadHex)
            if (hex.length) return { hex: hex, bits: s.payloadBits > 0 ? s.payloadBits : hex.length * 4 }
        }
        return { hex: leg.hex, bits: leg.bits }
    }

    function _onModem(dev) {
        if (!store || !dev) return
        var l = store.lifelineForDevice(dev)
        if (!l) return
        var ev = dev.modemLastEvent, addr = dev.modemLastAddressFrom, cmd = dev.modemLastCmdId
        var bits = dev.modemLastBitLength
        var hex = Payload.normalizeHex(dev.modemLastPayload)
        var legs = Compile.legsAt(store.dataLegs, l.id, ev, addr, cmd)
        var leg = legs.length ? legs[0] : null
        for (var i = 0; i < legs.length; ++i)
            if (curStep && legs[i].exchangeId === curStep.exchangeId) leg = legs[i]
        var e = { lifelineId: l.id, stepId: -1, exchangeId: -1, leg: "", hex: hex, bits: bits,
                  event: ev, addr: addr, cmd: cmd, atMs: Date.now(), text: Payload.prettyHex(hex),
                  verdict: "unattributed", expectedHex: "", expectedBits: 0, expectedText: "", differing: 0 }
        if (leg) {
            var exp = _expected(leg)
            e.stepId = curStep && curStep.exchangeId === leg.exchangeId ? curStep.stepId : -1
            e.exchangeId = leg.exchangeId
            e.leg = leg.leg
            e.text = Payload.decode(hex, bits, leg.data, store.formats).text
            e.expectedHex = exp.hex
            e.expectedBits = exp.bits
            e.expectedText = Payload.decode(exp.hex, exp.bits, leg.data, store.formats).text
            e.differing = bits === exp.bits ? Payload.differingBits(hex, exp.hex, bits) : -1
            e.verdict = e.differing === 0 ? "match" : "differs"
            rxLog = _with(rxLog, leg.exchangeId + "/" + leg.leg + "/" + l.id, e)
        }
        rxLast = _with(rxLast, l.id, e)
    }

    /** Data legs of an exchange: where its bit-array payloads are decoded. */
    function legsOf(exchangeId) {
        return store ? store.dataLegs.filter(function (g) { return g.exchangeId === exchangeId }) : []
    }

    /** The last decoded data of a leg at one lifeline, whenever it came, or null. */
    function rxFor(exchangeId, leg, lifelineId) {
        return rxLog[exchangeId + "/" + leg + "/" + lifelineId] || null
    }

    /**
     * What the device on `lifelineId` decoded for a step's data leg since the step was last sent:
     * { state: match | differs | waiting | missing | "", entry }. "" when nothing can be said:
     * the step has not run here, the receiver is not connected, or the reply itself was missed.
     */
    function dataState(stepId, leg, lifelineId) {
        var none = { state: "", entry: null }
        var L = stepLog[stepId]
        if (!L || !L.sentAt || L.state === "skipped" || !store || !store.deviceFor(lifelineId)) return none
        var s = _stepById(stepId)
        if (!s) return none
        var e = rxLog[s.exchangeId + "/" + leg + "/" + lifelineId]
        if (e && e.atMs >= L.sentAt && (e.stepId < 0 || e.stepId === stepId)) return { state: e.verdict, entry: e }
        if ((leg === "response" || leg === "followUpReply") && L.state === "missed") return none
        var open = !!curStep && curStep.stepId === stepId && nowMs - L.sentAt < s.requiredMs + Node.GRACE_MS
        return { state: open ? "waiting" : "missing", entry: null }
    }

    function _log(stepId, patch) {
        var n = {}
        for (var k in stepLog) n[k] = stepLog[k]
        var prev = n[stepId] || { count: 0, replies: 0 }
        var e = {}
        for (var p in prev) e[p] = prev[p]
        for (var q in patch) e[q] = patch[q]
        if (patch.state === "waiting") e.count = (prev.count || 0) + 1
        if (patch.replied) e.replies = (prev.replies || 0) + 1
        delete e.replied
        n[stepId] = e
        stepLog = n
    }

    function _send(s) {
        var d = store ? store.deviceFor(s.initiatorId) : null
        curStep = s
        if (!d || (typeof d.usblControlReady === "function" && !d.usblControlReady())) {
            _log(s.stepId, { state: "skipped", atMs: Date.now() })
            return
        }
        if (s.kind === "ping") {
            if (poll.waitKey) { poll = Node.noteTimeout(poll); _waitTimer.stop() }
            _log(s.stepId, { state: "pinged", sentAt: Date.now() })
        } else {
            poll = Node.noteSent(poll, s.nodeId, s.cmd, Date.now())
            _log(s.stepId, { state: "waiting", sentAt: Date.now() })
            _waitTimer.interval = Math.max(200, s.requiredMs + Node.GRACE_MS)
            _waitTimer.restart()
        }
        d.acousticPingRequestHex(s.addr, s.triggerUs, s.cmd, s.replyMm, s.payloadHex, s.payloadBits)
    }

    function _stepById(stepId) {
        if (!store) return null
        var all = store.compiled.steps
        for (var i = 0; i < all.length; ++i) if (all[i].stepId === stepId) return all[i]
        return null
    }

    /** Asks one step now; it waits for an open answer window to close. */
    function interrogateNow(stepId) {
        var s = _stepById(stepId)
        if (!s) return
        var q = manual.filter(function (m) { return m.stepId !== stepId })
        q.push({ stepId: stepId })
        manual = q
        if (!_running) _drainManual()
    }

    /** Sends one ping for an exchange's step with ad-hoc request data { hex, bits }. */
    function sendWithPayload(stepId, hex, bits) {
        var s = _stepById(stepId)
        if (!s) return
        var copy = {}
        for (var k in s) copy[k] = s[k]
        copy.payloadHex = hex || ""
        copy.payloadBits = (typeof bits === "number") ? bits : -1
        var q = manual.slice()
        q.push({ step: copy })
        manual = q
        if (!_running) _drainManual()
    }

    function _drainManual() {
        if (_running || !manual.length || poll.waitKey !== "") return
        var q = manual.slice()
        var m = q.shift()
        manual = q
        var s = m.step ? m.step : _stepById(m.stepId)
        if (s) _send(s)
    }

    function advance() {
        if (!_running) return
        if (manual.length && poll.waitKey === "") {
            var q = manual.slice(), m = q.shift()
            manual = q
            var ms = m.step ? m.step : _stepById(m.stepId)
            if (ms) {
                _send(ms)
                _stepTimer.interval = Math.max(200, ms.durationMs)
                _stepTimer.restart()
                return
            }
        }
        var list = runnable
        if (!list.length) { stop(); return }
        stepIndex = (stepIndex + 1) % list.length
        if (stepIndex === 0 && store && !store.st.cycle.repeat && _cycleDone) { stop(); return }
        if (stepIndex === list.length - 1) _cycleDone = true
        var s = list[stepIndex]
        _send(s)
        _stepTimer.interval = Math.max(200, s.durationMs)
        _stepTimer.restart()
    }
    property bool _cycleDone: false

    /** Starts the cycle: every head (-1) or one head's steps. */
    function start(lifelineId) {
        if (exclusiveWith && exclusiveWith.running) exclusiveWith.stop()
        scope = (typeof lifelineId === "number") ? lifelineId : -1
        stepIndex = -1
        _cycleDone = false
        _running = true
        advance()
    }
    function stop() {
        _running = false
        _stepTimer.stop()
    }
    function step(lifelineId) {
        if (exclusiveWith && exclusiveWith.running) exclusiveWith.stop()
        if (typeof lifelineId === "number") scope = lifelineId
        stop()
        var list = runnable
        if (!list.length) return
        stepIndex = (stepIndex + 1) % list.length
        _send(list[stepIndex])
    }
    function toggleRun() { running ? stop() : start(scope) }
    function isRunningFor(lifelineId) { return _running && (scope < 0 || scope === lifelineId) }

    // UsblEngine-compatible surface for the scene panel.
    readonly property int dwellMs: cycleMs
    function requestEmit(nodeId, cmd) {
        for (var i = 0; i < runnable.length; ++i)
            if (runnable[i].nodeId === nodeId && runnable[i].cmd === cmd) { interrogateNow(runnable[i].stepId); return }
    }
}

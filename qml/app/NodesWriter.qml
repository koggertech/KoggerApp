import QtQuick 2.15
import "NodesCompile.js" as Compile

QtObject {
    id: writer

    property var store: null
    property var queue: []
    property var job: null
    property var lastLog: []
    // A frame that got no answer is sent once more before the write gives up.
    property int maxAttempts: 2
    readonly property bool busy: !!job
    property int progressDone: 0
    property int progressTotal: 0
    readonly property int jobLifeline: job ? job.lifelineId : -1

    signal finished(int lifelineId, string state)

    readonly property var _baseline: {
        var w = [{ kind: "v3", ver: 3, timeoutUs: 0 }]
        for (var ev = 1; ev <= 2; ++ev)
            for (var c = 0; c < 8; ++c) {
                var s = Compile.defaultSlot(ev, c)
                s.kind = "v6"; s.ver = 6
                w.push(s)
            }
        w.push({ kind: "v7", ver: 7, suppressSelfResponseUs: 400000, suppressSelfRequestUs: 400000, receiveResponseInIdle: false })
        w.push({ kind: "v4", ver: 4, addresses: [] })
        return w
    }

    function _sendable(dev, writes) {
        var hasV0 = typeof dev.setUsblTriggerControl === "function"
        return writes.filter(function (w) { return w.kind !== "v0" || hasV0 })
    }

    function _enqueue(item) {
        var q = queue.filter(function (j) { return j.lifelineId !== item.lifelineId })
        q.push(item)
        queue = q
        if (!job) Qt.callLater(_startNext)
    }

    /** Writes a lifeline's compiled state to its bound device. */
    function writeLifeline(lifelineId) {
        if (!store) return false
        var dev = store.deviceFor(lifelineId)
        var d = store.compiled.devices[lifelineId]
        if (!dev || !d) return false
        _enqueue({ lifelineId: lifelineId, kind: "network", fingerprint: d.fingerprint, writes: d.writes.slice() })
        return true
    }

    function writeLifelines(ids) {
        var n = 0
        for (var i = 0; i < ids.length; ++i) if (writeLifeline(ids[i])) ++n
        return n
    }

    function writeAllConnected() {
        var ids = []
        for (var i = 0; i < store.lifelines.length; ++i)
            if (store.deviceFor(store.lifelines[i].id)) ids.push(store.lifelines[i].id)
        return writeLifelines(ids)
    }

    function writeChanged() {
        var ids = []
        for (var i = 0; i < store.lifelines.length; ++i) {
            var id = store.lifelines[i].id
            if (store.deviceFor(id) && store.needsWrite(id)) ids.push(id)
        }
        return writeLifelines(ids)
    }

    /** Known, deliberately deaf state: silent, no address accepted, every slot at defaults. */
    function resetToBaseline(lifelineId) {
        if (!store || !store.deviceFor(lifelineId)) return false
        _enqueue({ lifelineId: lifelineId, kind: "baseline", fingerprint: "", writes: _baseline.slice() })
        return true
    }

    /** Stops every connected device answering. Not tracked as a write. */
    function silenceAll() {
        if (!store) return
        for (var i = 0; i < store.devices.length; ++i) {
            var d = store.devices[i]
            if (typeof d.usblControlReady === "function" && !d.usblControlReady()) continue
            d.acousticResponceTimeout(0)
        }
    }

    function cancel() {
        queue = []
        if (job) _finish("cancelled", 0)
    }

    property Timer _timeout: Timer {
        interval: 2600
        repeat: false
        onTriggered: writer._noAnswer()
    }

    property Connections _devConn: Connections {
        target: writer.job ? writer.job.dev : null
        ignoreUnknownSignals: true
        function onUsblControlFrameSent(ver, checksum) {
            var j = writer.job
            if (!j || j.awaitVer !== ver || j.awaitCk >= 0) return
            j.awaitCk = checksum
        }
        function onUsblControlAck(ver, checksum, code) {
            var j = writer.job
            if (!j || j.awaitVer < 0 || j.awaitCk < 0 || checksum !== j.awaitCk) return
            writer._timeout.stop()
            writer._log(code)
            if (code === 1) {
                j.acked += 1
                j.index += 1
                j.attempt = 0
                j.awaitVer = -1
                j.awaitCk = -1
                writer._progress()
                Qt.callLater(writer._sendCurrent)
            } else if (code === 0) {
                writer._noAnswer()
            } else {
                writer._finish("rejected", code)
            }
        }
    }

    function _log(code) {
        var j = job
        if (!j) return
        var w = j.writes[j.index]
        var l = lastLog.slice()
        l.push({ index: j.index, kind: w.kind, ver: w.ver, cmd: w.kind === "v6" ? w.cmd : -1,
                 event: w.kind === "v6" ? w.event : 0, code: code, attempt: j.attempt,
                 ms: Date.now() - j.sentAt })
        lastLog = l
    }

    function _noAnswer() {
        var j = job
        if (!j || j.awaitVer < 0) return
        _timeout.stop()
        if (j.attempt < maxAttempts) {
            j.awaitVer = -1
            j.awaitCk = -1
            Qt.callLater(_sendCurrent)
        } else {
            _finish("noAnswer", 0)
        }
    }

    function _startNext() {
        if (job || !queue.length) return
        var q = queue.slice()
        var item = q.shift()
        queue = q
        var dev = store ? store.deviceFor(item.lifelineId) : null
        if (!dev) { finished(item.lifelineId, "noDevice"); Qt.callLater(_startNext); return }
        var writes = _sendable(dev, item.writes)
        if (typeof dev.usblControlReady === "function" && !dev.usblControlReady()) {
            store.setWriteStatus(item.lifelineId, { state: "notReady", serial: dev.devSN, at: store.linkKeyOf(dev), atMs: Date.now(),
                                                    done: 0, total: writes.length, fingerprint: item.fingerprint,
                                                    failure: null })
            finished(item.lifelineId, "notReady")
            Qt.callLater(_startNext)
            return
        }
        lastLog = []
        job = { lifelineId: item.lifelineId, kind: item.kind, dev: dev, serial: dev.devSN, at: store.linkKeyOf(dev),
                fingerprint: item.fingerprint, writes: writes, index: 0, acked: 0, attempt: 0,
                awaitVer: -1, awaitCk: -1, sentAt: 0 }
        _progress()
        Qt.callLater(_sendCurrent)
    }

    function _progress() {
        var j = job
        if (!j) return
        progressDone = j.acked
        progressTotal = j.writes.length
        store.setWriteStatus(j.lifelineId, { state: "writing", serial: j.serial, at: j.at, atMs: Date.now(),
                                             done: j.acked, total: j.writes.length,
                                             fingerprint: j.fingerprint, failure: null })
    }

    function _sendCurrent() {
        var j = job
        if (!j) return
        if (j.index >= j.writes.length) { _finish("confirmed", 1); return }
        var w = j.writes[j.index]
        var d = j.dev
        j.attempt += 1
        j.awaitVer = w.ver
        j.awaitCk = -1
        j.sentAt = Date.now()
        _timeout.restart()
        switch (w.kind) {
        case "v0": d.setUsblTriggerControl(w.source, w.periodUs); break
        case "v3": d.acousticResponceTimeout(w.timeoutUs); break
        case "v4": d.acousticResponceFilterSlots(w.addresses); break
        case "v7": d.setUsblMonitorConfig(w.suppressSelfResponseUs, w.suppressSelfRequestUs, w.receiveResponseInIdle); break
        case "v6":
            d.setUsblCmdConfig(w.cmd, w.event, w.recvFn, w.recvBits, w.sendFn, w.sendHex,
                               w.eventAction, w.cmdIdAction, w.cmdIdRepl, w.addrAction, w.addrRepl,
                               w.sendFn === 1 ? w.sendBits : -1)
            break
        }
    }

    function _finish(state, code) {
        _timeout.stop()
        var j = job
        if (!j) return
        var failure = null
        if (state !== "confirmed" && state !== "cancelled" && j.index < j.writes.length) {
            var w = j.writes[j.index]
            failure = { frame: j.index + 1, total: j.writes.length, kind: w.kind, ver: w.ver,
                        cmd: w.kind === "v6" ? w.cmd : -1, event: w.kind === "v6" ? w.event : 0,
                        code: code || 0, attempts: j.attempt }
        }
        if ((state === "noAnswer" || state === "cancelled") && j.acked > 0)
            state = "partial"
        progressDone = j.acked
        progressTotal = j.writes.length
        var status = { state: state, serial: j.serial, at: j.at, atMs: Date.now(), done: j.acked,
                       total: j.writes.length, fingerprint: j.fingerprint, failure: failure }
        if (j.kind === "baseline") {
            status.fingerprint = ""
            if (state === "confirmed") status.state = "baseline"
        } else if (state === "confirmed") {
            store.recordWrite(j.dev, j.lifelineId, j.fingerprint, j.acked, j.writes.length)
        }
        store.setWriteStatus(j.lifelineId, status)
        job = null
        finished(j.lifelineId, status.state)
        Qt.callLater(_startNext)
    }
}

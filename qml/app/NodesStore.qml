import QtQuick 2.15
import QtQml
import QtCore
import "NodesModel.js" as Model
import "NodesCompile.js" as Compile
import "NodesCheck.js" as Check
import "NodesTiming.js" as Timing
import "NodesPayload.js" as Payload
import "UsblPlanLogic.js" as PlanLogic

QtObject {
    id: store

    // ── document ─────────────────────────────────────────────────────────
    property var st: Model.emptyState("")
    readonly property var compiled: Compile.compile(st)
    readonly property var lifelines: Model.orderedLifelines(st)
    readonly property var exchanges: st.exchanges
    readonly property var formats: st.formats
    readonly property var cycleSteps: compiled.steps
    readonly property var dataLegs: Compile.dataLegs(st, compiled)

    readonly property var _sols: (typeof dataset !== "undefined" && dataset && dataset.usblSolutions)
                                 ? dataset.usblSolutions : ({})
    readonly property var problems: Check.check(st, compiled)
    readonly property var liveProblems: Check.liveCheck(st, compiled, _sols)
    readonly property var allProblems: problems.concat(liveProblems)
    readonly property var problemCounts: Check.counts(allProblems)

    readonly property var mapNodes: {
        var out = []
        for (var i = 0; i < st.lifelines.length; ++i) {
            var l = st.lifelines[i]
            if (l.kind === "head" || typeof l.address !== "number") continue
            out.push({ "id": l.id, "addr": l.address, "active": l.active !== false })
        }
        return out
    }
    readonly property bool hasSteps: compiled.steps.some(function (s) { return s.enabled })

    property string filePath: ""
    property string savedHash: ""
    readonly property string contentHash: Model.contentHash(st)
    readonly property bool dirty: contentHash !== savedHash
    readonly property string displayName: {
        if (filePath.length) {
            var p = filePath.replace(/\\/g, "/")
            return p.substring(p.lastIndexOf("/") + 1)
        }
        return st.name.length ? st.name : qsTr("Untitled network")
    }
    property bool readOnly: false
    property string lastError: ""
    property var lastNotes: []
    property var _fileStamp: null
    property bool externalChange: false

    signal networkReplaced()
    signal errorRaised(string text)

    // ── selection ────────────────────────────────────────────────────────
    property string selKind: "network"
    property int selId: -1
    function select(kind, id) { selKind = kind; selId = (typeof id === "number") ? id : -1 }
    readonly property var selLifeline: selKind === "lifeline" ? Model.lifeline(st, selId) : null
    readonly property var selStep: selKind === "step" ? Model.step(st, selId) : null
    readonly property var selExchange: selKind === "step"
                                       ? (selStep ? Model.exchange(st, selStep.exchange) : null)
                                       : (selKind === "exchange" ? Model.exchange(st, selId) : null)

    // ── undo ─────────────────────────────────────────────────────────────
    property var _undo: []
    property var _redo: []
    readonly property bool canUndo: _undo.length > 0
    readonly property bool canRedo: _redo.length > 0

    function _commit(next) {
        if (!next || next === st) return false
        if (readOnly) { _raise(qsTr("This network is open read-only.")); return false }
        var u = _undo.slice()
        u.push(st)
        if (u.length > 200) u.shift()
        _undo = u
        _redo = []
        st = next
        _autosave.restart()
        return true
    }
    function undo() {
        if (!_undo.length) return
        var u = _undo.slice(), r = _redo.slice()
        r.push(st)
        st = u.pop()
        _undo = u; _redo = r
        _fixSelection()
        _autosave.restart()
    }
    function redo() {
        if (!_redo.length) return
        var u = _undo.slice(), r = _redo.slice()
        u.push(st)
        st = r.pop()
        _undo = u; _redo = r
        _fixSelection()
        _autosave.restart()
    }
    function _replace(next, path, hash) {
        st = next
        filePath = path || ""
        savedHash = (typeof hash === "string") ? hash : Model.contentHash(next)
        _undo = []; _redo = []
        select("network", -1)
        externalChange = false
        _autosave.restart()
        networkReplaced()
    }
    function _fixSelection() {
        if (selKind === "lifeline" && !Model.lifeline(st, selId)) select("network", -1)
        if (selKind === "step" && !Model.step(st, selId)) select("network", -1)
        if (selKind === "exchange" && !Model.exchange(st, selId)) select("network", -1)
    }
    function _raise(text) { lastError = text; errorRaised(text) }

    // ── persistence of the working copy ──────────────────────────────────
    property Settings _persist: Settings {
        id: persist
        category: "main/nodes"
        property string docJson: ""
        property string filePath: ""
        property string savedHash: ""
        property string recentJson: "[]"
        property string openFolder: ""
        property string saveFolder: ""
        property string historyJson: "{}"
        property bool migratedFromPlan: false
        property bool readOnly: false
    }
    property Settings _oldPlan: Settings {
        id: oldPlan
        category: "main/usbl"
        property string planJson: ""
    }
    property Settings _oldSchedule: Settings {
        id: oldSchedule
        category: "main/usblSchedule"
        property int dwellMs: 0
        property bool fireOnPps: false
    }
    property Timer _autosave: Timer {
        interval: 400
        repeat: false
        onTriggered: {
            persist.docJson = Model.serialize(store.st)
            persist.filePath = store.filePath
            persist.savedHash = store.savedHash
            persist.readOnly = store.readOnly
        }
    }

    readonly property var recentFiles: {
        try { var a = JSON.parse(persist.recentJson); return Array.isArray(a) ? a : [] }
        catch (e) { return [] }
    }
    function _pushRecent(path) {
        if (!path) return
        var a = recentFiles.filter(function (p) { return p !== path })
        a.unshift(path)
        persist.recentJson = JSON.stringify(a.slice(0, 8))
    }
    function removeRecent(path) {
        persist.recentJson = JSON.stringify(recentFiles.filter(function (p) { return p !== path }))
    }
    readonly property string openFolder: persist.openFolder
    readonly property string saveFolder: persist.saveFolder

    function load() {
        if (persist.docJson.length) {
            var r = Model.deserialize(persist.docJson)
            if (r.state) {
                st = r.state
                filePath = persist.filePath
                savedHash = persist.savedHash
                readOnly = persist.readOnly
                _stampFile()
                return
            }
        }
        if (!persist.migratedFromPlan && oldPlan.planJson.length) {
            migrateFromPlan()
            persist.migratedFromPlan = true
            return
        }
        _replace(Model.emptyState(""), "", undefined)
    }

    function migrateFromPlan() {
        var planSt = PlanLogic.deserialize(oldPlan.planJson)
        var r = Model.fromPlan(planSt, { dwellMs: oldSchedule.dwellMs, fireOnPps: oldSchedule.fireOnPps })
        _replace(r.state, "", "")
        lastNotes = r.notes
    }

    // ── files ────────────────────────────────────────────────────────────
    function _local(pathOrUrl) {
        return (typeof appUtils !== "undefined" && appUtils) ? appUtils.toLocalPath(String(pathOrUrl)) : String(pathOrUrl)
    }
    function _stampFile() {
        if (!filePath.length || typeof appUtils === "undefined") { _fileStamp = null; return }
        var i = appUtils.fileInfo(filePath)
        _fileStamp = i.exists ? { size: i.size, modifiedMs: i.modifiedMs } : null
    }
    function checkExternalChange() {
        if (!filePath.length || !_fileStamp || typeof appUtils === "undefined") return false
        var i = appUtils.fileInfo(filePath)
        externalChange = !i.exists || i.size !== _fileStamp.size || i.modifiedMs !== _fileStamp.modifiedMs
        return externalChange
    }
    function keepMine() { _stampFile(); externalChange = false }

    function newNetwork(kind, params) {
        var next = (kind && kind !== "empty") ? Model.template(kind, params || {}) : Model.emptyState(qsTr("New network"))
        readOnly = false
        _replace(next, "", undefined)
        lastNotes = []
    }

    function openFile(pathOrUrl) {
        var path = _local(pathOrUrl)
        var text = appUtils.readTextFile(path)
        if (!text.length) {
            var err = appUtils.lastFileError()
            _raise(err.length ? qsTr("Cannot read %1: %2").arg(path).arg(err) : qsTr("%1 is empty").arg(path))
            return false
        }
        var r = Model.deserialize(text)
        if (r.error) {
            _raise(r.error.code === "notJson"
                   ? qsTr("%1 is not a valid JSON file: %2").arg(path).arg(r.error.detail)
                   : qsTr("%1 is not a Nodes-Configurator network").arg(path))
            return false
        }
        readOnly = r.readOnly
        _replace(r.state, path, undefined)
        lastNotes = r.problems.map(function (p) { return p.code })
        persist.openFolder = appUtils.folderUrlOf(path)
        _pushRecent(path)
        _stampFile()
        if (r.readOnly) _raise(qsTr("This file was written by a newer version and is open read-only."))
        return true
    }

    function _withResolved() {
        var next = Model.recordResolvedCmds(st, compiled.resolved)
        if (next !== st) st = next
    }

    function _writeTo(path, withBindings) {
        _withResolved()
        var text = Model.serialize(st, { withBindings: withBindings })
        if (!appUtils.writeTextFile(path, text)) {
            _raise(qsTr("Cannot write %1: %2").arg(path).arg(appUtils.lastFileError()))
            return false
        }
        return true
    }

    function saveFile() {
        if (!filePath.length) return false
        if (readOnly) { _raise(qsTr("This network is open read-only; use Save as.")); return false }
        if (!_writeTo(filePath, true)) return false
        savedHash = Model.contentHash(st)
        _stampFile()
        externalChange = false
        _autosave.restart()
        return true
    }

    function _withSuffix(path) {
        return /\.knodes$/i.test(path) ? path : path + ".knodes"
    }

    function saveFileAs(pathOrUrl) {
        var path = _withSuffix(_local(pathOrUrl))
        if (!_writeTo(path, true)) return false
        filePath = path
        readOnly = false
        savedHash = Model.contentHash(st)
        persist.saveFolder = appUtils.folderUrlOf(path)
        _pushRecent(path)
        _stampFile()
        externalChange = false
        _autosave.restart()
        return true
    }

    function saveCopyWithoutBindings(pathOrUrl) {
        var path = _withSuffix(_local(pathOrUrl))
        if (!_writeTo(path, false)) return false
        persist.saveFolder = appUtils.folderUrlOf(path)
        return true
    }

    function revert() {
        if (!filePath.length) return false
        return openFile(filePath)
    }

    function exportFrames(pathOrUrl) {
        var path = _local(pathOrUrl)
        if (!/\.json$/i.test(path)) path += ".json"
        var out = { network: st.name, devices: {}, steps: [] }
        for (var i = 0; i < st.lifelines.length; ++i) {
            var l = st.lifelines[i], d = compiled.devices[l.id]
            if (!d) continue
            out.devices[l.label + " (" + l.id + ")"] = {
                binding: l.binding, fingerprint: d.fingerprint, writes: d.writes
            }
        }
        for (var k = 0; k < compiled.steps.length; ++k) {
            var s = compiled.steps[k]
            out.steps.push({ step: s.stepId, initiator: s.initiatorId, address: s.addr, cmd: s.cmd,
                             replyDistanceMm: s.replyMm, triggerUs: s.triggerUs,
                             payloadHex: s.payloadHex, payloadBits: s.payloadBits,
                             durationMs: s.durationMs, requiredMs: s.requiredMs, enabled: s.enabled })
        }
        if (!appUtils.writeTextFile(path, JSON.stringify(out, null, 2))) {
            _raise(qsTr("Cannot write %1: %2").arg(path).arg(appUtils.lastFileError()))
            return false
        }
        return true
    }

    function exportPerNode(folderPathOrUrl) {
        var folder = _local(folderPathOrUrl).replace(/[\\\/]+$/, "")
        var ok = true
        for (var i = 0; i < st.lifelines.length; ++i) {
            var l = st.lifelines[i]
            if (l.kind === "external") continue
            var single = Model.clone(st)
            single.name = st.name + " — " + l.label
            var safe = l.label.replace(/[^A-Za-z0-9_\-]+/g, "_")
            if (!appUtils.writeTextFile(folder + "/" + safe + ".knodes", Model.serialize(single)))
                ok = false
        }
        if (!ok) _raise(qsTr("Some per-node files could not be written: %1").arg(appUtils.lastFileError()))
        return ok
    }

    // ── edits ────────────────────────────────────────────────────────────
    function setNetworkField(f, v) { _commit(Model.setNetworkField(st, f, v)) }
    function addLifeline(kind, address) {
        var r = Model.addLifeline(st, kind, address)
        if (_commit(r.state)) select("lifeline", r.id)
        return r.id
    }
    function removeLifeline(id) { _commit(Model.removeLifeline(st, id)); _fixSelection() }
    function setLifelineField(id, f, v) { _commit(Model.setLifelineField(st, id, f, v)) }
    function moveLifeline(id, idx) { _commit(Model.moveLifeline(st, id, idx)) }
    function bindingFromDevice(dev) {
        if (!dev) return null
        return { serial: dev.devSN, uid: String(dev.devUID || ""), devName: String(dev.devName || ""),
                 fw: String(dev.fwVersion || "") }
    }
    function bindLifeline(id, dev) { _commit(Model.bindLifeline(st, id, bindingFromDevice(dev))) }
    function unbindLifeline(id) { _commit(Model.bindLifeline(st, id, null)) }

    function addExchange(fromId, toId, kind, atIndex) {
        var r = Model.addExchange(st, fromId, toId, kind, atIndex)
        if (_commit(r.state)) select("step", r.stepId)
        return r.stepId
    }
    function removeExchange(id) { _commit(Model.removeExchange(st, id)); _fixSelection() }

    /** The head to interrogate from: `lifelineId` when it is a head, else the first connected head, else the first head; -1 when there is none. */
    function headFor(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        if (l && l.kind === "head") return l.id
        var first = -1
        for (var i = 0; i < st.lifelines.length; ++i) {
            var h = st.lifelines[i]
            if (h.kind !== "head") continue
            if (deviceFor(h.id)) return h.id
            if (first < 0) first = h.id
        }
        return first
    }
    /** Sets the interval between steps, as the old Dwell did: switches the cycle to period mode. */
    function setPeriodMs(ms) {
        _commit(Model.setNetworkField(Model.setNetworkField(st, "stepMode", "period"), "periodMs", ms))
    }
    readonly property bool periodMode: st.cycle.stepMode === "period"
    // The shortest period that leaves no step stretched: the longest step the cycle runs.
    readonly property int periodFloorMs: {
        var m = 0
        for (var i = 0; i < compiled.steps.length; ++i) {
            var s = compiled.steps[i]
            if (s.on && s.targetActive) m = Math.max(m, s.requiredMs)
        }
        return m > 0 ? m : Timing.requiredMs({ kind: "ranging", maxRangeM: st.maxRangeM })
    }
    readonly property var answeringTargets: st.lifelines.filter(function (l) {
        return l.kind !== "head" && typeof l.address === "number" && l.respond !== "off"
    })
    function setExchangeField(id, f, v) { _commit(Model.setExchangeField(st, id, f, v)) }
    function setExchangeData(id, which, data) { _commit(Model.setExchangeData(st, id, which, data)) }
    function setFollowUp(id, on) { _commit(Model.setFollowUp(st, id, on)) }
    function setFollowUpCmd(id, c) { _commit(Model.setFollowUpCmd(st, id, c)) }
    function setRewrite(id, rw) { _commit(Model.setRewrite(st, id, rw)) }
    function setListener(id, lid, on, hear) { _commit(Model.setListener(st, id, lid, on, hear)) }

    function addStep(exId, idx) {
        var r = Model.addStep(st, exId, idx)
        if (_commit(r.state)) select("step", r.stepId)
    }
    function removeStep(id) { _commit(Model.removeStep(st, id)); _fixSelection() }
    function moveStep(id, idx) { _commit(Model.moveStep(st, id, idx)) }
    function setStepField(id, f, v) { _commit(Model.setStepField(st, id, f, v)) }
    function duplicateStep(id) {
        var r = Model.duplicateStep(st, id)
        if (_commit(r.state)) select("step", r.stepId)
    }
    function addFormat(name) { var r = Model.addFormat(st, name); _commit(r.state); return r.id }
    function setFormat(id, f) { _commit(Model.setFormat(st, id, f)) }
    function removeFormat(id) { _commit(Model.removeFormat(st, id)) }

    function fitMaxRange(exchangeId, rangeM) {
        if (exchangeId >= 0) {
            var ex = Model.exchange(st, exchangeId)
            if (ex && ex.maxRange !== "network") { setExchangeField(exchangeId, "maxRange", rangeM); return }
        }
        setNetworkField("maxRangeM", rangeM)
    }
    function measuredRangeOf(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        if (!l || typeof l.address !== "number") return NaN
        var e = _sols[String(l.address)]
        return (e && typeof e.distance === "number") ? e.distance : NaN
    }
    function useMeasuredRanges() {
        var next = st
        for (var i = 0; i < st.lifelines.length; ++i) {
            var m = measuredRangeOf(st.lifelines[i].id)
            if (!isNaN(m) && m > 0) next = Model.setLifelineField(next, st.lifelines[i].id, "plannedRangeM", Math.round(m))
        }
        _commit(next)
    }
    function fitMaxRangeToMeasured() {
        var best = 0
        for (var i = 0; i < st.lifelines.length; ++i) {
            var m = measuredRangeOf(st.lifelines[i].id)
            if (!isNaN(m) && m > best) best = m
        }
        if (best > 0) setNetworkField("maxRangeM", Timing.fitRangeM(best))
    }

    function applyFix(p) {
        if (!p || !p.fix) return
        var f = p.fix
        switch (f.action) {
        case "respondAlways": setLifelineField(f.lifeline >= 0 ? f.lifeline : p.lifeline, "respond", "always"); break
        case "respondOff": setLifelineField(f.lifeline, "respond", "off"); break
        case "filterAuto": setLifelineField(p.lifeline, "filter", "auto"); break
        case "addToFilter": {
            var l = Model.lifeline(st, f.lifeline)
            if (l && Array.isArray(l.filter)) setLifelineField(l.id, "filter", l.filter.concat([f.address]))
            break
        }
        case "setAddress": setLifelineField(f.lifeline, "address", Model.freeAddress(st)); break
        case "freeAddress": setLifelineField(f.lifeline, "address", Model.freeAddress(st)); break
        case "makePing": setExchangeField(p.exchange, "kind", "ping"); break
        case "removeFollowUp": setFollowUp(p.exchange, false); break
        case "clearRewrite": setRewrite(p.exchange, null); break
        case "fitRange": fitMaxRange(f.exchange, f.rangeM); break
        case "cmdAuto": setExchangeField(f.exchange, "cmd", "auto"); break
        case "stepAuto": setStepField(f.step, "durationMs", "auto"); break
        case "executorHost": setLifelineField(f.lifeline, "executorMode", "host"); break
        case "bind": select("lifeline", p.lifeline); break
        }
    }

    // ── devices and bindings ─────────────────────────────────────────────
    // Reassigned only when the list itself changes: a device being flashed re-reports its
    // version several times a second, on the GUI thread that also carries the upgrade.
    property var devices: []
    readonly property var _allDevs: {
        var ds = (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper) ? deviceManagerWrapper.devs : []
        var out = []
        for (var i = 0; i < ds.length; ++i) out.push(ds[i])
        return out
    }
    function _refreshDevices() {
        var out = []
        for (var i = 0; i < _allDevs.length; ++i) {
            var d = _allDevs[i]
            if (d && d.isBoardInited && (d.isUSBL || d.isUSBLBeacon)) out.push(d)
        }
        var same = out.length === devices.length
        for (var k = 0; same && k < out.length; ++k) same = out[k] === devices[k]
        if (!same) devices = out
    }
    on_AllDevsChanged: _refreshDevices()
    Component.onCompleted: _refreshDevices()
    property Instantiator _versionWatch: Instantiator {
        model: store._allDevs
        delegate: Connections {
            required property var modelData
            target: modelData
            ignoreUnknownSignals: true
            function onDeviceVersionChanged() { store._refreshDevices() }
        }
    }
    function _matches(binding, dev) {
        if (!binding || !dev) return false
        if (binding.serial !== dev.devSN) return false
        return !binding.uid || binding.uid === String(dev.devUID || "")
    }
    function deviceFor(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        if (!l || !l.binding) return null
        for (var i = 0; i < devices.length; ++i) if (_matches(l.binding, devices[i])) return devices[i]
        return null
    }
    function lifelineForDevice(dev) {
        for (var i = 0; i < st.lifelines.length; ++i) if (_matches(st.lifelines[i].binding, dev)) return st.lifelines[i]
        return null
    }
    readonly property var unboundDevices: devices.filter(function (d) { return !store.lifelineForDevice(d) })

    function linkLabelOf(dev) {
        if (!dev || typeof deviceTopology === "undefined" || !deviceTopology) return ""
        var g = deviceTopology.groupForDevice(dev)
        if (!g) return ""
        var name = g.customName && g.customName.length ? g.customName : (g.portName || "")
        var port = (dev.busAddress >= 1 && dev.busAddress <= 8) ? " · " + qsTr("port %1").arg(dev.busAddress) : ""
        return name + port
    }

    // ── write state and history ──────────────────────────────────────────
    property var writeStatus: ({})
    function setWriteStatus(id, status) {
        var n = {}
        for (var k in writeStatus) n[k] = writeStatus[k]
        n[id] = status
        writeStatus = n
    }
    readonly property var history: {
        try { var o = JSON.parse(persist.historyJson); return (o && typeof o === "object") ? o : {} }
        catch (e) { return {} }
    }
    function _historyKey(dev) { return String(dev.devSN) + "/" + String(dev.devUID || "") }
    function _historyKeyOf(binding) { return String(binding.serial) + "/" + String(binding.uid || "") }
    function recordWrite(dev, lifelineId, fingerprint, ok, total) {
        var h = {}
        for (var k in history) h[k] = history[k]
        h[_historyKey(dev)] = { networkName: st.name, filePath: filePath, lifelineId: lifelineId,
                                fingerprint: fingerprint, ok: ok, total: total,
                                fw: String(dev.fwVersion || ""), atMs: Date.now() }
        persist.historyJson = JSON.stringify(h)
    }
    function historyFor(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        if (!l || !l.binding) return null
        return history[_historyKeyOf(l.binding)] || null
    }

    /**
     * What is known about a lifeline's device: unbound | external | writing | rejected | noAnswer |
     * partial | notReady | cancelled | baseline | confirmed | changed | earlier | earlierChanged |
     * unknown | notWritten.
     */
    function writeStateOf(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        if (!l) return "unbound"
        if (l.kind === "external") return "external"
        if (!l.binding) return "unbound"
        var fp = compiled.devices[l.id] ? compiled.devices[l.id].fingerprint : ""
        var dev = deviceFor(l.id)
        var s = writeStatus[l.id]
        if (dev && s && s.serial === dev.devSN) {
            if (s.state === "writing") return "writing"
            if (s.state === "baseline") return "baseline"
            if (s.state === "rejected" || s.state === "noAnswer" || s.state === "partial" || s.state === "notReady"
                || s.state === "cancelled") return s.state
            if (s.state === "confirmed") return s.fingerprint === fp ? "confirmed" : "changed"
        }
        var h = historyFor(l.id)
        if (h) return h.fingerprint === fp ? "earlier" : "earlierChanged"
        return dev ? "unknown" : "notWritten"
    }
    /** Whether "Write changed" rewrites the lifeline: every unwritten state, and "earlier", which cannot be verified. */
    function needsWrite(lifelineId) {
        var w = writeStateOf(lifelineId)
        return w === "earlier" || _unwrittenStates.indexOf(w) >= 0
    }

    property Connections _devWatch: Connections {
        target: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
        ignoreUnknownSignals: true
        function onDevChanged() { store._refreshDevices(); store._dropStaleStatus() }
    }
    function _dropStaleStatus() {
        var n = {}, changed = false
        for (var k in writeStatus) {
            var id = Number(k), dev = deviceFor(id), s = writeStatus[k]
            if (dev && s && s.serial === dev.devSN) n[k] = s
            else changed = true
        }
        if (changed) writeStatus = n
    }

    // ── quick interrogate ────────────────────────────────────────────────
    /** Returns the head lifeline id that interrogates `addresses` from `dev`. */
    function quickInterrogate(dev, addresses, maxRangeM) {
        var existing = lifelineForDevice(dev)
        if (existing && Model.isInitiator(st, existing.id)) return existing.id
        var b = bindingFromDevice(dev)
        if (!st.lifelines.length) {
            _replace(Model.quickNetwork(b, addresses, maxRangeM), "", undefined)
            return st.lifelines[0].id
        }
        var r = Model.addQuickHead(st, b, addresses)
        if (typeof maxRangeM === "number" && maxRangeM > 0 && !st.exchanges.length)
            r.state = Model.setNetworkField(r.state, "maxRangeM", maxRangeM)
        _commit(r.state)
        return r.headId
    }

    // ── texts ────────────────────────────────────────────────────────────
    function lifelineLabel(id) {
        var l = Model.lifeline(st, id)
        return l ? l.label : "?"
    }
    function exchangeLabel(ex) {
        if (!ex) return ""
        if (ex.label.length) return ex.label
        var a = Model.targetAddress(st, ex)
        return lifelineLabel(ex.from) + " → " + (ex.to && typeof ex.to.lifeline === "number" ? lifelineLabel(ex.to.lifeline) : qsTr("address %1").arg(a))
    }
    function dataSummary(d) { return Payload.summary(d, st.formats) }

    function frameText(f) {
        if (!f) return ""
        switch (f.kind) {
        case "v0": return qsTr("V0 trigger")
        case "v3": return qsTr("V3 answering")
        case "v4": return qsTr("V4 address filter")
        case "v7": return qsTr("V7 monitor")
        case "v6": return qsTr("V6 cmd %1 on %2").arg(f.cmd).arg(f.event === 2 ? qsTr("reply") : qsTr("request"))
        }
        return f.kind
    }
    function respCodeText(code) {
        switch (code) {
        case 0: return qsTr("no answer")
        case 1: return qsTr("OK")
        case 2: return qsTr("rejected: checksum")
        case 3: return qsTr("rejected: payload")
        case 4: return qsTr("rejected: unknown ID")
        case 5: return qsTr("rejected: unknown version")
        case 6: return qsTr("rejected: wrong type")
        case 7: return qsTr("rejected: key")
        case 8: return qsTr("rejected: runtime error")
        }
        return qsTr("rejected, code %1").arg(code)
    }
    /** Which frame stopped the last write to this lifeline's device, or "". */
    function writeFailureText(lifelineId) {
        var s = writeStatus[lifelineId]
        if (!s || !s.failure) return ""
        var f = s.failure
        var why = respCodeText(f.code)
        if (f.code === 0 && f.attempts > 1) why = qsTr("no answer after %1 tries").arg(f.attempts)
        return qsTr("Stopped at frame %1 of %2 (%3): %4. %5 of %2 confirmed.")
               .arg(f.frame).arg(f.total).arg(frameText(f)).arg(why).arg(s.done)
    }

    function dataLegText(leg) {
        switch (leg) {
        case "request": return qsTr("request data")
        case "response": return qsTr("reply data")
        case "followUp": return qsTr("follow-up data")
        case "followUpReply": return qsTr("second reply data")
        }
        return qsTr("data")
    }
    /** What a device decoded, compared with what was sent: "✓ A5 · 8 bit, as expected". */
    function rxEntryText(e) {
        if (!e) return ""
        var got = qsTr("%1 · %2 bit").arg(e.text).arg(e.bits)
        if (e.verdict === "match") return qsTr("✓ %1, as expected").arg(got)
        if (e.verdict === "differs") {
            if (e.differing < 0) return qsTr("≠ %1, expected %2 · %3 bit").arg(got).arg(e.expectedText).arg(e.expectedBits)
            return qsTr("≠ %1, expected %2 · %n bit(s) differ", "", e.differing).arg(got).arg(e.expectedText)
        }
        return got
    }
    readonly property var _unwrittenStates: ["changed", "earlierChanged", "notWritten", "unknown", "rejected", "noAnswer",
                                             "partial", "notReady", "cancelled"]
    /** Whether a lifeline's device does not hold the network's configuration yet. */
    function writePending(lifelineId) {
        var l = Model.lifeline(st, lifelineId)
        return !!l && l.kind !== "external" && _unwrittenStates.indexOf(writeStateOf(lifelineId)) >= 0
    }
    /** Lifelines a data leg depends on whose devices are not written yet: the sender's and the receiver's slots. */
    function unwrittenEnds(leg) {
        var ex = leg ? Model.exchange(st, leg.exchangeId) : null
        if (!ex) return []
        var ids = [ex.from]
        var t = Model.targetLifeline(st, ex)
        if (t && ids.indexOf(t.id) < 0) ids.push(t.id)
        if (ids.indexOf(leg.lifelineId) < 0) ids.push(leg.lifelineId)
        return ids.filter(function (id) { return store.writePending(id) })
    }
    /** Short live mark of a data leg on the canvas. */
    function rxMarkText(ds, leg) {
        if (!ds) return ""
        switch (ds.state) {
        case "match": return "✓ " + ds.entry.text
        case "differs": return "≠ " + ds.entry.text
        case "waiting": return qsTr("… data")
        case "missing": return unwrittenEnds(leg).length ? qsTr("✗ not written") : qsTr("✗ no data")
        }
        return ""
    }
    /** Full account of a data leg's live state, for tooltips. */
    function rxDetailText(ds, leg) {
        if (!ds || !leg) return ""
        var head = qsTr("%1 at %2").arg(dataLegText(leg.leg)).arg(lifelineLabel(leg.lifelineId))
        switch (ds.state) {
        case "match":
        case "differs": return head + ": " + rxEntryText(ds.entry)
        case "waiting": return qsTr("%1: waiting for the decoded data").arg(head)
        case "missing": {
            var ends = unwrittenEnds(leg).map(function (id) { return store.lifelineLabel(id) })
            if (ends.length)
                return qsTr("%1: nothing decoded — %2 not written with this configuration yet; use Write changed")
                       .arg(head).arg(ends.join(", "))
            return qsTr("%1: the device reported no decoded data for this send").arg(head)
        }
        }
        return head
    }
    /** Lifelines on bound devices that do not hold the network's configuration. */
    readonly property var unwrittenLifelines: st.lifelines.filter(function (l) {
        return !!l.binding && store.writePending(l.id)
    })
    function fmtNum(v, digits) { return (typeof v === "number" && isFinite(v)) ? v.toFixed(digits) : "—" }
    function fmtAmpDb(v) { return (typeof v === "number" && isFinite(v) && v > 0) ? (20 * Math.log10(v)).toFixed(1) : "—" }
    /** Range, direction and RSRQ of a step's last reply. */
    function fixText(L) {
        if (!L || typeof L.distance !== "number") return ""
        return qsTr("%1 m · az %2° · el %3° · RSRQ %4 dB")
               .arg(fmtNum(L.distance, 1)).arg(fmtNum(L.azimuth, 1)).arg(fmtNum(L.elevation, 1)).arg(fmtNum(L.snr, 0))
    }
    /** Sync quality of a step's last reply, the scene panel's signal details. */
    function syncText(L) {
        var q = L && L.sync ? L.sync : null
        if (!q || !(isFinite(q.val) || isFinite(q.snr) || isFinite(q.firstVal))) return ""
        return qsTr("peak %1 dB · SNR %2 dB · 1st %3 dB · SNR %4 dB · RMS %5 dB · 1st→peak %6 ms")
               .arg(fmtAmpDb(q.val)).arg(fmtNum(q.snr, 0)).arg(fmtAmpDb(q.firstVal)).arg(fmtNum(q.firstSnr, 0))
               .arg(fmtAmpDb(q.rms)).arg(fmtNum(q.leadMs, 2))
    }
    /** The beacon's position reported with a step's last reply, or "". */
    function beaconText(L) {
        var b = L ? L.beacon : null
        if (!b) return ""
        return qsTr("%1, %2 · depth %3 m").arg(fmtNum(b.lat, 6)).arg(fmtNum(b.lon, 6)).arg(fmtNum(b.depth, 1))
    }

    /** A ModemSolution as received, attributed or not. */
    function rxFrameText(e) {
        if (!e) return ""
        var where = e.exchangeId >= 0
                ? qsTr("%1 of %2").arg(dataLegText(e.leg)).arg(exchangeLabel(Model.exchange(st, e.exchangeId)))
                : qsTr("not from any exchange of this network")
        return qsTr("%1 · %2 bit · address %3 · cmd %4 · on %5 — %6")
               .arg(e.text).arg(e.bits).arg(e.addr).arg(e.cmd)
               .arg(e.event === 2 ? qsTr("reply") : qsTr("request")).arg(where)
    }

    function writeStateText(code) {
        switch (code) {
        case "unbound": return qsTr("not bound to a device")
        case "external": return qsTr("external — not configured by this app")
        case "writing": return qsTr("writing…")
        case "rejected": return qsTr("rejected by the device")
        case "noAnswer": return qsTr("no answer from the device")
        case "partial": return qsTr("partially written")
        case "notReady": return qsTr("device not ready to accept settings")
        case "cancelled": return qsTr("write cancelled — nothing confirmed")
        case "confirmed": return qsTr("confirmed, matches the network")
        case "changed": return qsTr("changed since written")
        case "earlier": return qsTr("written earlier — not verifiable, holds only if it stayed powered")
        case "earlierChanged": return qsTr("written earlier with other settings")
        case "unknown": return qsTr("connected, state unknown — not written since it connected")
        case "notWritten": return qsTr("never written from this computer")
        case "baseline": return qsTr("reset to baseline: silent, hears no address")
        }
        return code
    }

    function problemText(p) {
        var d = p.detail || {}
        var ex = p.exchange >= 0 ? exchangeLabel(Model.exchange(st, p.exchange)) : ""
        var ll = p.lifeline >= 0 ? lifelineLabel(p.lifeline) : ""
        switch (p.code) {
        case "emptyNetwork": return qsTr("The network is empty. Add a head and beacons, or start from a template.")
        case "unbound": return qsTr("%1 is not bound to a device.").arg(ll)
        case "windowExpires": return qsTr("%1 answers only for %2 s after each write.").arg(ll).arg(d.windowS)
        case "deafLifeline": return qsTr("%1 hears no address: its request filter is empty.").arg(ll)
        case "targetHasNoAddress": return qsTr("%1 has no acoustic address.").arg(ll)
        case "addressOutOfRange": return qsTr("%1 targets address %2; only 0–7 are received.").arg(ex).arg(d.address)
        case "targetUnanswered": return qsTr("%1 expects a reply, but %2 does not respond.").arg(ex).arg(ll)
        case "targetUnheard": return qsTr("%1: address %2 is not in %3's filter.").arg(ex).arg(d.address).arg(ll)
        case "noResponderInNetwork": return qsTr("%1: no lifeline answers address %2.").arg(ex).arg(d.address)
        case "externalInitiator": return qsTr("%1 is started by an external party, not by this app.").arg(ex)
        case "requestTooLongForReply": return qsTr("%1: %2 bits of request data get no reply (at most %3 bits).").arg(ex).arg(d.bits).arg(d.max)
        case "payloadTooLong": return qsTr("%1: %2 bits exceed the 183-bit payload limit.").arg(ex).arg(d.bits)
        case "llGeoNeedsNav": return qsTr("%1 sends the head's position: it needs a fresh GNSS fix on the head.").arg(ex)
        case "llGeoUnvalidated": return qsTr("%1: position + azimuth has a weak check; decoded values are unvalidated.").arg(ex)
        case "followUpEmpty": return qsTr("%1 has a follow-up with no data.").arg(ex)
        case "formatMissing": return qsTr("%1 uses a payload format that no longer exists.").arg(ex)
        case "replyAsRequest": return qsTr("%1 replies with a request; the head does not accept it.").arg(ex)
        case "plannedBeyondMax": return qsTr("%1 is planned at %2 m, beyond Max range %3 m: replies miss the window or fold.").arg(ll).arg(Math.round(d.plannedM)).arg(d.maxM)
        case "maxRangeShort": return qsTr("%1: Max range is only %2 m.").arg(ex).arg(d.maxM)
        case "listenerWouldAnswer": return qsTr("%1 listens to %2 but would also answer it.").arg(ll).arg(ex)
        case "addressCollision": return qsTr("More than one lifeline answers address %1: two replies collide.").arg(d.address)
        case "cmdConflict": return qsTr("%1: its pinned command id conflicts with another exchange.").arg(ex)
        case "tooManyCmds": return qsTr("%1: no free command id is left on one of its devices.").arg(ex)
        case "stepTooShort": return qsTr("A step lasts %1 ms but needs %2 ms: replies will miss or fold.").arg(d.durationMs).arg(d.requiredMs)
        case "stepMuchLonger": return qsTr("A step lasts %1 ms; %2 ms would do.").arg(d.durationMs).arg(d.requiredMs)
        case "ppsStepVsPeriod": return qsTr("A PPS step takes %1 pulse periods.").arg(d.periods)
        case "noSteps": return qsTr("No step is enabled; nothing will be interrogated.")
        case "periodicNeedsOneStep": return qsTr("%1: device-periodic transmission needs exactly one step (has %2).").arg(ll).arg(d.steps)
        case "periodicUnavailable": return qsTr("%1: device-periodic transmission is not available yet; the app keeps time instead.").arg(ll)
        case "multiHeadLive": return qsTr("Several heads are bound: live replies are told apart by address only.")
        case "measuredNearMax": return qsTr("%1: measured %2 m, close to Max range %3 m.").arg(ex).arg(Math.round(d.distance)).arg(d.maxM)
        case "negativeRange": return qsTr("%1: negative range %2 m — the head's firmware predates the range fix.").arg(ex).arg(Math.round(d.distance))
        }
        return p.code
    }
    function fixLabel(p) {
        if (!p || !p.fix) return ""
        switch (p.fix.action) {
        case "respondAlways": return qsTr("Respond")
        case "respondOff": return qsTr("Stop answering")
        case "filterAuto": return qsTr("Auto filter")
        case "addToFilter": return qsTr("Add address")
        case "setAddress":
        case "freeAddress": return qsTr("Free address")
        case "makePing": return qsTr("Make it a ping")
        case "removeFollowUp": return qsTr("Remove follow-up")
        case "clearRewrite": return qsTr("Remove rewrite")
        case "fitRange": return qsTr("Max range %1 m").arg(p.fix.rangeM)
        case "cmdAuto": return qsTr("Auto cmd")
        case "stepAuto": return qsTr("Auto duration")
        case "executorHost": return qsTr("App keeps time")
        case "bind": return qsTr("Bind…")
        }
        return ""
    }
}

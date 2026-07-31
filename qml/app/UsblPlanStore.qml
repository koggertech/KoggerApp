import QtQuick 2.15
import QtCore

// Host-side model for the USBL command plan.
//
// A plan item ("group") owns a SET of cmd slots and declares both halves of a
// transaction. `role` selects which half is written: the initiator writes its ping
// schedule plus on-response slots, the transponder writes on-request slots plus its
// response gating. See docs/KoggerApp-Docs/usbl-protocol.md.
//
// Ownership of a slot is exclusive — taking a slot removes it from the group that held
// it — so the eight hardware slots are always cleanly partitioned and there is never a
// write-order question about which config wins.
QtObject {
    id: store

    readonly property int slotCount: 8
    readonly property var groupColors: ["#3E8FD6", "#7C5CD3", "#16A34A", "#E0902B", "#0E9BB5", "#D6539B"]

    // Payload FORMAT only. The dispositions that also live in the Function enums are not
    // formats and are offered separately. A format's wire value depends on the struct that
    // carries it: USBLCmdSlotConfig numbers formats 3/4, USBLCmdConfig 1/2.
    readonly property var formats: [
        { "id": "bits",  "label": qsTr("Bit array"),          "slotWire": 3, "cfgWire": 1, "sized": true  },
        { "id": "llgeo", "label": qsTr("Position + azimuth"), "slotWire": 4, "cfgWire": 2, "sized": false }
    ]
    // Slot-level disposition, expressible only while no payload section is attached:
    // USBLCmdSlotConfig has a single `function` field.
    readonly property var dispositions: [
        { "id": "ack",    "label": qsTr("Acknowledge"),   "wire": 2 },
        { "id": "silent", "label": qsTr("Stay silent"),   "wire": 1 },
        { "id": "off",    "label": qsTr("Turn slot off"), "wire": 0 }
    ]
    // USBLPingRequest::Function — a different enum: BitArray is 1 here, 3 in SlotConfig.
    readonly property var pingFunctions: [
        { "id": "default", "label": qsTr("Plain interrogation"),      "wire": 0, "payload": false },
        { "id": "bits",    "label": qsTr("Carry payload bytes"),      "wire": 1, "payload": true  },
        { "id": "llgeo",   "label": qsTr("Carry position + azimuth"), "wire": 2, "payload": false }
    ]

    // Bumped on every mutation; derived properties depend on it so bindings re-evaluate
    // without deep-copying the arrays (see qml-widgets.md → Reactivity).
    property int rev: 0
    property var groups: []
    property var nodes: []
    property string role: "initiator"
    property int activeGroup: 0
    property bool appliedOnce: false
    property string appliedSnapshot: ""
    property int nextId: 1

    // Slots this host has actually written, as a set of "cmd:eventFilter" keys. Persisted,
    // because the DEVICE keeps its slot table across app restarts while ID_USBL_CONTROL
    // offers no read-back — if the host forgets, nothing can ever clean those slots up.
    property var written: ({})

    signal planChanged()

    // Settings needs an explicit id for its declared properties to be addressable — same
    // shape as WorkspaceStore's persisted blocks.
    property Settings persisted: Settings {
        id: usblPersisted
        category: "main/usbl"
        property string planJson: ""
    }
    property alias planJson: usblPersisted.planJson

    // ── ids / lookups ─────────────────────────────────────────────────────
    function _id() { nextId = nextId + 1; return nextId }

    function groupById(id) {
        for (var i = 0; i < groups.length; ++i)
            if (groups[i].id === id) return groups[i]
        return null
    }
    function groupIndexById(id) {
        for (var i = 0; i < groups.length; ++i)
            if (groups[i].id === id) return i
        return -1
    }
    function colorOf(g) {
        var i = groupIndexById(g ? g.id : -1)
        return groupColors[(i < 0 ? 0 : i) % groupColors.length]
    }
    function hasSlot(g, cmd) { return g && g.slots.indexOf(cmd) >= 0 }
    function ownerOf(cmd) {
        for (var i = 0; i < groups.length; ++i)
            if (hasSlot(groups[i], cmd)) return groups[i]
        return null
    }
    function slotCountOf(g) { return g ? g.slots.length : 0 }
    function schedulable(g) { return !!(g && g.ini.send && g.slots.length > 0) }

    // Compact "0,2–5,7" rendering of a slot set.
    function slotLabel(g) {
        if (!g || !g.slots.length) return qsTr("none")
        var s = g.slots, runs = [], start = s[0], prev = s[0]
        for (var i = 1; i <= s.length; ++i) {
            if (i < s.length && s[i] === prev + 1) { prev = s[i]; continue }
            runs.push(start === prev ? String(start) : start + "–" + prev)
            if (i < s.length) { start = s[i]; prev = s[i] }
        }
        return runs.join(",")
    }

    function payloadBytes(hex) {
        if (!hex) return 0
        return Math.floor(String(hex).replace(/[^0-9a-fA-F]/g, "").length / 2)
    }
    function findBy(arr, id) {
        for (var i = 0; i < arr.length; ++i)
            if (arr[i].id === id) return arr[i]
        return arr[0]
    }

    // ── construction ──────────────────────────────────────────────────────
    function newTrigger() {
        return {
            "disposition": "ack",
            "recv": null,          // { fmt, bits }
            "send": null,          // { fmt, payload }
            "advOpen": false,
            "adv": { "eventAction": "Swap", "cmdIdAction": "Incoming", "cmdIdRepl": 0,
                     "addrAction": "Incoming", "addrRepl": 0 }
        }
    }
    function newSend() { return { "fn": "default", "payload": "", "reply": 20000 } }
    function newGroup(slots) {
        return { "id": _id(), "slots": (slots || []).slice().sort(function (a, b) { return a - b }),
                 "ini": { "send": newSend(), "reply": null }, "tr": { "request": null } }
    }

    function addGroup() {
        var free = []
        for (var c = 0; c < slotCount; ++c)
            if (!ownerOf(c)) free.push(c)
        var g = newGroup(free.length ? [free[0]] : [])
        groups = groups.concat([g])
        activeGroup = groups.length - 1
        _touch()
    }
    function removeGroup(id) {
        var i = groupIndexById(id)
        if (i < 0) return
        for (var n = 0; n < nodes.length; ++n)
            nodes[n].refs = nodes[n].refs.filter(function (r) { return r.group !== id })
        var next = groups.slice()
        next.splice(i, 1)
        groups = next
        if (activeGroup >= groups.length) activeGroup = Math.max(0, groups.length - 1)
        _touch()
    }

    function addNode() {
        var used = nodes.map(function (n) { return n.addr })
        var a = 1
        while (used.indexOf(a) >= 0 && a < 8) ++a
        nodes = nodes.concat([{ "id": _id(), "addr": a, "active": true, "refs": [] }])
        _touch()
    }
    function removeNode(id) {
        nodes = nodes.filter(function (n) { return n.id !== id })
        _touch()
    }
    function setNodeAddr(id, addr) {
        var n = _node(id); if (!n) return
        n.addr = Math.max(0, Math.min(8, addr)); _touch()
    }
    function toggleNode(id) {
        var n = _node(id); if (!n) return
        n.active = !n.active; _touch()
    }
    function _node(id) {
        for (var i = 0; i < nodes.length; ++i)
            if (nodes[i].id === id) return nodes[i]
        return null
    }
    function addStep(nodeId, groupId) {
        var n = _node(nodeId), g = groupById(groupId)
        if (!n || !g || !g.slots.length) return
        n.refs = n.refs.concat([{ "group": g.id, "cmd": g.slots[0] }])
        _touch()
    }
    function removeStep(nodeId, index) {
        var n = _node(nodeId); if (!n) return
        var next = n.refs.slice(); next.splice(index, 1); n.refs = next
        _touch()
    }
    function setStepCmd(nodeId, index, cmd) {
        var n = _node(nodeId); if (!n || !n.refs[index]) return
        n.refs[index].cmd = cmd; _touch()
    }

    // Exclusive assignment: taking a slot removes it from the group that held it.
    // Returns the group it was taken from, or null.
    function toggleSlot(groupId, cmd) {
        var g = groupById(groupId); if (!g) return null
        var prev = null
        if (hasSlot(g, cmd)) {
            g.slots = g.slots.filter(function (v) { return v !== cmd })
        } else {
            prev = ownerOf(cmd)
            if (prev) prev.slots = prev.slots.filter(function (v) { return v !== cmd })
            g.slots = g.slots.concat([cmd]).sort(function (a, b) { return a - b })
        }
        _clampRefs()
        _touch()
        return prev
    }

    // A scheduled step must name a slot its group still owns; otherwise fall back to the
    // group's first remaining slot, or drop the step if the group owns none.
    function _clampRefs() {
        for (var i = 0; i < nodes.length; ++i) {
            var n = nodes[i]
            n.refs = n.refs.filter(function (r) {
                var g = groupById(r.group)
                if (!g || !g.slots.length) return false
                if (!hasSlot(g, r.cmd)) r.cmd = g.slots[0]
                return true
            })
        }
    }

    // ── trigger / section mutation ────────────────────────────────────────
    function setSendField(groupId, field, value) {
        var g = groupById(groupId); if (!g || !g.ini.send) return
        g.ini.send[field] = value; _touch()
    }
    function attachSend(groupId) {
        var g = groupById(groupId); if (!g) return
        g.ini.send = newSend(); _touch()
    }
    function detachSend(groupId) {
        var g = groupById(groupId); if (!g) return
        g.ini.send = null
        for (var n = 0; n < nodes.length; ++n)
            nodes[n].refs = nodes[n].refs.filter(function (r) { return r.group !== groupId })
        _touch()
    }
    function trigger(groupId, which) {
        var g = groupById(groupId); if (!g) return null
        return which === "request" ? g.tr.request : g.ini.reply
    }
    function attachTrigger(groupId, which) {
        var g = groupById(groupId); if (!g) return
        if (which === "request") g.tr.request = newTrigger()
        else g.ini.reply = newTrigger()
        _touch()
    }
    function detachTrigger(groupId, which) {
        var g = groupById(groupId); if (!g) return
        if (which === "request") g.tr.request = null
        else g.ini.reply = null
        _touch()
    }
    function setDisposition(groupId, which, id) {
        var t = trigger(groupId, which); if (!t) return
        t.disposition = id; _touch()
    }
    function attachSection(groupId, which, side) {
        var t = trigger(groupId, which); if (!t) return
        if (side === "recv") t.recv = { "fmt": "bits", "bits": 0 }
        else t.send = { "fmt": "bits", "payload": "" }
        _touch()
    }
    function detachSection(groupId, which, side) {
        var t = trigger(groupId, which); if (!t) return
        if (side === "recv") t.recv = null
        else t.send = null
        _touch()
    }
    function setSectionField(groupId, which, side, field, value) {
        var t = trigger(groupId, which); if (!t) return
        var s = side === "recv" ? t.recv : t.send
        if (!s) return
        s[field] = value; _touch()
    }
    function setAdvOpen(groupId, which, open) {
        var t = trigger(groupId, which); if (!t) return
        t.advOpen = open
        if (!open) t.adv = { "eventAction": "Swap", "cmdIdAction": "Incoming", "cmdIdRepl": 0,
                             "addrAction": "Incoming", "addrRepl": 0 }
        _touch()
    }
    function setAdvField(groupId, which, field, value) {
        var t = trigger(groupId, which); if (!t) return
        t.adv[field] = value; _touch()
    }

    function sectionCount(t) { return (t && t.recv ? 1 : 0) + (t && t.send ? 1 : 0) }
    function hasRewrite(t) {
        return !!t && (t.adv.cmdIdAction === "Replacement" || t.adv.addrAction === "Replacement"
                    || t.adv.eventAction === "Same")
    }
    // Two attached sections need USBLCmdConfig — the only struct with both a
    // receiver_function and a sender_function. Rewrite rules force it too, and there an
    // unattached section emits Default = 0.
    function structOf(t) {
        if (!t) return ""
        return (sectionCount(t) === 2 || hasRewrite(t)) ? "USBLCmdConfig" : "USBLCmdSlotConfig"
    }

    // ── derived roles (never configured) ──────────────────────────────────
    function subroleInitiator(g) {
        if (!g || !g.ini.send) return qsTr("not an initiator")
        return g.ini.send.reply > 0 ? qsTr("interrogator") : qsTr("pinger / beacon")
    }
    function subroleTransponder(g) {
        var t = g ? g.tr.request : null
        if (!t) return qsTr("default transponder")
        if (t.disposition === "off" && sectionCount(t) === 0) return qsTr("disabled")
        if (t.disposition === "silent" && sectionCount(t) === 0) return qsTr("silent receiver")
        if (t.recv && t.send) return qsTr("slave data transceiver")
        if (t.send) return qsTr("data source")
        if (t.recv) return qsTr("data sink")
        return qsTr("transponder")
    }

    // ── derived views ─────────────────────────────────────────────────────
    readonly property var schedule: {
        var _d = rev
        var out = []
        for (var i = 0; i < nodes.length; ++i) {
            var n = nodes[i]
            if (!n.active) continue
            if (!n.refs.length) {
                out.push({ "addr": n.addr, "cmd": 0, "implicit": true, "nodeId": n.id, "groupId": -1 })
                continue
            }
            for (var j = 0; j < n.refs.length; ++j)
                out.push({ "addr": n.addr, "cmd": n.refs[j].cmd, "implicit": false,
                           "nodeId": n.id, "groupId": n.refs[j].group })
        }
        return out
    }
    readonly property int activeNodeCount: {
        var _d = rev
        return nodes.filter(function (n) { return n.active }).length
    }
    readonly property var coverage: {
        var _d = rev
        var map = []
        for (var c = 0; c < slotCount; ++c) {
            var o = ownerOf(c)
            map.push({ "cmd": c, "groupId": o ? o.id : -1,
                       "index": o ? groupIndexById(o.id) : -1 })
        }
        return map
    }
    // Slot frames scale with the slot set — cmd_id is a single uint8 with no wildcard.
    readonly property int setupFrames: {
        var _d = rev
        var n = 0
        for (var i = 0; i < groups.length; ++i) {
            var g = groups[i]
            if (role === "initiator") { if (g.ini.reply) n += g.slots.length }
            else if (g.tr.request) n += g.slots.length
        }
        if (role !== "initiator") n += 2   // USBLResponseAddressFilter + transponder enable
        return n
    }
    readonly property int runFrames: role === "initiator" ? schedule.length : 0

    // ── release tracking ──────────────────────────────────────────────────
    // A role only ever writes its own event (initiator → ev 2, transponder → ev 1), so the
    // diff is confined to that event. Otherwise applying as one role would "release" the
    // slots the other role legitimately wrote.
    readonly property int roleEvent: role === "initiator" ? 2 : 1

    function plannedWrites() {
        var out = {}
        var ev = roleEvent
        for (var i = 0; i < groups.length; ++i) {
            var g = groups[i]
            var t = role === "initiator" ? g.ini.reply : g.tr.request
            if (!t) continue
            for (var j = 0; j < g.slots.length; ++j)
                out[g.slots[j] + ":" + ev] = true
        }
        return out
    }

    // Slots this role wrote before and no longer configures. Detaching a handler host-side
    // does not change the device, so these have to be explicitly switched off.
    function staleWrites() {
        var ev = roleEvent
        var planned = plannedWrites()
        var out = []
        for (var k in written) {
            var parts = String(k).split(":")
            if (parseInt(parts[1], 10) !== ev) continue
            if (!planned[k]) out.push({ "cmd": parseInt(parts[0], 10), "event": ev })
        }
        return out
    }

    readonly property int releaseFrames: {
        var _d = rev
        var _a = appliedOnce
        return staleWrites().length
    }

    function _recordWrites() {
        var ev = roleEvent
        var planned = plannedWrites()
        var next = {}
        // Entries for the other event belong to the other role — leave them intact.
        for (var k in written)
            if (parseInt(String(k).split(":")[1], 10) !== ev) next[k] = true
        for (var p in planned) next[p] = true
        written = next
        _save()
    }

    readonly property var issues: {
        var _d = rev
        var out = []
        var gaps = []
        for (var c = 0; c < slotCount; ++c)
            if (!ownerOf(c)) gaps.push(c)
        if (gaps.length)
            out.push({ "key": qsTr("slots"), "sev": "warn",
                       "text": qsTr("%1 belong to no group — those slots keep whatever the device already holds")
                               .arg(gaps.join(", ")) })
        for (var i = 0; i < groups.length; ++i) {
            var g = groups[i], k = "cmd " + slotLabel(g)
            if (!g.slots.length)
                out.push({ "key": qsTr("group %1").arg(i + 1), "sev": "warn",
                           "text": qsTr("owns no slots, so it writes nothing and cannot be scheduled") })
            var snd = g.ini.send, rq = g.tr.request, rs = g.ini.reply
            var outBits = (snd && snd.fn === "bits") ? payloadBytes(snd.payload) * 8 : 0
            if (outBits > 0 && !(rq && rq.recv))
                out.push({ "key": k, "sev": "warn",
                           "text": qsTr("request carries %1 bit, but the transponder half has no receive section").arg(outBits) })
            if (outBits > 0 && rq && rq.recv && rq.recv.fmt === "bits" && rq.recv.bits !== outBits)
                out.push({ "key": k, "sev": "crit",
                           "text": qsTr("request sends %1 bit, transponder expects %2 bit").arg(outBits).arg(rq.recv.bits) })
            if (rq && rq.send && rq.send.fmt === "bits") {
                var back = payloadBytes(rq.send.payload) * 8
                if (!(rs && rs.recv))
                    out.push({ "key": k, "sev": "warn",
                               "text": qsTr("transponder answers with %1 bit, but the initiator has no reply handler to read it").arg(back) })
                else if (rs.recv.fmt === "bits" && rs.recv.bits !== back)
                    out.push({ "key": k, "sev": "crit",
                               "text": qsTr("transponder answers %1 bit, reply handler expects %2 bit").arg(back).arg(rs.recv.bits) })
            }
            if (snd && snd.reply > 0 && rq && sectionCount(rq) === 0
                    && (rq.disposition === "silent" || rq.disposition === "off"))
                out.push({ "key": k, "sev": "crit",
                           "text": qsTr("interrogator waits for a reply, transponder is \"%1\" — every step times out")
                                   .arg(findBy(dispositions, rq.disposition).label) })
            if (snd && snd.reply === 0 && rq && sectionCount(rq) > 0)
                out.push({ "key": k, "sev": "warn",
                           "text": qsTr("pinger expects no reply, but the transponder half is configured to answer") })
            if (rs && !snd)
                out.push({ "key": k, "sev": "warn",
                           "text": qsTr("has a reply handler but never sends a request") })
        }
        return out
    }

    // Only the half this role actually writes counts toward staleness.
    readonly property string snapshot: {
        var _d = rev
        return JSON.stringify(groups.map(function (g) {
            return [g.slots, role === "initiator" ? g.ini.reply : g.tr.request]
        }))
    }
    readonly property bool stale: appliedOnce && appliedSnapshot !== snapshot

    // Records the write set as part of the same step, so a caller cannot mark the plan
    // applied while leaving the device's slot table untracked.
    function markApplied() {
        _recordWrites()
        appliedSnapshot = snapshot
        appliedOnce = true
    }
    function setRole(r) { role = r; appliedOnce = false; appliedSnapshot = "" }

    // ── persistence ───────────────────────────────────────────────────────
    function _touch() { rev = rev + 1; _save(); planChanged() }

    function _save() {
        usblPersisted.planJson = JSON.stringify({ "v": 2, "groups": groups, "nodes": nodes,
                                                  "nextId": nextId, "written": written })
    }
    function load() {
        var raw = usblPersisted.planJson
        if (raw && raw.length) {
            try {
                var d = JSON.parse(raw)
                if (d && d.groups && d.nodes) {
                    groups = d.groups
                    nodes = d.nodes
                    nextId = d.nextId || 1000
                    written = _normWritten(d.written)
                    _normalize()
                    rev = rev + 1
                    return
                }
            } catch (e) { /* fall through to defaults */ }
        }
        _defaults()
    }
    // A v1 payload has no write set; treat it as "nothing known written" rather than
    // inventing one, so the first Apply cannot release slots it never wrote.
    function _normWritten(raw) {
        var out = {}
        if (!raw) return out
        for (var k in raw) {
            var p = String(k).split(":")
            var cmd = parseInt(p[0], 10), ev = parseInt(p[1], 10)
            if (isNaN(cmd) || cmd < 0 || cmd >= slotCount) continue
            if (ev !== 1 && ev !== 2) continue
            out[cmd + ":" + ev] = true
        }
        return out
    }
    // Repair anything a hand-edited or older payload could get wrong: slot bounds,
    // exclusivity, missing sub-objects, dangling step references.
    function _normalize() {
        var seen = {}
        for (var i = 0; i < groups.length; ++i) {
            var g = groups[i]
            if (!g.ini) g.ini = { "send": null, "reply": null }
            if (!g.tr) g.tr = { "request": null }
            if (g.ini.reply) _normTrigger(g.ini.reply)
            if (g.tr.request) _normTrigger(g.tr.request)
            var keep = []
            var raw = Array.isArray(g.slots) ? g.slots : []
            for (var j = 0; j < raw.length; ++j) {
                var c = raw[j]
                if (c < 0 || c >= slotCount || seen[c]) continue
                seen[c] = true; keep.push(c)
            }
            g.slots = keep.sort(function (a, b) { return a - b })
        }
        _clampRefs()
    }
    function _normTrigger(t) {
        if (!t.adv) t.adv = { "eventAction": "Swap", "cmdIdAction": "Incoming", "cmdIdRepl": 0,
                              "addrAction": "Incoming", "addrRepl": 0 }
        if (t.recv && typeof t.recv.bits !== "number") t.recv.bits = 0
        if (t.send && typeof t.send.payload !== "string") t.send.payload = ""
    }
    function _defaults() {
        var baseline = newGroup([0, 4, 5, 6, 7])
        baseline.tr.request = newTrigger()
        var data = newGroup([1, 2, 3])
        data.ini.send.fn = "bits"
        data.ini.send.payload = "0A 1B"
        data.tr.request = newTrigger()
        data.tr.request.recv = { "fmt": "bits", "bits": 16 }
        data.tr.request.send = { "fmt": "bits", "payload": "FF 01" }
        data.ini.reply = newTrigger()
        data.ini.reply.recv = { "fmt": "bits", "bits": 16 }
        groups = [baseline, data]
        nodes = [{ "id": _id(), "addr": 1, "active": true,
                   "refs": [{ "group": data.id, "cmd": 1 }, { "group": data.id, "cmd": 2 },
                            { "group": data.id, "cmd": 3 }] },
                 { "id": _id(), "addr": 2, "active": true, "refs": [] }]
        activeGroup = 0
        written = ({})
        appliedOnce = false
        appliedSnapshot = ""
        rev = rev + 1
        _save()
    }
    function resetToDefaults() { _defaults(); planChanged() }
}

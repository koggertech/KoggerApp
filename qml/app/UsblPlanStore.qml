import QtQuick 2.15
import QtCore
import "UsblPlanLogic.js" as Logic

// QML face of the USBL command plan. All of the rules live in UsblPlanLogic.js; this file
// holds only three things QML has to own: the translated label tables, persistence, and the
// derived properties consumers bind to.
//
// THE ONE INVARIANT: `st` is replaced, never edited.
//
// Every mutator assigns a brand-new state object, so `st` changes identity and QML
// re-evaluates every binding that reads through it — including `plan.groups[0].ini.reply`,
// which is exactly what used to go stale. There is no `rev` discipline to remember and no
// snapshot workaround; a binding cannot be wrong about the plan any more.
//
// Editing `st` in place would silently restore the old bug, so don't. Route changes through
// the Logic functions, which return copies.
QtObject {
    id: store

    readonly property int slotCount: Logic.SLOT_COUNT
    readonly property var groupColors: ["#3E8FD6", "#7C5CD3", "#16A34A", "#E0902B", "#0E9BB5", "#D6539B"]

    // Payload FORMAT only. A format's wire value depends on the struct carrying it:
    // USBLCmdSlotConfig numbers formats 3/4, USBLCmdConfig 1/2 — see usbl-protocol.md.
    readonly property var formats: [
        { "id": "bits",  "label": qsTr("Raw bytes"),          "slotWire": 3, "cfgWire": 1, "sized": true  },
        { "id": "llgeo", "label": qsTr("Position + azimuth"), "slotWire": 4, "cfgWire": 2, "sized": false }
    ]
    // Slot-level disposition, expressible only while no payload section is attached: the
    // slot struct has a single `function` field.
    readonly property var dispositions: [
        { "id": "ack",    "label": qsTr("Acknowledge"),   "wire": 2 },
        { "id": "silent", "label": qsTr("Stay silent"),   "wire": 1 },
        { "id": "off",    "label": qsTr("Turn slot off"), "wire": 0 }
    ]
    // A different enum again: bit-array is 1 here and 3 in the slot struct.
    readonly property var pingFunctions: [
        { "id": "default", "label": qsTr("Plain interrogation"),      "wire": 0, "payload": false },
        { "id": "bits",    "label": qsTr("Carry payload bytes"),      "wire": 1, "payload": true  },
        { "id": "llgeo",   "label": qsTr("Carry position + azimuth"), "wire": 2, "payload": false }
    ]

    // ── the state ─────────────────────────────────────────────────────────
    property var st: Logic.emptyState()

    // Retained only so existing `_rev` touches in the UI keep compiling; nothing needs it
    // now that `st` changes identity. New bindings should not reference it.
    property int rev: 0

    signal planChanged()

    readonly property var groups: st.groups
    readonly property var nodes: st.nodes
    readonly property string role: st.role
    readonly property bool appliedOnce: st.appliedOnce
    readonly property var written: st.written

    // Writable, because consumers do `plan.activeGroup = i`. Mirrored into `st` one way; the
    // property stays the source of truth for reads.
    property int activeGroup: 0
    onActiveGroupChanged: if (st.activeGroup !== activeGroup) st = Logic.setActiveGroup(st, activeGroup)

    // ── derived (all re-evaluate when `st` is replaced) ────────────────────
    readonly property var coverage: Logic.coverage(st)
    readonly property var nodesView: Logic.nodesView(st)
    readonly property int activeSlotCount: Logic.activeSlotCount(st)
    readonly property int activeNodeCount: Logic.activeNodeCount(st)
    readonly property var schedule: Logic.schedule(st)
    readonly property int setupFrames: Logic.setupFrames(st)
    readonly property int runFrames: Logic.runFrames(st)
    readonly property int releaseFrames: Logic.releaseFrames(st)
    readonly property string snapshot: Logic.snapshot(st)
    readonly property bool stale: Logic.isStale(st)
    readonly property int roleEvent: Logic.roleEvent(st)

    // Colour is a QML concern, so it is grafted on here rather than in the logic module.
    readonly property var groupsView: {
        var base = Logic.groupsView(st), out = []
        for (var i = 0; i < base.length; ++i) {
            var v = base[i]
            out.push({ "id": v.id, "index": v.index, "label": v.label, "count": v.count,
                       "slots": v.slots, "schedulable": v.schedulable,
                       "color": groupColors[i % groupColors.length] })
        }
        return out
    }

    // ── pure reads, forwarded ─────────────────────────────────────────────
    function groupById(id)       { return Logic.groupById(st, id) }
    function groupIndexById(id)  { return Logic.groupIndexById(st, id) }
    function ownerOf(cmd)        { return Logic.ownerOf(st, cmd) }
    function hasSlot(g, cmd)     { return Logic.hasSlot(g, cmd) }
    function slotCountOf(g)      { return g ? g.slots.length : 0 }
    function schedulable(g)      { return Logic.schedulable(g) }
    function payloadBytes(hex)   { return Logic.payloadBytes(hex) }
    function trigger(gid, which) { return Logic.trigger(st, gid, which) }
    function sectionCount(t)     { return Logic.sectionCount(t) }
    function hasRewrite(t)       { return Logic.hasRewrite(t) }
    function structOf(t)         { return Logic.structOf(t) }
    function plannedWrites()     { return Logic.plannedWrites(st) }
    function staleWrites()       { return Logic.staleWrites(st) }
    function colorOf(g) {
        var i = Logic.groupIndexById(st, g ? g.id : -1)
        return groupColors[(i < 0 ? 0 : i) % groupColors.length]
    }
    // "none" is user-visible, so the empty case is translated here rather than in the
    // logic module.
    function slotLabel(g) {
        var s = Logic.slotLabel(g)
        return s.length ? s : qsTr("none")
    }
    function findBy(arr, id) {
        for (var i = 0; i < arr.length; ++i)
            if (arr[i].id === id) return arr[i]
        return arr[0]
    }

    // ── mutators ──────────────────────────────────────────────────────────
    function _commit(next) {
        if (next === st) return false      // no-op: do not churn every binding for nothing
        st = next
        if (activeGroup !== st.activeGroup) activeGroup = st.activeGroup
        rev = rev + 1
        _save()
        planChanged()
        return true
    }

    function addGroup()                 { _commit(Logic.addGroup(st)) }
    function removeGroup(id)            { _commit(Logic.removeGroup(st, id)) }
    function addNode()                  { _commit(Logic.addNode(st)) }
    function removeNode(id)             { _commit(Logic.removeNode(st, id)) }
    function setNodeAddr(id, addr)      { _commit(Logic.setNodeAddr(st, id, addr)) }
    function toggleNode(id)             { _commit(Logic.toggleNode(st, id)) }
    function addStep(nodeId, groupId)   { _commit(Logic.addStep(st, nodeId, groupId)) }
    function removeStep(nodeId, index)  { _commit(Logic.removeStep(st, nodeId, index)) }
    function setStepCmd(nId, i, cmd)    { _commit(Logic.setStepCmd(st, nId, i, cmd)) }

    // Returns the group the slot was taken from, or null — the caller shows that in a note.
    function toggleSlot(groupId, cmd) {
        var r = Logic.toggleSlot(st, groupId, cmd)
        var from = r.takenFrom
        _commit(r.state)
        return from >= 0 ? Logic.groupById(st, from) : null
    }

    function setSendField(gid, f, v)        { _commit(Logic.setSendField(st, gid, f, v)) }
    function attachSend(gid)                { _commit(Logic.attachSend(st, gid)) }
    function detachSend(gid)                { _commit(Logic.detachSend(st, gid)) }
    function attachTrigger(gid, which)      { _commit(Logic.attachTrigger(st, gid, which)) }
    function detachTrigger(gid, which)      { _commit(Logic.detachTrigger(st, gid, which)) }
    function setDisposition(gid, which, id) { _commit(Logic.setDisposition(st, gid, which, id)) }
    function attachSection(gid, w, side)    { _commit(Logic.attachSection(st, gid, w, side)) }
    function detachSection(gid, w, side)    { _commit(Logic.detachSection(st, gid, w, side)) }
    function setSectionField(gid, w, side, f, v) {
        _commit(Logic.setSectionField(st, gid, w, side, f, v))
    }
    function setAdvOpen(gid, w, open)       { _commit(Logic.setAdvOpen(st, gid, w, open)) }
    function setAdvField(gid, w, f, v)      { _commit(Logic.setAdvField(st, gid, w, f, v)) }
    function setRole(r)                     { _commit(Logic.setRole(st, r)) }
    function markApplied()                  { _commit(Logic.markApplied(st)) }

    // ── derived roles, translated from the logic module's codes ────────────
    readonly property var _subroleText: ({
        "notInitiator":       qsTr("not an initiator"),
        "interrogator":       qsTr("interrogator"),
        "pinger":            qsTr("pinger / beacon"),
        "defaultTransponder": qsTr("default transponder"),
        "disabled":          qsTr("disabled"),
        "silentReceiver":    qsTr("silent receiver"),
        "relay":             qsTr("data relay"),
        "source":            qsTr("data source"),
        "sink":              qsTr("data sink"),
        "transponder":       qsTr("transponder")
    })
    function subroleInitiator(g)   { return _subroleText[Logic.subroleInitiatorCode(g)] || "" }
    function subroleTransponder(g) { return _subroleText[Logic.subroleTransponderCode(g)] || "" }

    // ── consistency check: codes in, sentences out ────────────────────────
    readonly property var issues: {
        var raw = Logic.issueCodes(st), out = []
        for (var i = 0; i < raw.length; ++i) {
            var e = raw[i]
            var key = e.code === "unownedSlots"     ? qsTr("slots")
                    : e.code === "groupOwnsNoSlots" ? qsTr("group %1").arg(e.group + 1)
                                                    : "cmd " + (e.key.length ? e.key : qsTr("none"))
            out.push({ "key": key, "sev": e.sev, "text": _issueText(e) })
        }
        return out
    }
    function _issueText(e) {
        switch (e.code) {
        case "unownedSlots":
            return qsTr("%1 belong to no group — those slots keep whatever the device already holds")
                   .arg(e.slots.join(", "))
        case "groupOwnsNoSlots":
            return qsTr("owns no slots, so it writes nothing and cannot be scheduled")
        case "requestCarriesButNoReceiver":
            return qsTr("request carries %1 bit, but the transponder half has no receive section")
                   .arg(e.bits)
        case "requestBitsMismatch":
            return qsTr("request sends %1 bit, transponder expects %2 bit")
                   .arg(e.bits).arg(e.expected)
        case "answerButNoReplyHandler":
            return qsTr("transponder answers with %1 bit, but the initiator has no reply handler to read it")
                   .arg(e.bits)
        case "answerBitsMismatch":
            return qsTr("transponder answers %1 bit, reply handler expects %2 bit")
                   .arg(e.bits).arg(e.expected)
        case "interrogatorVsSilent":
            return qsTr("interrogator waits for a reply, transponder is \"%1\" — every step times out")
                   .arg(findBy(dispositions, e.disposition).label)
        case "pingerVsAnswering":
            return qsTr("pinger expects no reply, but the transponder half is configured to answer")
        case "replyHandlerWithoutRequest":
            return qsTr("has a reply handler but never sends a request")
        }
        return e.code
    }

    // ── persistence ───────────────────────────────────────────────────────
    // Settings needs an explicit id for its declared properties to be addressable — same
    // shape WorkspaceStore's persisted blocks use.
    property Settings persisted: Settings {
        id: usblPersisted
        category: "main/usbl"
        property string planJson: ""
    }
    property alias planJson: usblPersisted.planJson

    function _save() { usblPersisted.planJson = Logic.serialize(st) }

    function load() {
        var raw = usblPersisted.planJson
        // deserialize() repairs rather than trusts: out-of-range or doubly-owned slots,
        // missing sub-objects and dangling step references are all fixed on read.
        var next = (raw && raw.length) ? Logic.deserialize(raw) : Logic.defaults()
        if (!next.groups.length && !next.nodes.length) next = Logic.defaults()
        st = next
        activeGroup = st.activeGroup
        rev = rev + 1
        planChanged()
    }
    function resetToDefaults() {
        st = Logic.defaults()
        activeGroup = st.activeGroup
        rev = rev + 1
        _save()
        planChanged()
    }
}

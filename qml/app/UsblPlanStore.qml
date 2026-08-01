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
    readonly property int maxGroups: Logic.MAX_GROUPS
    readonly property var groupColors: ["#3E8FD6", "#7C5CD3", "#16A34A", "#E0902B", "#0E9BB5", "#D6539B"]

    // Payload format. `wire` is the Function value in both v1 USBLPingRequest and
    // v6 USBLCmdConfig — the two enums agree.
    readonly property var formats: [
        { "id": "bits",  "label": qsTr("Raw bytes"),          "wire": 1, "sized": true  },
        { "id": "llgeo", "label": qsTr("Position + azimuth"), "wire": 2, "sized": false }
    ]
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
    readonly property bool anyStale: Logic.anyStale(st)
    // A property, not a function: the add chip's `visible` is a binding, and a function
    // call in a binding is not a tracked dependency.
    readonly property bool canAddGroup: Logic.canAddGroup(st)

    // Per-role apply state as a PROPERTY, not a function. A function call in a binding is
    // not a tracked dependency, so `plan.applyFrames("initiator")` would render once and
    // then lie -- the exact failure this store was rewritten to make impossible. One
    // property recomputed when `st` is replaced; QML indexes into it by role.
    readonly property var applyInfo: {
        var out = {}
        for (var i = 0; i < Logic.ROLES.length; ++i) {
            var r = Logic.ROLES[i]
            out[r] = { "frames": Logic.applyFrames(st, r),
                       "configured": Logic.configuredSlots(st, r),
                       "applied": Logic.appliedOnce(st, r),
                       "stale": Logic.isStale(st, r) }
        }
        return out
    }

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
    // Every ID_USBL_CONTROL v6 write Apply will emit for `role`, already reduced to wire
    // values -- all eight slots, always. Only ever called from the click handler, never
    // from a binding. Tested by tools/qml_test/test_usbl_plan_logic.mjs.
    function applyWrites(role)   { return Logic.applyWrites(st, role) }
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

    // The one slot interaction — see Logic.slotClick. Returns what happened
    // (select | release | assign | create | none) so the caller can say so.
    function slotClick(cmd) {
        var r = Logic.slotClick(st, cmd)
        _commit(r.state)
        return r.action
    }

    function setSendField(gid, f, v)        { _commit(Logic.setSendField(st, gid, f, v)) }
    function attachSend(gid)                { _commit(Logic.attachSend(st, gid)) }
    function detachSend(gid)                { _commit(Logic.detachSend(st, gid)) }
    function attachTrigger(gid, which)      { _commit(Logic.attachTrigger(st, gid, which)) }
    function detachTrigger(gid, which)      { _commit(Logic.detachTrigger(st, gid, which)) }
    function attachSection(gid, w, side)    { _commit(Logic.attachSection(st, gid, w, side)) }
    function detachSection(gid, w, side)    { _commit(Logic.detachSection(st, gid, w, side)) }
    function setSectionField(gid, w, side, f, v) {
        _commit(Logic.setSectionField(st, gid, w, side, f, v))
    }
    function setAdvOpen(gid, w, open)       { _commit(Logic.setAdvOpen(st, gid, w, open)) }
    function setAdvField(gid, w, f, v)      { _commit(Logic.setAdvField(st, gid, w, f, v)) }
    function markApplied(role)              { _commit(Logic.markApplied(st, role)) }

    // ── derived roles, translated from the logic module's codes ────────────
    readonly property var _subroleText: ({
        "notInitiator":       qsTr("not an initiator"),
        "interrogator":       qsTr("interrogator"),
        "pinger":            qsTr("pinger / beacon"),
        "defaultTransponder": qsTr("default transponder"),
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
            return qsTr("%1 belong to no group — Apply resets them to defaults on the device")
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

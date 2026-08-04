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
    property var st: Logic.initialState()

    // Retained only so existing `_rev` touches in the UI keep compiling; nothing needs it
    // now that `st` changes identity. New bindings should not reference it.
    property int rev: 0

    signal planChanged()

    readonly property var groups: st.groups
    readonly property var nodes: st.nodes

    // ── derived (all re-evaluate when `st` is replaced) ────────────────────
    // Selection is READ-ONLY here and changed through selectGroup(). It used to be a writable
    // property mirrored into `st` by a handler, which meant two places could disagree about
    // which group was open -- and selecting now has a side effect (an unfilled group
    // dissolves), so it has to go through _commit like every other edit or the dissolve would
    // never be saved.
    readonly property int activeId: st.activeId
    // The selected group. Consumers read this instead of indexing `groups` themselves, so
    // "which group is open" has exactly one answer. Never null in practice: the partition
    // guarantees a group exists and activeGroupOf falls back to the first.
    readonly property var activeGroupObj: Logic.activeGroupOf(st)
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

    // Colour and the `def` name are QML concerns, so they are grafted on here rather than in
    // the logic module. `isDefault` comes from the logic and follows the SETTINGS -- it moves
    // from group to group as they are edited, and may be true of none or (until the plan
    // check's join is taken) of several.
    readonly property var groupsView: {
        var base = Logic.groupsView(st), out = []
        for (var i = 0; i < base.length; ++i) {
            var v = base[i]
            out.push({ "id": v.id, "index": v.index, "label": v.label, "count": v.count,
                       "slots": v.slots, "schedulable": v.schedulable,
                       "isDefault": v.isDefault, "name": nameOf(v.index),
                       "color": groupColors[i % groupColors.length] })
        }
        return out
    }
    // What a tab and a coverage cell call a group: ALWAYS its number.
    //
    // Naming the default-settings group `def` instead was tried and is wrong. Duplicates are
    // legal, so "the group at the defaults" is routinely several groups -- a fresh plan is
    // every group -- and every tab then read `def`, identifying nothing. Identity wins the
    // two lines a cell has; which settings a group carries is said in its pane, where there
    // is room for a sentence.
    function nameOf(index) { return "G" + (index + 1) }
    // Sets of groups holding identical settings. Legal, and only the plan check cares.
    readonly property var duplicateSets: Logic.duplicateSets(st)

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
    // The name of a group object, for the panes that hold one rather than a view row.
    function nameOfGroup(g) { return nameOf(Logic.groupIndexById(st, g ? g.id : -1)) }
    // Whether a group carries the settings a slot has when nothing was configured. Said in
    // words in the pane, because that is where there is room to say it.
    function isAtDefaults(g) { return Logic.isDefaultSettings(g) }
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
        rev = rev + 1
        _save()
        planChanged()
        return true
    }

    // Selecting is an edit, not just a view change: an unfilled group dissolves when the
    // selection leaves it, and that has to be saved like anything else.
    function selectGroup(id)            { _commit(Logic.setActiveGroup(st, id)) }

    function addGroup()                 { _commit(Logic.addGroup(st)) }
    // Not a delete -- the slots move to the def group and this one dissolves. See the logic
    // module: under a total partition the slots have to go somewhere.
    function removeGroup(id)            { _commit(Logic.removeGroup(st, id)) }
    // The plan check's fix for duplicate groups. Writes nothing new to the device.
    function joinGroups(ids)            { _commit(Logic.joinGroups(st, ids)) }
    function addNode()                  { _commit(Logic.addNode(st)) }
    function removeNode(id)             { _commit(Logic.removeNode(st, id)) }
    function setNodeAddr(id, addr)      { _commit(Logic.setNodeAddr(st, id, addr)) }
    function toggleNode(id)             { _commit(Logic.toggleNode(st, id)) }
    function addStep(nodeId, groupId)   { _commit(Logic.addStep(st, nodeId, groupId)) }
    function removeStep(nodeId, index)  { _commit(Logic.removeStep(st, nodeId, index)) }
    // Mute a step without losing it. Not the same as the node's switch, which stops the node.
    function toggleStep(nodeId, index)  { _commit(Logic.toggleStep(st, nodeId, index)) }
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
    // An entry may carry a FIX: `action` names it and `ids` are what it applies to, so the
    // delegate can offer a button instead of leaving the operator to work out the edit.
    readonly property var issues: {
        var raw = Logic.issueCodes(st), out = []
        for (var i = 0; i < raw.length; ++i) {
            var e = raw[i]
            var key = e.code === "duplicateGroups"  ? _groupNames(e.ids).join(" · ")
                    : e.code === "groupOwnsNoSlots" ? nameOf(e.group)
                                                    : "cmd " + (e.key.length ? e.key : qsTr("none"))
            out.push({ "key": key, "sev": e.sev, "text": _issueText(e),
                       "action": e.action ? e.action : "", "ids": e.ids ? e.ids : [],
                       "fixLabel": e.action === "join" ? qsTr("Join") : "" })
        }
        return out
    }
    function _groupNames(ids) {
        var out = []
        for (var i = 0; i < (ids ? ids.length : 0); ++i)
            out.push(nameOfGroup(Logic.groupById(st, ids[i])))
        return out
    }
    function _issueText(e) {
        switch (e.code) {
        case "duplicateGroups":
            // Deliberately not phrased as a fault: identical groups write identical bytes to
            // their own slots, so there is nothing wrong with the plan -- only with how many
            // tabs it takes to read it.
            return qsTr("%1 hold identical settings, so they write the same bytes. "
                      + "Joining them changes nothing on the device.")
                   .arg(_groupNames(e.ids).join(", "))
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
        // deserialize() repairs rather than trusts: out-of-range or doubly-owned slots, slots
        // no group claims, saved empty groups, missing sub-objects and dangling step
        // references are all fixed on read, and the partition comes back total.
        var next = (raw && raw.length) ? Logic.deserialize(raw) : Logic.defaults()
        if (!next.nodes.length && !raw) next = Logic.defaults()
        st = next
        rev = rev + 1
        planChanged()
    }
    function resetToDefaults() {
        st = Logic.defaults()
        rev = rev + 1
        _save()
        planChanged()
    }
}

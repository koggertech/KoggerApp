// Pure logic for the USBL command plan. No QML types, no translations, no side effects.
//
// WHY THIS IS A SEPARATE FILE
//
// The plan used to live in UsblPlanStore.qml and was edited IN PLACE, with a `rev` counter
// as the only change signal. That made every binding a trap: `_g.ini.reply` or
// `plan.slotLabel(g)` reads correctly once and then silently renders stale forever, because
// QML never saw the object change. Three separate rounds of "the button does nothing" were
// all that one design decision.
//
// Here every mutator takes a state and returns a NEW state. QML reassigns one `var`
// property, its identity changes, and every dependent binding re-evaluates on its own. No
// `rev` discipline to remember, no snapshots, and the failure mode stops existing.
//
// Being plain JS is the second half of the point: `node` can test all of it with no Qt, no
// window and no GPU. Anything user-visible (labels, issue prose) stays in QML where qsTr
// lives -- this module returns CODES and numbers, never sentences.

// ── THE MODEL ────────────────────────────────────────────────────────────────
//
// The eight slots are the state. Each one is configured, always -- with the defaults if
// nothing else -- so THE SLOTS ARE TOTALLY PARTITIONED: every slot is in exactly one group,
// there is always at least one group, and there is no such thing as an unowned slot.
//
// A GROUP OWNS A SET OF SLOTS AND ONE SETTINGS OBJECT THEY SHARE. That sharing is the whole
// point of the concept: a group exists so several slots -- or all of them -- can be edited at
// once. It is not a protocol object; nothing on the wire knows about groups.
//
// Because the wire does not carry them, group membership cannot be read back from a device,
// only RECONSTRUCTED -- and the reconstruction rule is the definition: slots with the exact
// same settings belong in one group (see `reconstruct`).
//
// Two groups holding identical settings are LEGAL. Collapsing them the moment they match
// would fight the operator mid-edit, so duplicates are allowed to exist and the plan check
// offers to join them (`duplicateSets` -> `joinGroups`). Joining changes nothing a device
// sees -- Apply writes per-slot bytes, and duplicates write identical bytes -- so it is a
// suggestion about how the pane is organised, never a correctness gate.
//
// "def" is a LABEL, not a group: whichever group's settings equal the defaults renders as
// `def`. Edit it and the label moves off it. There may be no def group at all, and
// (transiently) more than one.
//
// ── WHY THIS IS A SEPARATE FILE ──────────────────────────────────────────────
//
// The plan used to live in UsblPlanStore.qml and was edited IN PLACE, with a `rev` counter
// as the only change signal. That made every binding a trap: `_g.ini.reply` or
// `plan.slotLabel(g)` reads correctly once and then silently renders stale forever, because
// QML never saw the object change. Three separate rounds of "the button does nothing" were
// all that one design decision.
//
// Here every mutator takes a state and returns a NEW state. QML reassigns one `var`
// property, its identity changes, and every dependent binding re-evaluates on its own. No
// `rev` discipline to remember, no snapshots, and the failure mode stops existing.
//
// Being plain JS is the second half of the point: `node` can test all of it with no Qt, no
// window and no GPU. Anything user-visible (labels, issue prose) stays in QML where qsTr
// lives -- this module returns CODES and numbers, never sentences.

var SLOT_COUNT = 8;
// Eight slots partition into at most eight non-empty groups, so a ninth could only ever be
// the empty one being edited -- and there would be no slot left to put in it.
var MAX_GROUPS = SLOT_COUNT;
var ROLES = ["initiator", "transponder"];

// ── state ────────────────────────────────────────────────────────────────────
// `applied` maps a role to the snapshot that was last written as that role. There is no
// current role: a role is an argument to Apply, not a mode the plan sits in.
//
// The starting plan is ONE group at the defaults holding all eight slots -- the honest
// reading of a device nobody has configured, and the smallest state the partition invariant
// allows. `activeId` names the selected group by id; an index shifts when a group dissolves.
function initialState() {
    var st = { groups: [], nodes: [], activeId: 0, nextId: 1, applied: {} };
    var all = [];
    for (var c = 0; c < SLOT_COUNT; ++c) all.push(c);
    st.groups = [newGroup(_next(st), all)];
    st.activeId = st.groups[0].id;
    return st;
}

// Deep copy per edit. A plan is at most 8 slots and 8 nodes, so the cost is irrelevant and
// the correctness is total: nothing can accidentally alias the previous state.
function clone(st) { return JSON.parse(JSON.stringify(st)); }

function _next(st) { st.nextId = st.nextId + 1; return st.nextId; }

function newSend()    { return { fn: "default", payload: "", reply: 20000 }; }
function newTrigger() {
    return { recv: null, send: null, advOpen: false,
             adv: { eventAction: "Swap", cmdIdAction: "Incoming", cmdIdRepl: 0,
                    addrAction: "Incoming", addrRepl: 0 } };
}
function newGroup(id, slots) {
    return { id: id, slots: (slots || []).slice().sort(function (a, b) { return a - b; }),
             ini: { send: newSend(), reply: null }, tr: { request: null } };
}

// ── group identity: the shared settings, canonically ─────────────────────────
// What a group's slots share, and therefore what makes two groups the same group. Slots, id
// and selection are deliberately NOT part of it.
function settingsOf(g) {
    return g ? { send: g.ini.send, reply: g.ini.reply, request: g.tr.request } : null;
}
// Key order is sorted rather than trusted: a blob written by an older build, or hand-edited,
// can carry the same settings with the fields in another order, and JSON.stringify would then
// call two identical groups different. Cheap at eight groups, and it makes `signature`
// depend on the values alone.
function _canon(v) {
    if (v === null || typeof v !== "object") return v;
    if (Array.isArray(v)) return v.map(_canon);
    var keys = Object.keys(v).sort(), out = {};
    for (var i = 0; i < keys.length; ++i) out[keys[i]] = _canon(v[keys[i]]);
    return out;
}
function signature(g) { return JSON.stringify(_canon(settingsOf(g))); }
// The `def` label: this group's settings are the ones a slot has when nothing was configured.
function isDefaultSettings(g) {
    return !!g && signature(g) === signature(newGroup(0, []));
}

// ── lookups (pure reads) ─────────────────────────────────────────────────────
function groupById(st, id) {
    for (var i = 0; i < st.groups.length; ++i)
        if (st.groups[i].id === id) return st.groups[i];
    return null;
}
function groupIndexById(st, id) {
    for (var i = 0; i < st.groups.length; ++i)
        if (st.groups[i].id === id) return i;
    return -1;
}
function nodeById(st, id) {
    for (var i = 0; i < st.nodes.length; ++i)
        if (st.nodes[i].id === id) return st.nodes[i];
    return null;
}
function hasSlot(g, cmd) { return !!g && g.slots.indexOf(cmd) >= 0; }
function ownerOf(st, cmd) {
    for (var i = 0; i < st.groups.length; ++i)
        if (hasSlot(st.groups[i], cmd)) return st.groups[i];
    return null;
}
function schedulable(g) { return !!(g && g.ini.send && g.slots.length > 0); }
// Selection is a group ID. Falls back to the first group rather than returning null: the
// partition guarantees one exists, and a pane with nothing selected is not a state the UI
// should have to render.
function activeGroupOf(st) {
    if (!st.groups.length) return null;
    var g = groupById(st, st.activeId);
    return g ? g : st.groups[0];
}
// The group currently carrying the defaults, if any. Ordinary lookup, not a special member:
// it is whichever group happens to match, and there may be none.
function defaultGroupOf(st) {
    for (var i = 0; i < st.groups.length; ++i)
        if (isDefaultSettings(st.groups[i])) return st.groups[i];
    return null;
}
// Groups holding identical settings, as sets of ids. Empty groups are excluded: a fresh one
// carries the defaults and would duplicate the def group by construction, and it dissolves
// as soon as the selection leaves it -- warning about that would be noise.
function duplicateSets(st) {
    var bySig = {}, order = [];
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        if (!g.slots.length) continue;
        var s = signature(g);
        if (!bySig[s]) { bySig[s] = []; order.push(s); }
        bySig[s].push(g.id);
    }
    var out = [];
    for (var k = 0; k < order.length; ++k)
        if (bySig[order[k]].length > 1) out.push(bySig[order[k]]);
    return out;
}

// Compact "0,2–5,7" rendering of a slot set. Digits and separators only, so it needs no
// translation.
function slotLabel(g) { return slotRunLabel(g ? g.slots : []); }
function slotRunLabel(s) {
    if (!s || !s.length) return "";
    var runs = [], start = s[0], prev = s[0];
    for (var i = 1; i <= s.length; ++i) {
        if (i < s.length && s[i] === prev + 1) { prev = s[i]; continue; }
        runs.push(start === prev ? String(start) : start + "–" + prev);
        if (i < s.length) { start = s[i]; prev = s[i]; }
    }
    return runs.join(",");
}
function payloadBytes(hex) {
    if (!hex) return 0;
    return Math.floor(String(hex).replace(/[^0-9a-fA-F]/g, "").length / 2);
}
function trigger(st, groupId, which) {
    var g = groupById(st, groupId);
    if (!g) return null;
    return which === "request" ? g.tr.request : g.ini.reply;
}
function sectionCount(t) { return (t && t.recv ? 1 : 0) + (t && t.send ? 1 : 0); }
function hasRewrite(t) {
    return !!t && (t.adv.cmdIdAction === "Replacement" || t.adv.addrAction === "Replacement"
                || t.adv.eventAction === "Same");
}
// The eventFilter this role writes: an initiator configures what happens when a RESPONSE
// comes back (2), a transponder what happens when a REQUEST arrives (1).
function roleEvent(role) { return role === "initiator" ? 2 : 1; }
function triggerFor(g, role) { return role === "initiator" ? g.ini.reply : g.tr.request; }

// ── mutators: state in, NEW state out ────────────────────────────────────────
function canAddGroup(st) { return st.groups.length < MAX_GROUPS; }

// A new group is born EMPTY, at the defaults, and selected -- there is no free slot to claim
// any more, because there are no free slots. You fill it by clicking slots in the bar, which
// is also the only way to move a slot anywhere. If it is still empty when the selection
// leaves it, it dissolves (see setActiveGroup): an empty group cannot survive a
// reconstruction, so persisting one would be a promise the model cannot keep.
//
// Refused at MAX_GROUPS -- enforced here, not only by hiding the button, so a stale binding
// or a repaired blob cannot get past it.
function addGroup(st, slots) {
    if (!canAddGroup(st)) return st;
    var n = clone(st);
    var g = newGroup(_next(n), slots || []);
    n.groups = n.groups.concat([g]);
    n.activeId = g.id;
    return n;
}

// "Remove" cannot mean delete: the slots need a home, and every slot must have exactly one.
// So it means RESET THESE SLOTS TO DEFAULTS -- they move into the def group, which is created
// if no group currently holds the defaults, and this group dissolves. Selection follows the
// slots, because that is where the operator is looking.
function removeGroup(st, id) {
    var n = clone(st), g = groupById(n, id);
    if (!g) return st;
    var moving = g.slots.slice();
    n.groups = n.groups.filter(function (x) { return x.id !== id; });

    var target = defaultGroupOf(n);
    if (moving.length && !target) {
        target = newGroup(_next(n), []);
        n.groups = n.groups.concat([target]);
    }
    if (target) {
        target.slots = target.slots.concat(moving).sort(function (a, b) { return a - b; });
        n.activeId = target.id;
    } else if (n.groups.length) {
        n.activeId = n.groups[0].id;
    }
    _repairPartition(n);
    _clampRefs(n);
    return n;
}

// Join groups holding identical settings into one, which is what the plan check's button
// does. The survivor is the earliest of them, so the tab an operator was reading keeps its
// place; the others hand over their slots and disappear. This writes nothing new -- the
// settings are equal by definition, which is why the device cannot tell the difference.
function joinGroups(st, ids) {
    if (!ids || ids.length < 2) return st;
    var n = clone(st), keep = null, moved = [];
    for (var i = 0; i < n.groups.length; ++i) {
        var g = n.groups[i];
        if (ids.indexOf(g.id) < 0) continue;
        if (!keep) { keep = g; continue; }
        moved = moved.concat(g.slots);
    }
    if (!keep || !moved.length) return st;
    keep.slots = keep.slots.concat(moved).sort(function (a, b) { return a - b; });
    n.groups = n.groups.filter(function (g) {
        return g.id === keep.id || ids.indexOf(g.id) < 0;
    });
    if (!groupById(n, n.activeId)) n.activeId = keep.id;
    _clampRefs(n);
    return n;
}

// Group membership rebuilt from the slot settings ALONE: one group per distinct settings
// value, holding every slot that carries it. This is the most that can be recovered when
// nothing stored the membership -- the reconstruction rule that gives the concept its
// definition. Idempotent, and a plan with no duplicates is already a fixed point.
//
// Used for a blob whose group identity cannot be trusted, and as the shape the join button
// converges on.
function reconstruct(st) {
    var n = clone(st), bySig = {}, order = [], groups = [];
    for (var i = 0; i < n.groups.length; ++i) {
        var g = n.groups[i];
        if (!g.slots.length) continue;
        var s = signature(g);
        if (!bySig[s]) { bySig[s] = g; order.push(s); groups.push(g); continue; }
        bySig[s].slots = bySig[s].slots.concat(g.slots);
    }
    for (var k = 0; k < groups.length; ++k)
        groups[k].slots.sort(function (a, b) { return a - b; });
    n.groups = groups;
    if (!groupById(n, n.activeId) && n.groups.length) n.activeId = n.groups[0].id;
    _repairPartition(n);
    _clampRefs(n);
    return n;
}
function addNode(st) {
    var n = clone(st);
    var used = n.nodes.map(function (x) { return x.addr; });
    var a = 1;
    while (used.indexOf(a) >= 0 && a < SLOT_COUNT) ++a;
    n.nodes = n.nodes.concat([{ id: _next(n), addr: a, active: true, refs: [] }]);
    return n;
}
function removeNode(st, id) {
    var n = clone(st);
    n.nodes = n.nodes.filter(function (x) { return x.id !== id; });
    return n;
}
function setNodeAddr(st, id, addr) {
    var n = clone(st), x = nodeById(n, id);
    if (!x) return st;
    x.addr = Math.max(0, Math.min(SLOT_COUNT, addr));
    return n;
}
function toggleNode(st, id) {
    var n = clone(st), x = nodeById(n, id);
    if (!x) return st;
    x.active = !x.active;
    return n;
}

// Adding N steps from one group walks that group's slots rather than repeating slots[0]:
// the point of a multi-slot group is one interrogation per slot. Once every slot is already
// referenced it repeats the first, deliberately.
function addStep(st, nodeId, groupId) {
    var n = clone(st), x = nodeById(n, nodeId), g = groupById(n, groupId);
    if (!x || !g || !g.slots.length) return st;
    var used = {};
    for (var i = 0; i < x.refs.length; ++i)
        if (x.refs[i].group === g.id) used[x.refs[i].cmd] = true;
    var cmd = g.slots[0];
    for (var j = 0; j < g.slots.length; ++j)
        if (!used[g.slots[j]]) { cmd = g.slots[j]; break; }
    x.refs = x.refs.concat([{ group: g.id, cmd: cmd, on: true }]);
    return n;
}
// Whether the schedule interrogates this step. A step is a plan item that keeps its position and
// its group binding; muting one used to mean DELETING it and adding it back, which lost both.
//
// It is deliberately not the same thing as the node's own switch: that stops the whole node.
function toggleStep(st, nodeId, index) {
    var n = clone(st), x = nodeById(n, nodeId);
    if (!x || !x.refs[index]) return st;
    x.refs[index].on = !x.refs[index].on;
    return n;
}
function removeStep(st, nodeId, index) {
    var n = clone(st), x = nodeById(n, nodeId);
    if (!x || index < 0 || index >= x.refs.length) return st;
    x.refs.splice(index, 1);
    return n;
}
function setStepCmd(st, nodeId, index, cmd) {
    var n = clone(st), x = nodeById(n, nodeId);
    if (!x || !x.refs[index]) return st;
    x.refs[index].cmd = cmd;
    return n;
}

// THE slot interaction, and now it has exactly one meaning: a click MOVES the slot into the
// selected group.
//
//   in another group      -> move it here. The group it left dissolves if that was its last
//                            slot, because a group with nothing to bulk-edit is nothing.
//   in the SELECTED group -> nothing. It is already here; saying so is the whole response.
//
// Moving used to be forbidden ("never steal") and it had to be: clicking a slot was also how
// you selected a group, so one click meant two things and a mistake cost you someone else's
// slot. The tab row selects now, which frees the bar to do the one job it is for -- and under
// a total partition, moving is the ONLY way a slot can change hands, so refusing it would
// make the bar decorative.
//
// Returns { state, action } with action in move | already | none.
function slotClick(st, cmd) {
    if (cmd < 0 || cmd >= SLOT_COUNT) return { state: st, action: "none" };
    var sel = activeGroupOf(st);
    if (!sel) return { state: st, action: "none" };
    if (hasSlot(sel, cmd)) return { state: st, action: "already" };

    var n = clone(st), target = groupById(n, sel.id), owner = ownerOf(n, cmd);
    if (owner) {
        owner.slots = owner.slots.filter(function (v) { return v !== cmd; });
        // Dissolve on empty -- never the target, which just gained a slot.
        if (!owner.slots.length)
            n.groups = n.groups.filter(function (g) { return g.id !== owner.id; });
    }
    target.slots = target.slots.concat([cmd]).sort(function (a, b) { return a - b; });
    _clampRefs(n);
    return { state: n, action: "move" };
}

// Every slot in exactly one group, at least one group, no duplicates and nothing out of
// range. Orphans -- which only a damaged or foreign blob can produce -- land in the def
// group, created if no group holds the defaults, because that is what an unconfigured slot
// is. Mutates in place; only ever called on a state that is already a private copy.
function _repairPartition(st) {
    var seen = {}, i, j;
    for (i = 0; i < st.groups.length; ++i) {
        var keep = [], raw = st.groups[i].slots || [];
        for (j = 0; j < raw.length; ++j) {
            var c = raw[j];
            if (typeof c !== "number" || c < 0 || c >= SLOT_COUNT || seen[c]) continue;
            seen[c] = true; keep.push(c);
        }
        st.groups[i].slots = keep.sort(function (a, b) { return a - b; });
    }
    var orphans = [];
    for (var cmd = 0; cmd < SLOT_COUNT; ++cmd) if (!seen[cmd]) orphans.push(cmd);

    if (orphans.length) {
        var target = defaultGroupOf(st);
        if (!target) {
            target = newGroup(_next(st), []);
            st.groups = st.groups.concat([target]);
        }
        target.slots = target.slots.concat(orphans).sort(function (a, b) { return a - b; });
    }
    if (!st.groups.length) {
        var all = [];
        for (var k = 0; k < SLOT_COUNT; ++k) all.push(k);
        st.groups = [newGroup(_next(st), all)];
    }
    if (!groupById(st, st.activeId)) st.activeId = st.groups[0].id;
}

// A scheduled step names a slot; which group that is follows from who owns the slot, so it is
// recomputed rather than stored twice. Under a total partition the owner always exists, which
// is why this can no longer drop a step: a slot cannot go missing, only change hands.
function _clampRefs(st) {
    for (var i = 0; i < st.nodes.length; ++i) {
        st.nodes[i].refs = st.nodes[i].refs.filter(function (r) {
            if (typeof r.cmd !== "number" || r.cmd < 0 || r.cmd >= SLOT_COUNT) return false;
            var o = ownerOf(st, r.cmd);
            if (!o) return false;
            r.group = o.id;
            // Absent means scheduled. Every blob written before the flag existed holds steps
            // that WERE being interrogated, so defaulting to false would silently stop a
            // working schedule on upgrade.
            r.on = (r.on === undefined) ? true : !!r.on;
            return true;
        });
    }
}

function setSendField(st, groupId, field, value) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g || !g.ini.send) return st;
    g.ini.send[field] = value;
    return n;
}
function attachSend(st, groupId) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g) return st;
    g.ini.send = newSend();
    return n;
}
function detachSend(st, groupId) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g) return st;
    g.ini.send = null;
    for (var k = 0; k < n.nodes.length; ++k)
        n.nodes[k].refs = n.nodes[k].refs.filter(function (r) { return r.group !== groupId; });
    return n;
}
function attachTrigger(st, groupId, which) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g) return st;
    if (which === "request") g.tr.request = newTrigger();
    else g.ini.reply = newTrigger();
    return n;
}
function detachTrigger(st, groupId, which) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g) return st;
    if (which === "request") g.tr.request = null;
    else g.ini.reply = null;
    return n;
}
function attachSection(st, groupId, which, side) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    if (side === "recv") t.recv = { fmt: "bits", bits: 0 };
    else t.send = { fmt: "bits", payload: "" };
    return n;
}
function detachSection(st, groupId, which, side) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    if (side === "recv") t.recv = null;
    else t.send = null;
    return n;
}
function setSectionField(st, groupId, which, side, field, value) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    var s = side === "recv" ? t.recv : t.send;
    if (!s) return st;
    s[field] = value;
    return n;
}
function setAdvOpen(st, groupId, which, open) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    t.advOpen = open;
    if (!open) t.adv = { eventAction: "Swap", cmdIdAction: "Incoming", cmdIdRepl: 0,
                         addrAction: "Incoming", addrRepl: 0 };
    return n;
}
function setAdvField(st, groupId, which, field, value) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    t.adv[field] = value;
    return n;
}
// Selection by id, and the one place an empty group dies. A group added but never filled has
// nothing to bulk-edit and could not be reconstructed, so leaving it behind would put a group
// in the tab row that a reload would not produce. Selecting away is exactly the moment the
// operator stopped filling it.
function setActiveGroup(st, id) {
    var target = groupById(st, id);
    if (!target) return st;
    if (target.id === st.activeId) return st;
    var n = clone(st), prev = groupById(n, n.activeId);
    n.activeId = target.id;
    if (prev && prev.id !== target.id && !prev.slots.length)
        n.groups = n.groups.filter(function (g) { return g.id !== prev.id; });
    return n;
}

// ── derived views ────────────────────────────────────────────────────────────
// Every slot is owned, so `groupId` and `index` are always real. They stay in the shape
// anyway: the bar reads them per cell and should not have to know that.
function coverage(st) {
    var out = [];
    for (var c = 0; c < SLOT_COUNT; ++c) {
        var o = ownerOf(st, c);
        out.push({ cmd: c, groupId: o ? o.id : -1, index: o ? groupIndexById(st, o.id) : -1,
                   isDefault: isDefaultSettings(o) });
    }
    return out;
}
// `isDefault` is what makes a tab render as `def` instead of `Gn`. It is a property of the
// settings, so it moves from group to group as they are edited, and can be true of none of
// them or -- until the plan check's join is taken -- of several.
function groupsView(st) {
    var out = [];
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        out.push({ id: g.id, index: i, label: slotLabel(g), count: g.slots.length,
                   slots: g.slots.slice(), schedulable: schedulable(g),
                   isDefault: isDefaultSettings(g) });
    }
    return out;
}
function nodesView(st) {
    var out = [];
    for (var i = 0; i < st.nodes.length; ++i) {
        var x = st.nodes[i], steps = [];
        for (var j = 0; j < x.refs.length; ++j)
            steps.push({ group: x.refs[j].group, cmd: x.refs[j].cmd, index: j,
                         on: x.refs[j].on !== false,
                         groupIndex: groupIndexById(st, x.refs[j].group) });
        out.push({ id: x.id, addr: x.addr, active: x.active,
                   cmdCount: x.refs.length, steps: steps });
    }
    return out;
}
function activeNodeCount(st) {
    var n = 0;
    for (var i = 0; i < st.nodes.length; ++i) if (st.nodes[i].active) ++n;
    return n;
}
function activeSlotCount(st) {
    var g = activeGroupOf(st);
    return g ? g.slots.length : 0;
}
// One entry per interrogation the host will perform, in order. A node with no commands is
// interrogated once with cmd 0.
function schedule(st) {
    var out = [];
    for (var i = 0; i < st.nodes.length; ++i) {
        var n = st.nodes[i];
        if (!n.active) continue;
        if (!n.refs.length) {
            out.push({ addr: n.addr, cmd: 0, implicit: true, nodeId: n.id, groupId: -1 });
            continue;
        }
        // A node whose every step is muted contributes NOTHING -- not even the implicit cmd 0
        // that a node with no steps at all gets. Falling back to it would make muting the last
        // step silently start interrogating something else.
        for (var j = 0; j < n.refs.length; ++j) {
            if (n.refs[j].on === false) continue;
            out.push({ addr: n.addr, cmd: n.refs[j].cmd, implicit: false,
                       nodeId: n.id, groupId: n.refs[j].group });
        }
    }
    return out;
}
// Payload FORMAT -> the Function value on the wire. USBLPingRequest::Function and
// USBLCmdConfig::Function agree (0 default / 1 bit array / 2 lat-lon-azimuth), which is
// the only reason one table serves both. Absence is 0, never a menu item.
function formatWire(id) { return id === "bits" ? 1 : (id === "llgeo" ? 2 : 0); }

// The exact ID_USBL_CONTROL v6 writes Apply will emit, in cmd order.
//
// APPLY IS TOTAL: one frame for every one of the eight slots, every time. A slot a group
// configures gets that configuration; every other slot gets USBLCmdConfig's own defaults.
//
// That is what makes Apply mean "the device's slot table now matches this pane" rather
// than "some slots were poked". Three things fall out of it:
//
//   - An empty plan is a meaningful thing to apply: it resets the side to defaults.
//   - Releasing a slot needs no bookkeeping. It reverts on the next Apply because it is
//     written every time, so `written` / staleWrites / releaseFrames are all gone -- and
//     with them the class of bug where the device held a configuration the pane had
//     forgotten. ID_USBL_CONTROL has no read-back, so "write everything" is the only way
//     to actually know what the device holds.
//   - The frame count is fixed: 8, plus the transponder's two globals.
//
// The cost is that Apply overwrites slots configured outside this pane. The pane presents
// all eight as its own, so that is the intended reading.
//
// Pure, so `node` asserts every byte with no device, no window and no click -- which is
// the only reason Apply is verified at all.
function applyWrites(st, role) {
    var ev = roleEvent(role), out = [], byCmd = {}, i, j;

    for (i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i], t = triggerFor(g, role);
        if (!t) continue;
        for (j = 0; j < g.slots.length; ++j) byCmd[g.slots[j]] = t;
    }

    for (var cmd = 0; cmd < SLOT_COUNT; ++cmd) {
        var e = { cmd: cmd, event: ev, configured: false,
                  recvFn: 0, recvBits: 0, sendFn: 0, sendHex: "",
                  eventAction: 0, cmdIdAction: 0, cmdIdRepl: 0,
                  addrAction: 0, addrRepl: 0 };
        var h = byCmd[cmd];
        if (h) {
            e.configured = true;
            e.recvFn = h.recv ? formatWire(h.recv.fmt) : 0;
            e.recvBits = h.recv ? h.recv.bits : 0;
            e.sendFn = h.send ? formatWire(h.send.fmt) : 0;
            e.sendHex = h.send ? h.send.payload : "";
            e.eventAction = h.adv.eventAction === "Same" ? 1 : 0;
            e.cmdIdAction = h.adv.cmdIdAction === "Replacement" ? 1 : 0;
            e.cmdIdRepl = h.adv.cmdIdRepl;
            e.addrAction = h.adv.addrAction === "Replacement" ? 1 : 0;
            e.addrRepl = h.adv.addrRepl;
        }
        out.push(e);
    }
    return out;
}

// Derived from applyWrites(), never counted separately, so the number on the button
// cannot disagree with what the button sends.
function applyFrames(st, role) {
    return applyWrites(st, role).length + (role === "transponder" ? 2 : 0);
}
// How many of those eight carry a configuration rather than defaults. Only ever a label.
function configuredSlots(st, role) {
    var n = 0, w = applyWrites(st, role);
    for (var i = 0; i < w.length; ++i) if (w[i].configured) ++n;
    return n;
}

// Staleness is measured in WHAT THE DEVICE WOULD RECEIVE, not in how the pane is organised:
// the snapshot is the role's eight per-slot writes. Joining duplicate groups, dissolving an
// emptied one, or renumbering therefore does not read as a change -- which is what lets the
// plan check offer a join without provoking a pointless re-apply. Only the half this role
// writes counts.
function snapshot(st, role) { return JSON.stringify(applyWrites(st, role)); }
// The pre-partition scheme, kept for ONE purpose: deciding whether a v5 blob's recorded
// snapshot still describes the plan being loaded (see _migrateApplied). Never write it.
function _legacySnapshot(st, role) {
    return JSON.stringify(st.groups.map(function (g) {
        return [g.slots, triggerFor(g, role)];
    }));
}
function appliedOnce(st, role) { return typeof st.applied[role] === "string"; }
function isStale(st, role) {
    return appliedOnce(st, role) && st.applied[role] !== snapshot(st, role);
}
// Any role applied, then edited. Drives the header badge while the pane is collapsed.
function anyStale(st) {
    for (var i = 0; i < ROLES.length; ++i) if (isStale(st, ROLES[i])) return true;
    return false;
}
function markApplied(st, role) {
    var n = clone(st);
    n.applied[role] = snapshot(n, role);
    return n;
}

// ── derived roles (never configured, always displayed) ────────────────────────
function subroleInitiatorCode(g) {
    if (!g || !g.ini.send) return "notInitiator";
    return g.ini.send.reply > 0 ? "interrogator" : "pinger";
}
function subroleTransponderCode(g) {
    var t = g ? g.tr.request : null;
    if (!t) return "defaultTransponder";
    if (t.recv && t.send) return "relay";
    if (t.send) return "source";
    if (t.recv) return "sink";
    return "transponder";
}

// ── consistency check ────────────────────────────────────────────────────────
// Returns CODES and numbers; the QML layer turns them into translated sentences. Keeping
// prose out of here is what lets `node` test the rules.
//
// An entry may carry an ACTION -- a fix the UI can offer as a button, with the ids it applies
// to. `duplicateGroups` is the first: groups holding identical settings are legal (nothing is
// wrong on the wire, they write the same bytes to their own slots), so this is an offer to
// tidy, not a defect. Its severity says so.
//
// Slots at the defaults are not reported at all: they are a group like any other now, and
// there is no such thing as an unowned slot to warn about.
function issueCodes(st) {
    var out = [], dups = duplicateSets(st);
    for (var d = 0; d < dups.length; ++d) {
        var ids = dups[d], idx = [];
        for (var q = 0; q < ids.length; ++q) idx.push(groupIndexById(st, ids[q]));
        out.push({ code: "duplicateGroups", sev: "info", groups: idx, ids: ids,
                   action: "join", key: slotLabel(groupById(st, ids[0])) });
    }
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i], gi = i, key = slotLabel(g);
        if (!g.slots.length)
            out.push({ code: "groupOwnsNoSlots", sev: "warn", group: gi, key: key });

        var snd = g.ini.send, rq = g.tr.request, rs = g.ini.reply;
        var outBits = (snd && snd.fn === "bits") ? payloadBytes(snd.payload) * 8 : 0;

        if (outBits > 0 && !(rq && rq.recv))
            out.push({ code: "requestCarriesButNoReceiver", sev: "warn",
                       group: gi, key: key, bits: outBits });
        if (outBits > 0 && rq && rq.recv && rq.recv.fmt === "bits" && rq.recv.bits !== outBits)
            out.push({ code: "requestBitsMismatch", sev: "crit",
                       group: gi, key: key, bits: outBits, expected: rq.recv.bits });

        if (rq && rq.send && rq.send.fmt === "bits") {
            var back = payloadBytes(rq.send.payload) * 8;
            if (!(rs && rs.recv))
                out.push({ code: "answerButNoReplyHandler", sev: "warn",
                           group: gi, key: key, bits: back });
            else if (rs.recv.fmt === "bits" && rs.recv.bits !== back)
                out.push({ code: "answerBitsMismatch", sev: "crit",
                           group: gi, key: key, bits: back, expected: rs.recv.bits });
        }

        if (snd && snd.reply === 0 && rq && sectionCount(rq) > 0)
            out.push({ code: "pingerVsAnswering", sev: "warn", group: gi, key: key });
        if (rs && !snd)
            out.push({ code: "replyHandlerWithoutRequest", sev: "warn", group: gi, key: key });
    }
    return out;
}

// ── persistence payload ──────────────────────────────────────────────────────
function serialize(st) {
    return JSON.stringify({ v: 7, groups: st.groups, nodes: st.nodes,
                            nextId: st.nextId, activeId: st.activeId,
                            applied: st.applied });
}
// Tolerant by design: a malformed or older blob yields a usable plan rather than a broken one.
// The partition is repaired, not trusted -- orphan slots, double ownership and empty groups
// are all things a v5 blob can legitimately contain, because v5 allowed them.
function deserialize(raw) {
    var st = initialState();
    if (!raw || !String(raw).length) return st;
    var o;
    try { o = JSON.parse(raw); } catch (e) { return st; }
    if (!o || typeof o !== "object") return st;
    if (Array.isArray(o.groups)) st.groups = o.groups;
    if (Array.isArray(o.nodes)) st.nodes = o.nodes;
    if (typeof o.nextId === "number") st.nextId = o.nextId;

    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        if (!g || typeof g !== "object") { st.groups.splice(i--, 1); continue; }
        if (!g.ini) g.ini = { send: newSend(), reply: null };
        if (!g.tr) g.tr = { request: null };
        if (typeof g.id !== "number") g.id = _next(st);
        if (!Array.isArray(g.slots)) g.slots = [];
    }
    // An empty group is transient by design (setActiveGroup drops it), so a reload -- which
    // has no "still filling this one" context -- keeps none of them.
    st.groups = st.groups.filter(function (x) { return x.slots.length > 0; });
    if (st.groups.length > MAX_GROUPS) st.groups = st.groups.slice(0, MAX_GROUPS);

    // Selection: v6 stores an id, v5 stored an index into `groups`. Translate rather than
    // discard, or every reload of a v5 plan would open the wrong group.
    if (typeof o.activeId === "number") st.activeId = o.activeId;
    else if (typeof o.activeGroup === "number" && st.groups[o.activeGroup])
        st.activeId = st.groups[o.activeGroup].id;

    for (var gi = 0; gi < st.groups.length; ++gi) {
        _repairTrigger(st.groups[gi].ini.reply);
        _repairTrigger(st.groups[gi].tr.request);
    }
    for (var k = 0; k < st.nodes.length; ++k) {
        if (!st.nodes[k] || typeof st.nodes[k] !== "object") { st.nodes.splice(k--, 1); continue; }
        if (!Array.isArray(st.nodes[k].refs)) st.nodes[k].refs = [];
        if (typeof st.nodes[k].addr !== "number") st.nodes[k].addr = 0;
        st.nodes[k].active = !!st.nodes[k].active;
    }
    // Measured BEFORE the partition repair, because that is the shape the old snapshot
    // described. Repairing adds a group to own slots the blob never mentioned, which changes
    // the old-format string without changing a single byte Apply would send -- compare after
    // it and every migrated plan would falsely read as stale.
    var legacyBefore = {};
    for (var r = 0; r < ROLES.length; ++r) legacyBefore[ROLES[r]] = _legacySnapshot(st, ROLES[r]);

    _repairPartition(st);
    _clampRefs(st);
    _migrateApplied(st, o, legacyBefore);
    return st;
}

// What was applied, across the change of snapshot format.
//
// v<=4 carried a single current role with one snapshot; v5 carried one per role, in the
// per-group format `_legacySnapshot` still computes. A v5 string can never equal a v6 one, so
// carrying it over verbatim would make every previously-applied plan claim "changed since
// applied" on first load -- a lie, and one that costs an operator a needless write.
//
// It is decidable, though: if the blob's own LEGACY snapshot matches what was recorded,
// nothing has changed since that Apply, so the same plan can be re-recorded in the new format.
// If it does not match, the plan really did change and the stale reading is correct -- keep
// the old string, which cannot match, and let it read as stale.
function _migrateApplied(st, o, legacyBefore) {
    var raw = {};
    if (o.applied && typeof o.applied === "object") raw = o.applied;
    else if (o.appliedOnce && typeof o.appliedSnapshot === "string"
             && ROLES.indexOf(o.role) >= 0) raw[o.role] = o.appliedSnapshot;

    for (var r = 0; r < ROLES.length; ++r) {
        var role = ROLES[r], was = raw[role];
        if (typeof was !== "string") continue;
        if (typeof o.v === "number" && o.v >= 6) { st.applied[role] = was; continue; }
        st.applied[role] = (was === legacyBefore[role]) ? snapshot(st, role) : was;
    }
}

// A blob written by an older build may lack `adv`, or carry the wrong type in a section.
function _repairTrigger(t) {
    if (!t) return;
    if (!t.adv) t.adv = { eventAction: "Swap", cmdIdAction: "Incoming", cmdIdRepl: 0,
                          addrAction: "Incoming", addrRepl: 0 };
    if (t.recv && typeof t.recv.bits !== "number") t.recv.bits = 0;
    if (t.send && typeof t.send.payload !== "string") t.send.payload = "";
    // v3 blobs carry a slot disposition. USBLCmdSlotConfig no longer exists, so drop it
    // rather than let a dead field ride along in every later save.
    if ("disposition" in t) delete t.disposition;
    t.advOpen = !!t.advOpen;
}

// The seeded plan a first-run operator sees: one baseline group answering on the spare
// slots, one data group carrying a payload both ways, and two nodes. Between them they hold
// all eight slots -- the partition invariant applies to the seed like anything else.
function defaults() {
    var st = initialState();
    var baseline = newGroup(_next(st), [0, 4, 5, 6, 7]);
    baseline.tr.request = newTrigger();
    var data = newGroup(_next(st), [1, 2, 3]);
    data.ini.send.fn = "bits";
    data.ini.send.payload = "0A 1B";
    data.tr.request = newTrigger();
    data.tr.request.recv = { fmt: "bits", bits: 16 };
    data.tr.request.send = { fmt: "bits", payload: "FF 01" };
    data.ini.reply = newTrigger();
    data.ini.reply.recv = { fmt: "bits", bits: 16 };
    // Replacing `groups` orphans the initial group's id, so say which group is selected
    // rather than leaving activeGroupOf to fall back to the first one.
    st.groups = [baseline, data];
    st.activeId = baseline.id;
    st.nodes = [
        { id: _next(st), addr: 1, active: true,
          refs: [{ group: data.id, cmd: 1, on: true },
                 { group: data.id, cmd: 2, on: true },
                 { group: data.id, cmd: 3, on: true }] },
        { id: _next(st), addr: 2, active: true, refs: [] }
    ];
    return st;
}

// Node consumes this via module.exports; QML sees the top-level functions directly and
// ignores the guard.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        SLOT_COUNT: SLOT_COUNT, MAX_GROUPS: MAX_GROUPS, ROLES: ROLES,
        canAddGroup: canAddGroup,
        initialState: initialState, clone: clone,
        newSend: newSend, newTrigger: newTrigger, newGroup: newGroup,
        settingsOf: settingsOf, signature: signature,
        isDefaultSettings: isDefaultSettings, defaultGroupOf: defaultGroupOf,
        duplicateSets: duplicateSets, joinGroups: joinGroups, reconstruct: reconstruct,
        groupById: groupById, groupIndexById: groupIndexById, nodeById: nodeById,
        hasSlot: hasSlot, ownerOf: ownerOf, schedulable: schedulable,
        activeGroupOf: activeGroupOf,
        slotLabel: slotLabel, slotRunLabel: slotRunLabel, payloadBytes: payloadBytes,
        trigger: trigger, sectionCount: sectionCount, hasRewrite: hasRewrite,
        roleEvent: roleEvent, triggerFor: triggerFor,
        addGroup: addGroup, removeGroup: removeGroup, addNode: addNode,
        removeNode: removeNode, setNodeAddr: setNodeAddr, toggleNode: toggleNode,
        addStep: addStep, removeStep: removeStep, toggleStep: toggleStep, setStepCmd: setStepCmd,
        slotClick: slotClick,
        setSendField: setSendField, attachSend: attachSend, detachSend: detachSend,
        attachTrigger: attachTrigger, detachTrigger: detachTrigger,
        attachSection: attachSection,
        detachSection: detachSection, setSectionField: setSectionField,
        setAdvOpen: setAdvOpen, setAdvField: setAdvField,
        setActiveGroup: setActiveGroup,
        coverage: coverage, groupsView: groupsView, nodesView: nodesView,
        activeSlotCount: activeSlotCount, activeNodeCount: activeNodeCount,
        schedule: schedule, defaults: defaults,
        applyFrames: applyFrames, configuredSlots: configuredSlots,
        formatWire: formatWire,
        applyWrites: applyWrites, snapshot: snapshot,
        appliedOnce: appliedOnce, isStale: isStale, anyStale: anyStale,
        markApplied: markApplied,
        subroleInitiatorCode: subroleInitiatorCode,
        subroleTransponderCode: subroleTransponderCode,
        issueCodes: issueCodes, serialize: serialize, deserialize: deserialize
    };
}

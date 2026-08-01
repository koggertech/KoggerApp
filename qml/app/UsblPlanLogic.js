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

var SLOT_COUNT = 8;
// One group per slot is the most that can ever be useful: groups partition the slots, so
// a ninth group is guaranteed to own nothing.
var MAX_GROUPS = SLOT_COUNT;
var ROLES = ["initiator", "transponder"];

// ── state ────────────────────────────────────────────────────────────────────
// `applied` maps a role to the snapshot that was last written as that role. There is no
// current role: a role is an argument to Apply, not a mode the plan sits in.
function emptyState() {
    return { groups: [], nodes: [], activeGroup: 0, nextId: 1, applied: {} };
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
function activeGroupOf(st) {
    if (!st.groups.length) return null;
    return st.groups[Math.min(st.activeGroup, st.groups.length - 1)];
}

// Compact "0,2–5,7" rendering of a slot set. Digits and separators only, so it needs no
// translation.
function slotLabel(g) {
    if (!g || !g.slots.length) return "";
    var s = g.slots, runs = [], start = s[0], prev = s[0];
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

// With no argument the new group claims the first free slot, so a group is never born
// unable to do anything. `slots` overrides that when the caller knows which slot it wants.
// Refused at MAX_GROUPS -- enforced here, not only by hiding the button, so a stale
// binding or a repaired blob cannot get past it.
function addGroup(st, slots) {
    if (!canAddGroup(st)) return st;
    var n = clone(st), want = slots;
    if (!want) {
        var free = [];
        for (var c = 0; c < SLOT_COUNT; ++c)
            if (!ownerOf(n, c)) free.push(c);
        want = free.length ? [free[0]] : [];
    }
    n.groups = n.groups.concat([newGroup(_next(n), want)]);
    n.activeGroup = n.groups.length - 1;
    return n;
}
function removeGroup(st, id) {
    var n = clone(st), i = groupIndexById(n, id);
    if (i < 0) return st;
    for (var k = 0; k < n.nodes.length; ++k)
        n.nodes[k].refs = n.nodes[k].refs.filter(function (r) { return r.group !== id; });
    n.groups.splice(i, 1);
    if (n.activeGroup >= n.groups.length) n.activeGroup = Math.max(0, n.groups.length - 1);
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
    x.refs = x.refs.concat([{ group: g.id, cmd: cmd }]);
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

// The single slot interaction. One bar, and what a click means follows from the cell's
// relationship to the selected group -- no modifier, no mode:
//
//   owned by ANOTHER group  -> select that group. Never steals: a transfer that happens
//                              behind your back is the thing that made the old bar
//                              unusable in practice.
//   owned by the SELECTED   -> release it. The holder is the only thing that can let go.
//   unowned                 -> assign it to the selected group, creating one if the plan
//                              is empty so the first click on an empty plan still works.
//
// Moving a slot is therefore two explicit clicks, and both are visible in the bar.
// Returns { state, action } with action in select | release | assign | create | none.
function slotClick(st, cmd) {
    if (cmd < 0 || cmd >= SLOT_COUNT) return { state: st, action: "none" };
    var owner = ownerOf(st, cmd);
    var sel = activeGroupOf(st);

    if (owner) {
        if (!sel || owner.id !== sel.id) {
            var i = groupIndexById(st, owner.id);
            if (i === st.activeGroup) return { state: st, action: "select" };
            return { state: setActiveGroup(st, i), action: "select" };
        }
        var n = clone(st), g = groupById(n, owner.id);
        g.slots = g.slots.filter(function (v) { return v !== cmd; });
        _clampRefs(n);
        return { state: n, action: "release" };
    }

    if (!sel) {
        var made = addGroup(st, [cmd]);
        return made === st ? { state: st, action: "none" }
                           : { state: made, action: "create" };
    }
    var m = clone(st), t = groupById(m, sel.id);
    t.slots = t.slots.concat([cmd]).sort(function (a, b) { return a - b; });
    return { state: m, action: "assign" };
}

// A scheduled step must name a slot its group still owns; otherwise fall back to the
// group's first remaining slot, or drop the step if the group owns none. Mutates in place --
// only ever called on a state that is already a private copy.
function _clampRefs(st) {
    for (var i = 0; i < st.nodes.length; ++i) {
        st.nodes[i].refs = st.nodes[i].refs.filter(function (r) {
            var g = groupById(st, r.group);
            if (!g || !g.slots.length) return false;
            if (!hasSlot(g, r.cmd)) r.cmd = g.slots[0];
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
function setActiveGroup(st, i) {
    var n = clone(st);
    n.activeGroup = Math.max(0, Math.min(i, Math.max(0, n.groups.length - 1)));
    return n;
}

// ── derived views ────────────────────────────────────────────────────────────
function coverage(st) {
    var out = [];
    for (var c = 0; c < SLOT_COUNT; ++c) {
        var o = ownerOf(st, c);
        out.push({ cmd: c, groupId: o ? o.id : -1, index: o ? groupIndexById(st, o.id) : -1 });
    }
    return out;
}
function groupsView(st) {
    var out = [];
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        out.push({ id: g.id, index: i, label: slotLabel(g), count: g.slots.length,
                   slots: g.slots.slice(), schedulable: schedulable(g) });
    }
    return out;
}
function nodesView(st) {
    var out = [];
    for (var i = 0; i < st.nodes.length; ++i) {
        var x = st.nodes[i], steps = [];
        for (var j = 0; j < x.refs.length; ++j)
            steps.push({ group: x.refs[j].group, cmd: x.refs[j].cmd, index: j,
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
        for (var j = 0; j < n.refs.length; ++j)
            out.push({ addr: n.addr, cmd: n.refs[j].cmd, implicit: false,
                       nodeId: n.id, groupId: n.refs[j].group });
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

// Only the half this role writes counts toward staleness.
function snapshot(st, role) {
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
function issueCodes(st) {
    var out = [], gaps = [];
    for (var c = 0; c < SLOT_COUNT; ++c)
        if (!ownerOf(st, c)) gaps.push(c);
    if (gaps.length)
        out.push({ code: "unownedSlots", sev: "warn", slots: gaps });

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
    return JSON.stringify({ v: 5, groups: st.groups, nodes: st.nodes,
                            nextId: st.nextId, activeGroup: st.activeGroup,
                            applied: st.applied });
}
// Tolerant by design: a malformed or older blob yields defaults rather than a broken plan.
function deserialize(raw) {
    var st = emptyState();
    if (!raw || !String(raw).length) return st;
    var o;
    try { o = JSON.parse(raw); } catch (e) { return st; }
    if (!o || typeof o !== "object") return st;
    if (Array.isArray(o.groups)) st.groups = o.groups;
    if (Array.isArray(o.nodes)) st.nodes = o.nodes;
    if (typeof o.nextId === "number") st.nextId = o.nextId;
    if (typeof o.activeGroup === "number") st.activeGroup = o.activeGroup;
    // v<=4 carried a single current role with one snapshot. Migrate it onto that role and
    // leave the other unapplied, which is the truth: the other half was never written.
    if (o.applied && typeof o.applied === "object") {
        for (var r = 0; r < ROLES.length; ++r)
            if (typeof o.applied[ROLES[r]] === "string")
                st.applied[ROLES[r]] = o.applied[ROLES[r]];
    } else if (o.appliedOnce && typeof o.appliedSnapshot === "string"
               && ROLES.indexOf(o.role) >= 0) {
        st.applied[o.role] = o.appliedSnapshot;
    }

    // Repair anything the blob got wrong rather than trusting it: slots must be unique,
    // in range and owned by exactly one group, and every step must name a slot its group
    // still owns. A blob from a build without the cap is trimmed to it -- the extras
    // could only be groups owning nothing.
    if (st.groups.length > MAX_GROUPS) st.groups = st.groups.slice(0, MAX_GROUPS);
    var seen = {};
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        if (!g || typeof g !== "object") { st.groups.splice(i--, 1); continue; }
        if (!g.ini) g.ini = { send: newSend(), reply: null };
        if (!g.tr) g.tr = { request: null };
        var keep = [];
        var raws = Array.isArray(g.slots) ? g.slots : [];
        for (var j = 0; j < raws.length; ++j) {
            var c = raws[j];
            if (typeof c !== "number" || c < 0 || c >= SLOT_COUNT || seen[c]) continue;
            seen[c] = true; keep.push(c);
        }
        g.slots = keep.sort(function (a, b) { return a - b; });
    }
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
    _clampRefs(st);
    if (st.activeGroup >= st.groups.length) st.activeGroup = Math.max(0, st.groups.length - 1);
    return st;
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
// slots, one data group carrying a payload both ways, and two nodes.
function defaults() {
    var st = emptyState();
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
    st.groups = [baseline, data];
    st.nodes = [
        { id: _next(st), addr: 1, active: true,
          refs: [{ group: data.id, cmd: 1 }, { group: data.id, cmd: 2 },
                 { group: data.id, cmd: 3 }] },
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
        emptyState: emptyState, clone: clone,
        newSend: newSend, newTrigger: newTrigger, newGroup: newGroup,
        groupById: groupById, groupIndexById: groupIndexById, nodeById: nodeById,
        hasSlot: hasSlot, ownerOf: ownerOf, schedulable: schedulable,
        activeGroupOf: activeGroupOf, slotLabel: slotLabel, payloadBytes: payloadBytes,
        trigger: trigger, sectionCount: sectionCount, hasRewrite: hasRewrite,
        roleEvent: roleEvent, triggerFor: triggerFor,
        addGroup: addGroup, removeGroup: removeGroup, addNode: addNode,
        removeNode: removeNode, setNodeAddr: setNodeAddr, toggleNode: toggleNode,
        addStep: addStep, removeStep: removeStep, setStepCmd: setStepCmd,
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

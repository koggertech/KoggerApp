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

// ── state ────────────────────────────────────────────────────────────────────
function emptyState() {
    return { groups: [], nodes: [], role: "initiator", activeGroup: 0,
             appliedOnce: false, appliedSnapshot: "", nextId: 1, written: {} };
}

// Deep copy per edit. A plan is at most 8 slots and 8 nodes, so the cost is irrelevant and
// the correctness is total: nothing can accidentally alias the previous state.
function clone(st) { return JSON.parse(JSON.stringify(st)); }

function _next(st) { st.nextId = st.nextId + 1; return st.nextId; }

function newSend()    { return { fn: "default", payload: "", reply: 20000 }; }
function newTrigger() {
    return { disposition: "ack", recv: null, send: null, advOpen: false,
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
// Two attached sections need USBLCmdConfig -- the only struct carrying both a
// receiver_function and a sender_function. Rewrite rules force it too.
function structOf(t) {
    if (!t) return "";
    return (sectionCount(t) === 2 || hasRewrite(t)) ? "USBLCmdConfig" : "USBLCmdSlotConfig";
}
function roleEvent(st) { return st.role === "initiator" ? 2 : 1; }

// ── mutators: state in, NEW state out ────────────────────────────────────────
function addGroup(st) {
    var n = clone(st), free = [];
    for (var c = 0; c < SLOT_COUNT; ++c)
        if (!ownerOf(n, c)) free.push(c);
    n.groups = n.groups.concat([newGroup(_next(n), free.length ? [free[0]] : [])]);
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

// Exclusive assignment: taking a slot removes it from whichever group held it, so the eight
// hardware slots stay partitioned and no write-order question can arise.
// Returns { state, takenFrom } -- takenFrom is the previous owner's id, or -1.
function toggleSlot(st, groupId, cmd) {
    var n = clone(st), g = groupById(n, groupId);
    if (!g) return { state: st, takenFrom: -1 };
    var takenFrom = -1;
    if (hasSlot(g, cmd)) {
        g.slots = g.slots.filter(function (v) { return v !== cmd; });
    } else {
        var prev = ownerOf(n, cmd);
        if (prev) { takenFrom = prev.id; prev.slots = prev.slots.filter(function (v) { return v !== cmd; }); }
        g.slots = g.slots.concat([cmd]).sort(function (a, b) { return a - b; });
    }
    _clampRefs(n);
    return { state: n, takenFrom: takenFrom };
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
function setDisposition(st, groupId, which, id) {
    var n = clone(st), t = trigger(n, groupId, which);
    if (!t) return st;
    t.disposition = id;
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
// Switching role invalidates "applied": the two roles write different halves.
function setRole(st, r) {
    var n = clone(st);
    n.role = r; n.appliedOnce = false; n.appliedSnapshot = "";
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
// cmd_id has no wildcard, so a group of N slots costs N frames.
function setupFrames(st) {
    var n = 0;
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        if (st.role === "initiator") { if (g.ini.reply) n += g.slots.length; }
        else if (g.tr.request) n += g.slots.length;
    }
    if (st.role !== "initiator") n += 2;   // address filter + transponder enable
    return n;
}
function runFrames(st) { return st.role === "initiator" ? schedule(st).length : 0; }

function plannedWrites(st) {
    var out = {}, ev = roleEvent(st);
    for (var i = 0; i < st.groups.length; ++i) {
        var g = st.groups[i];
        var t = st.role === "initiator" ? g.ini.reply : g.tr.request;
        if (!t) continue;
        for (var j = 0; j < g.slots.length; ++j) out[g.slots[j] + ":" + ev] = true;
    }
    return out;
}
// Slots this role wrote before and no longer configures. ID_USBL_CONTROL has no read-back,
// so detaching a handler host-side leaves the device holding it -- these must be explicitly
// switched off. Scoped to this role's own event so one role cannot tear down the other's.
function staleWrites(st) {
    var ev = roleEvent(st), planned = plannedWrites(st), out = [];
    for (var k in st.written) {
        var parts = String(k).split(":");
        if (parseInt(parts[1], 10) !== ev) continue;
        if (!planned[k]) out.push({ cmd: parseInt(parts[0], 10), event: ev });
    }
    return out;
}
function releaseFrames(st) { return staleWrites(st).length; }

// Only the half this role writes counts toward staleness.
function snapshot(st) {
    return JSON.stringify(st.groups.map(function (g) {
        return [g.slots, st.role === "initiator" ? g.ini.reply : g.tr.request];
    }));
}
function isStale(st) { return st.appliedOnce && st.appliedSnapshot !== snapshot(st); }

// Records the write set in the same step as marking applied, so a caller cannot mark the
// plan applied while leaving the device's slot table untracked.
function markApplied(st) {
    var n = clone(st), ev = roleEvent(n), planned = plannedWrites(n), next = {};
    for (var k in n.written)                       // other event belongs to the other role
        if (parseInt(String(k).split(":")[1], 10) !== ev) next[k] = true;
    for (var p in planned) next[p] = true;
    n.written = next;
    n.appliedSnapshot = snapshot(n);
    n.appliedOnce = true;
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
    if (t.disposition === "off" && sectionCount(t) === 0) return "disabled";
    if (t.disposition === "silent" && sectionCount(t) === 0) return "silentReceiver";
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

        if (snd && snd.reply > 0 && rq && sectionCount(rq) === 0
                && (rq.disposition === "silent" || rq.disposition === "off"))
            out.push({ code: "interrogatorVsSilent", sev: "crit",
                       group: gi, key: key, disposition: rq.disposition });
        if (snd && snd.reply === 0 && rq && sectionCount(rq) > 0)
            out.push({ code: "pingerVsAnswering", sev: "warn", group: gi, key: key });
        if (rs && !snd)
            out.push({ code: "replyHandlerWithoutRequest", sev: "warn", group: gi, key: key });
    }
    return out;
}

// ── persistence payload ──────────────────────────────────────────────────────
function serialize(st) {
    return JSON.stringify({ v: 3, groups: st.groups, nodes: st.nodes,
                            nextId: st.nextId, written: st.written,
                            role: st.role, activeGroup: st.activeGroup,
                            appliedOnce: st.appliedOnce,
                            appliedSnapshot: st.appliedSnapshot });
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
    if (o.written && typeof o.written === "object") st.written = o.written;
    if (o.role === "initiator" || o.role === "transponder") st.role = o.role;
    if (typeof o.activeGroup === "number") st.activeGroup = o.activeGroup;
    if (typeof o.appliedOnce === "boolean") st.appliedOnce = o.appliedOnce;
    if (typeof o.appliedSnapshot === "string") st.appliedSnapshot = o.appliedSnapshot;

    // Repair anything the blob got wrong rather than trusting it: slots must be unique,
    // in range and owned by exactly one group, and every step must name a slot its group
    // still owns.
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
    st.written = normWritten(st.written);
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
    if (typeof t.disposition !== "string") t.disposition = "ack";
    t.advOpen = !!t.advOpen;
}

// A v1/v2 payload has no write set; treat an unreadable one as "nothing known written" so
// the first Apply cannot release slots it never wrote.
function normWritten(raw) {
    var out = {};
    if (!raw) return out;
    for (var k in raw) {
        var p = String(k).split(":");
        var cmd = parseInt(p[0], 10), ev = parseInt(p[1], 10);
        if (isNaN(cmd) || cmd < 0 || cmd >= SLOT_COUNT) continue;
        if (ev !== 1 && ev !== 2) continue;
        out[cmd + ":" + ev] = true;
    }
    return out;
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
        SLOT_COUNT: SLOT_COUNT,
        emptyState: emptyState, clone: clone,
        newSend: newSend, newTrigger: newTrigger, newGroup: newGroup,
        groupById: groupById, groupIndexById: groupIndexById, nodeById: nodeById,
        hasSlot: hasSlot, ownerOf: ownerOf, schedulable: schedulable,
        activeGroupOf: activeGroupOf, slotLabel: slotLabel, payloadBytes: payloadBytes,
        trigger: trigger, sectionCount: sectionCount, hasRewrite: hasRewrite,
        structOf: structOf, roleEvent: roleEvent,
        addGroup: addGroup, removeGroup: removeGroup, addNode: addNode,
        removeNode: removeNode, setNodeAddr: setNodeAddr, toggleNode: toggleNode,
        addStep: addStep, removeStep: removeStep, setStepCmd: setStepCmd,
        toggleSlot: toggleSlot,
        setSendField: setSendField, attachSend: attachSend, detachSend: detachSend,
        attachTrigger: attachTrigger, detachTrigger: detachTrigger,
        setDisposition: setDisposition, attachSection: attachSection,
        detachSection: detachSection, setSectionField: setSectionField,
        setAdvOpen: setAdvOpen, setAdvField: setAdvField,
        setActiveGroup: setActiveGroup, setRole: setRole,
        coverage: coverage, groupsView: groupsView, nodesView: nodesView,
        activeSlotCount: activeSlotCount, activeNodeCount: activeNodeCount,
        schedule: schedule, defaults: defaults, normWritten: normWritten,
        setupFrames: setupFrames, runFrames: runFrames,
        plannedWrites: plannedWrites, staleWrites: staleWrites,
        releaseFrames: releaseFrames, snapshot: snapshot, isStale: isStale,
        markApplied: markApplied,
        subroleInitiatorCode: subroleInitiatorCode,
        subroleTransponderCode: subroleTransponderCode,
        issueCodes: issueCodes, serialize: serialize, deserialize: deserialize
    };
}

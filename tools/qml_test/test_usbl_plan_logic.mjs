// Behaviour tests for the USBL plan logic. No Qt, no window, no GPU.
//
//   node tools/qml_test/test_usbl_plan_logic.mjs
//
// Exit 0 = all pass. Every UI defect this feature shipped is a rule in here.
//
// THE MODEL UNDER TEST. The eight slots are the state and they are totally partitioned:
// every slot is in exactly one group, always. A group owns a set of slots and one settings
// object they share -- that sharing is why the concept exists, so several slots can be edited
// at once. Groups are not on the wire, so membership can only be RECONSTRUCTED, and the rule
// that makes that possible is the definition: same settings, same group. Duplicates are
// legal (collapsing them mid-edit would fight the operator); the plan check offers a join.

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import path from "node:path";

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));
const L = require(path.join(here, "..", "..", "qml", "app", "UsblPlanLogic.js"));

let pass = 0;
const fails = [];

function ok(name, cond, detail) {
    if (cond) { pass++; console.log("  ok   " + name); }
    else { fails.push(name); console.log(`  FAIL ${name}${detail ? "   -- " + detail : ""}`); }
}
const j = (v) => JSON.stringify(v);
function eq(name, got, want) { ok(name, j(got) === j(want), `got ${j(got)}, want ${j(want)}`); }

// The bar always acts on the SELECTED group, so moving slots means selecting a target first.
// That is the model, not a convenience: under a total partition a slot cannot go "nowhere",
// it can only change hands.
function move(st, gid, cmds) {
    st = L.setActiveGroup(st, gid);
    for (const c of cmds) st = L.slotClick(st, c).state;
    return st;
}
// How a real plan is built: add a group (empty, at the defaults) and move slots into it.
function split(st, cmds) {
    st = L.addGroup(st);
    const id = st.groups[st.groups.length - 1].id;
    return { st: move(st, id, cmds), id };
}
// Union of every group's slots, which must always be exactly 0..7 with no repeats.
function partition(st) {
    const all = [];
    for (const g of st.groups) all.push(...g.slots);
    return all.sort((a, b) => a - b);
}
const ALL_SLOTS = [0, 1, 2, 3, 4, 5, 6, 7];

// ── the partition invariant ──────────────────────────────────────────────────
// The foundation: there is no unowned slot and never no group. Every other rule leans on it.
console.log("the partition");
{
    const st0 = L.initialState();
    eq("a fresh plan is ONE group holding every slot", st0.groups.length, 1);
    eq("...covering all eight", st0.groups[0].slots, ALL_SLOTS);
    eq("...and it is the selected one", L.activeGroupOf(st0).id, st0.groups[0].id);
    ok("...carrying the defaults", L.isDefaultSettings(st0.groups[0]));

    // Every structural mutator has to leave the partition total. Checked as a property over a
    // sequence of edits rather than one case, because that is how it broke before it existed.
    let st = st0;
    const seen = [];
    const a = split(st, [1, 2, 3]); st = a.st; seen.push(partition(st));
    const b = split(st, [5]);       st = b.st; seen.push(partition(st));
    st = L.setSendField(st, b.id, "fn", "bits");            seen.push(partition(st));
    st = move(st, a.id, [5]);                               seen.push(partition(st));
    st = L.removeGroup(st, a.id);                           seen.push(partition(st));
    st = L.joinGroups(st, st.groups.map((g) => g.id));       seen.push(partition(st));
    ok("every edit leaves all eight slots owned exactly once",
       seen.every((p) => j(p) === j(ALL_SLOTS)), j(seen));
    ok("...and never leaves zero groups", st.groups.length >= 1);

    eq("the seeded first-run plan is total too", partition(L.defaults()), ALL_SLOTS);
    ok("...with more than one group", L.defaults().groups.length > 1);
}

// ── immutability: the whole reason this module exists ────────────────────────
console.log("immutability");
{
    let st = L.initialState();
    const before = j(st);
    const after = L.addGroup(st);
    ok("a mutator returns a NEW object", after !== st);
    eq("...and leaves the input untouched", j(st), before);

    const s2 = L.addGroup(L.initialState());
    const fresh = s2.groups[s2.groups.length - 1].id;
    const s3 = L.slotClick(s2, 4).state;
    ok("nested state is copied, not aliased", s3.groups[0] !== s2.groups[0]);
    ok("...so the old snapshot still reads the old owner",
       !L.hasSlot(L.groupById(s2, fresh), 4) && L.hasSlot(L.groupById(s3, fresh), 4));

    // A no-op must not fabricate a new identity: QML would re-render for nothing.
    ok("an impossible edit returns the SAME state", L.removeGroup(s2, 9999) === s2);
    ok("...and so does a step on a missing node", L.addStep(s2, 9999, fresh) === s2);
    ok("...and so does selecting a group that is already selected",
       L.setActiveGroup(s2, fresh) === s2);
}

// ── the slot click: one meaning, MOVE ────────────────────────────────────────
// It used to mean three things depending on what you clicked, including "select that group",
// which is why stealing had to be forbidden. The tab row selects now, so the bar has exactly
// one job -- and under a total partition, moving is the only way a slot can change hands.
console.log("slot clicks");
{
    let st = L.initialState();
    const home = st.groups[0].id;

    st = L.addGroup(st);
    const g2 = st.groups[1].id;
    eq("a new group is born empty", L.groupById(st, g2).slots, []);
    eq("...and selected", L.activeGroupOf(st).id, g2);

    let r = L.slotClick(st, 3);
    eq("clicking a slot moves it into the selected group", r.action, "move");
    st = r.state;
    eq("...arriving there", L.groupById(st, g2).slots, [3]);
    ok("...and leaving its old group", !L.hasSlot(L.groupById(st, home), 3));
    eq("...with the partition intact", partition(st), ALL_SLOTS);

    r = L.slotClick(st, 3);
    eq("clicking a slot the selected group already owns does nothing", r.action, "already");
    ok("...returning the SAME state", r.state === st);

    st = L.slotClick(st, 4).state;
    eq("slots arrive in order, not click order", L.groupById(st, g2).slots, [3, 4]);

    eq("clicking out of range does nothing", L.slotClick(st, 99).action, "none");
    ok("...and returns the same object", L.slotClick(st, 99).state === st);

    // No slot is ever owned twice, which under a move-only model is structural rather than
    // something the click handler has to remember.
    const owners = L.coverage(st).map((c) => c.groupId);
    ok("every slot has exactly one owner", owners.every((o) => o >= 0));
    eq("...and coverage agrees with the groups",
       L.coverage(st).filter((c) => c.groupId === g2).map((c) => c.cmd), [3, 4]);
}

// ── dissolve on empty ────────────────────────────────────────────────────────
// A group with no slots has nothing to bulk-edit and could not survive a reconstruction, so
// it does not linger. Two ways in: it loses its last slot, or it was never filled.
console.log("dissolve on empty");
{
    let st = L.initialState();
    const home = st.groups[0].id;
    const s = split(st, [7]); st = s.st;
    eq("two groups now", st.groups.length, 2);

    // Take the lone slot back: the group it came from is emptied and goes away.
    st = move(st, home, [7]);
    eq("a group that loses its last slot dissolves", st.groups.length, 1);
    ok("...and it is gone by id", L.groupById(st, s.id) === null);
    eq("...with the partition still total", partition(st), ALL_SLOTS);

    // Never the target: it just gained a slot, and it is what the operator is looking at.
    let t = L.initialState();
    const one = split(t, ALL_SLOTS);
    eq("moving every slot into one group leaves exactly that group", one.st.groups.length, 1);
    eq("...holding all of them", L.groupById(one.st, one.id).slots, ALL_SLOTS);

    // Added but never filled: selecting away is the moment it is abandoned.
    let u = L.addGroup(L.initialState());
    const ghost = u.groups[u.groups.length - 1].id;
    const back = u.groups[0].id;
    eq("an unfilled group exists while selected", L.groupById(u, ghost).slots, []);
    u = L.setActiveGroup(u, back);
    ok("...and dissolves when the selection leaves it", L.groupById(u, ghost) === null);
    eq("...leaving the plan as it was", u.groups.length, 1);

    // A filled one survives the same navigation, obviously -- asserted because the dissolve
    // rule keys on emptiness and a wrong condition here would delete real work.
    let v = L.initialState();
    const kept = split(v, [0, 1]); v = kept.st;
    v = L.setActiveGroup(v, v.groups[0].id);
    ok("a group with slots survives losing the selection", !!L.groupById(v, kept.id));
}

// ── the group cap ────────────────────────────────────────────────────────────
console.log("group cap");
{
    let st = L.initialState();
    ok("a fresh plan can take another group", L.canAddGroup(st));

    // Eight groups is one slot each: the cap is exactly the point where every group is a
    // single slot, and a ninth could hold nothing.
    for (let c = 1; c < L.SLOT_COUNT; ++c) st = split(st, [c]).st;
    eq("eight slots split into eight groups", st.groups.length, L.MAX_GROUPS);
    ok("...and the plan says it is full", !L.canAddGroup(st));
    eq("...each holding one slot", st.groups.map((g) => g.slots.length),
       [1, 1, 1, 1, 1, 1, 1, 1]);

    const full = st;
    ok("a ninth addGroup is refused, returning the SAME state", L.addGroup(full) === full);

    // Joining two frees a place again.
    const fewer = L.joinGroups(full, [full.groups[0].id, full.groups[1].id]);
    ok("joining makes room", L.canAddGroup(fewer));

    // A blob from a build without the cap is trimmed, and the slots it drops come back
    // through the partition repair rather than vanishing.
    const groups = [];
    for (let i = 0; i < 12; ++i) groups.push(L.newGroup(i + 1, [i % L.SLOT_COUNT]));
    const over = L.deserialize(JSON.stringify({ v: 6, groups: groups, nodes: [],
                                                nextId: 20, activeId: 99, applied: {} }));
    ok("an over-long blob is trimmed to the cap", over.groups.length <= L.MAX_GROUPS);
    eq("...without losing a slot", partition(over), ALL_SLOTS);
    ok("...and the selection lands on something real", !!L.groupById(over, over.activeId));
}

// ── the views the UI binds to must agree ─────────────────────────────────────
// The operator saw coverage, the group tabs, the slot labels and the counter all disagreeing.
// They can only agree if each is derived from the same state.
console.log("derived views agree");
{
    let st = L.initialState();
    const s = split(st, [3, 4, 5]); st = s.st;

    const gv = L.groupsView(st).find((v) => v.id === s.id);
    eq("groupsView label is the compact run form", gv.label, "3–5");
    eq("groupsView count matches the slot set", gv.count, L.groupById(st, s.id).slots.length);
    eq("activeSlotCount matches too", L.activeSlotCount(st), gv.count);
    for (const c of [3, 4, 5]) eq(`coverage marks slot ${c}`, L.coverage(st)[c].groupId, s.id);

    const noGaps = L.groupsView(st).every((v) => v.label === L.slotLabel(L.groupById(st, v.id)));
    ok("every tab label matches its group", noGaps);
    ok("no cell is unowned, because there is no such thing",
       L.coverage(st).every((c) => c.groupId >= 0 && c.index >= 0));
    eq("coverage's isDefault agrees with the group's",
       L.coverage(st).map((c) => c.isDefault),
       L.coverage(st).map((c) => L.isDefaultSettings(L.groupById(st, c.groupId))));
}

// ── `def` is a label, not a group ────────────────────────────────────────────
// Whichever group carries the default settings renders as `def`. Nothing about it is special:
// edit it and the label moves off, and there may be none or -- until a join -- several.
console.log("the def label");
{
    let st = L.initialState();
    ok("the starting group is the def group", L.isDefaultSettings(st.groups[0]));
    eq("...and defaultGroupOf finds it", L.defaultGroupOf(st).id, st.groups[0].id);

    // Split at the defaults: two groups, both def. Legal, and the plan check will offer to
    // join them -- it must not be prevented, or editing fights the operator.
    const s = split(st, [6, 7]); st = s.st;
    eq("a fresh split leaves two groups at the defaults",
       L.groupsView(st).filter((v) => v.isDefault).length, 2);

    // Edit one and the label moves off it, with no bookkeeping anywhere.
    st = L.setSendField(st, s.id, "fn", "bits");
    ok("editing a group takes the def label off it", !L.isDefaultSettings(L.groupById(st, s.id)));
    eq("...leaving one def group", L.groupsView(st).filter((v) => v.isDefault).length, 1);

    // Edit the other one too and there is no def group at all, which is a legal plan.
    st = L.setSendField(st, st.groups[0].id, "reply", 999);
    ok("a plan can have no def group", L.defaultGroupOf(st) === null);
    eq("...and no tab claims the label", L.groupsView(st).filter((v) => v.isDefault).length, 0);

    // Back to the defaults by value, not by identity: the label is a property of settings.
    st = L.setSendField(st, st.groups[0].id, "reply", L.newSend().reply);
    ok("restoring the default values restores the label",
       L.isDefaultSettings(L.groupById(st, st.groups[0].id)));

    // Field order must not decide identity: the same settings written in another key order is
    // the same settings. Only a canonical signature gets this right.
    const twisted = L.clone(st);
    const g = twisted.groups[0];
    g.ini = { reply: g.ini.reply, send: { reply: g.ini.send.reply, payload: g.ini.send.payload,
                                          fn: g.ini.send.fn } };
    ok("settings identity ignores key order", L.isDefaultSettings(g));
}

// ── duplicates are legal, and joinable ───────────────────────────────────────
// Two groups with identical settings write identical bytes to their own slots, so nothing is
// wrong on the wire. The plan check says so and offers the tidy-up.
console.log("duplicates and join");
{
    let st = L.initialState();
    const a = split(st, [4, 5]); st = a.st;                 // both at the defaults
    const dups = L.duplicateSets(st);
    eq("groups with identical settings are reported as one set", dups.length, 1);
    eq("...naming both", dups[0].slice().sort(), [st.groups[0].id, a.id].sort());
    ok("...as an offer, not an error",
       L.issueCodes(st).find((i) => i.code === "duplicateGroups").sev === "info");
    eq("...carrying the ids the fix applies to",
       L.issueCodes(st).find((i) => i.code === "duplicateGroups").ids.slice().sort(),
       dups[0].slice().sort());

    // Different settings are never lumped together, however close.
    let diff = L.setSendField(st, a.id, "reply", 21000);
    eq("a single differing field splits the set", L.duplicateSets(diff).length, 0);
    ok("...and the plan check goes quiet",
       !L.issueCodes(diff).some((i) => i.code === "duplicateGroups"));

    // The join: union of slots, earliest survives, others gone.
    const joined = L.joinGroups(st, dups[0]);
    eq("joining leaves one group", joined.groups.length, 1);
    eq("...holding every slot", joined.groups[0].slots, ALL_SLOTS);
    eq("...keeping the earliest group's id", joined.groups[0].id, st.groups[0].id);
    eq("...and nothing left to report", L.duplicateSets(joined).length, 0);
    ok("...with the selection still pointing at a real group",
       !!L.groupById(joined, joined.activeId));

    // Joining groups that are NOT duplicates is not offered, but if asked it still preserves
    // the partition -- a fix must never be able to lose a slot.
    eq("joining preserves the partition", partition(L.joinGroups(diff, [st.groups[0].id, a.id])),
       ALL_SLOTS);
    ok("joining fewer than two groups is a no-op", L.joinGroups(st, [a.id]) === st);

    // Empty groups are not duplicate candidates: a fresh one carries the defaults by
    // construction and dissolves on its own, so reporting it would be noise.
    const withGhost = L.addGroup(st);
    eq("an unfilled group is not reported as a duplicate",
       L.duplicateSets(withGhost).length, L.duplicateSets(st).length);
}

// ── reconstruction: the rule that defines the concept ────────────────────────
// Nothing on the wire carries group membership, so it can only be rebuilt from the slot
// settings: same settings, same group. This is the most that is recoverable, and it is what
// the join converges on.
console.log("reconstruction");
{
    let st = L.initialState();
    const a = split(st, [2, 3]); st = a.st;
    const b = split(st, [6]);    st = b.st;                 // three groups, all at defaults

    const re = L.reconstruct(st);
    eq("identical settings reconstruct into ONE group", re.groups.length, 1);
    eq("...holding every slot that carried them", re.groups[0].slots, ALL_SLOTS);
    eq("...and the partition survives", partition(re), ALL_SLOTS);

    // Distinct settings stay distinct, and a plan with no duplicates is already the answer.
    let mixed = L.setSendField(st, a.id, "fn", "bits");
    mixed = L.setSendField(mixed, b.id, "reply", 12345);
    const re2 = L.reconstruct(mixed);
    eq("three distinct settings reconstruct as three groups", re2.groups.length, 3);
    eq("...as a fixed point", j(L.reconstruct(re2).groups.map((g) => g.slots)),
       j(re2.groups.map((g) => g.slots)));
    ok("...leaving what the device would receive unchanged",
       L.ROLES.every((r) => j(L.applyWrites(mixed, r)) === j(L.applyWrites(re2, r))));

    // A join of every duplicate set is reconstruction, restricted. One rule, two entry points.
    let viaJoin = st;
    for (const set of L.duplicateSets(st)) viaJoin = L.joinGroups(viaJoin, set);
    eq("joining every duplicate set gives the reconstruction",
       viaJoin.groups.map((g) => g.slots), L.reconstruct(st).groups.map((g) => g.slots));
}

// ── remove means reset to defaults ───────────────────────────────────────────
// Delete cannot mean delete: the slots need a home and every slot must have exactly one.
console.log("remove is reset-to-defaults");
{
    let st = L.initialState();
    const a = split(st, [0, 1]); st = a.st;
    st = L.setSendField(st, a.id, "fn", "bits");            // so it is not the def group
    const homeId = st.groups[0].id;

    st = L.removeGroup(st, a.id);
    ok("the group is gone", L.groupById(st, a.id) === null);
    eq("...its slots landed in the def group", L.groupById(st, homeId).slots, ALL_SLOTS);
    eq("...the partition is intact", partition(st), ALL_SLOTS);
    eq("...and the selection followed the slots", st.activeId, homeId);

    // With no def group to receive them, one is created -- that is what an unconfigured slot
    // belongs to, and the alternative is an orphan.
    let t = L.initialState();
    t = L.setSendField(t, t.groups[0].id, "fn", "bits");    // now nothing is at the defaults
    const only = t.groups[0].id;
    ok("no def group to start with", L.defaultGroupOf(t) === null);
    t = L.removeGroup(t, only);
    ok("removing the last group creates the def group to hold its slots",
       !!L.defaultGroupOf(t));
    eq("...with every slot in it", L.defaultGroupOf(t).slots, ALL_SLOTS);
    eq("...and exactly one group", t.groups.length, 1);
}

// ── steps ────────────────────────────────────────────────────────────────────
console.log("steps");
{
    let st = L.initialState();
    const s = split(st, [2, 6]); st = s.st;
    const g = s.id;
    st = L.addNode(st);
    const n = st.nodes[0].id;

    eq("a fresh node has no steps", L.nodesView(st)[0].cmdCount, 0);

    st = L.addStep(st, n, g);
    st = L.addStep(st, n, g);
    eq("consecutive steps walk the group's slots",
       L.nodesView(st)[0].steps.map((x) => x.cmd), [2, 6]);

    st = L.addStep(st, n, g);
    eq("a third step repeats rather than inventing a slot",
       L.nodesView(st)[0].steps.map((x) => x.cmd), [2, 6, 2]);
    eq("cmdCount follows", L.nodesView(st)[0].cmdCount, 3);

    st = L.removeStep(st, n, 0);
    eq("removeStep drops the right one",
       L.nodesView(st)[0].steps.map((x) => x.cmd), [6, 2]);

    // A step names a SLOT. When the slot changes hands the step follows it rather than being
    // dropped: the interrogation is still meaningful, only the group around it changed. Under
    // the old model the slot could become unowned and the step had to be deleted.
    const home = st.groups[0].id;
    st = move(st, home, [6]);
    eq("a step survives its slot moving", L.nodesView(st)[0].steps.map((x) => x.cmd), [6, 2]);
    eq("...now pointing at the new owner",
       L.nodesView(st)[0].steps.map((x) => x.group),
       L.nodesView(st)[0].steps.map((x) => L.ownerOf(st, x.cmd).id));
    ok("every step's group is the slot's actual owner",
       L.nodesView(st)[0].steps.every((x) => L.hasSlot(L.groupById(st, x.group), x.cmd)));
}

// ── schedule ─────────────────────────────────────────────────────────────────
console.log("schedule");
{
    let st = L.initialState();
    const g = st.groups[0].id;
    st = L.addNode(st);
    st = L.addNode(st);
    const [n1, n2] = st.nodes.map((x) => x.id);
    st = L.addStep(st, n1, g);

    const sched = L.schedule(st);
    eq("a node with no steps contributes one implicit step",
       sched.filter((s) => s.implicit).length, 1);
    eq("...whose cmd is 0", sched.find((s) => s.implicit).cmd, 0);
    eq("schedule agrees with the node cards", sched.length,
       L.nodesView(st).reduce((a, v) => a + Math.max(1, v.steps.length), 0));

    st = L.toggleNode(st, n2);
    eq("a disabled node contributes nothing", L.schedule(st).length, 1);

    st = L.setNodeAddr(st, n1, 200);
    eq("addresses are clamped to the wire range", L.nodesView(st)[0].addr, 8);
}

// ── frame accounting ─────────────────────────────────────────────────────────
// Apply is total: eight frames for the eight slots, always, plus the transponder's two
// globals. A plan where nothing is configured is still applicable -- it resets the side to
// defaults -- which is why release tracking does not exist.
console.log("frame accounting");
{
    const plain = L.initialState();
    eq("an unconfigured plan still writes all eight slots as initiator",
       L.applyFrames(plain, "initiator"), 8);
    eq("...and eight plus the two globals as transponder",
       L.applyFrames(plain, "transponder"), 10);
    eq("...with nothing configured", L.configuredSlots(plain, "initiator"), 0);
    ok("...every frame being an all-default reset",
       L.applyWrites(plain, "initiator").every(
           (w) => !w.configured && w.recvFn === 0 && w.sendFn === 0
               && w.recvBits === 0 && w.sendHex === "" && w.eventAction === 0));

    let st = L.initialState();
    const s = split(st, [0, 1]); st = s.st;
    const g = s.id;
    eq("a plan with no handler writes eight defaults",
       L.configuredSlots(st, "initiator"), 0);

    st = L.attachTrigger(st, g, "reply");
    eq("attaching a reply handler configures that group's slots",
       L.configuredSlots(st, "initiator"), L.groupById(st, g).slots.length);
    eq("...but the frame count never moves", L.applyFrames(st, "initiator"), 8);
    eq("...and the transponder half is untouched by it",
       L.configuredSlots(st, "transponder"), 0);

    st = L.attachTrigger(st, g, "request");
    eq("each half is configured independently",
       [L.configuredSlots(st, "initiator"), L.configuredSlots(st, "transponder")],
       [2, 2]);

    // The invariant that stops the count and the wire from drifting apart again.
    for (const r of L.ROLES)
        eq(`applyFrames == applyWrites + globals (${r})`,
           L.applyFrames(st, r),
           L.applyWrites(st, r).length + (r === "transponder" ? 2 : 0));
}

// ── section and rewrite state ────────────────────────────────────────────────
// What has to be right is how many sections are attached and whether a rewrite is armed,
// because those become receiver_function / sender_function / the SendBack* actions.
console.log("sections and rewrites");
{
    let st = L.initialState();
    const g = st.groups[0].id;
    st = L.attachTrigger(st, g, "request");
    eq("a bare trigger has no sections", L.sectionCount(L.trigger(st, g, "request")), 0);
    ok("a bare trigger has no rewrite", !L.hasRewrite(L.trigger(st, g, "request")));

    st = L.attachSection(st, g, "request", "recv");
    eq("one section counted", L.sectionCount(L.trigger(st, g, "request")), 1);

    st = L.attachSection(st, g, "request", "send");
    eq("both sections counted", L.sectionCount(L.trigger(st, g, "request")), 2);

    st = L.detachSection(st, g, "request", "send");
    eq("detaching drops back to one", L.sectionCount(L.trigger(st, g, "request")), 1);

    st = L.setAdvField(st, g, "request", "cmdIdAction", "Replacement");
    ok("a replacement rule arms the rewrite", L.hasRewrite(L.trigger(st, g, "request")));
    st = L.setAdvOpen(st, g, "request", false);
    ok("closing advanced disarms it", !L.hasRewrite(L.trigger(st, g, "request")));
}

// ── consistency rules ────────────────────────────────────────────────────────
console.log("consistency rules");
{
    let st = L.initialState();
    const s = split(st, [0, 1]); st = s.st;
    const g = s.id;
    const codes = (x) => L.issueCodes(x).map((i) => i.code);

    // Slots at the defaults are not a defect: they are a group like any other, and there is
    // no unowned slot left to warn about.
    ok("slots at the defaults are not reported", !codes(st).includes("unownedSlots"));

    st = L.setSendField(st, g, "fn", "bits");
    st = L.setSendField(st, g, "payload", "AA BB");     // 2 bytes -> 16 bit
    ok("a payload with no receiver is reported",
       codes(st).includes("requestCarriesButNoReceiver"));

    st = L.attachTrigger(st, g, "request");
    st = L.attachSection(st, g, "request", "recv");
    st = L.setSectionField(st, g, "request", "recv", "bits", 8);
    const m = L.issueCodes(st).find((i) => i.code === "requestBitsMismatch");
    ok("a bit-length mismatch is critical", m && m.sev === "crit");
    eq("...and names both numbers", [m.bits, m.expected], [16, 8]);

    st = L.setSectionField(st, g, "request", "recv", "bits", 16);
    ok("matching lengths clear it", !codes(st).includes("requestBitsMismatch"));

    st = L.setSendField(st, g, "reply", 0);
    ok("a pinger facing an answering transponder is flagged",
       codes(st).includes("pingerVsAnswering"));
    st = L.setSendField(st, g, "reply", 20000);
    ok("...and an interrogator is not",
       !codes(st).includes("pingerVsAnswering"));

    // An unfilled group is worth mentioning while it exists, because it writes nothing.
    const ghost = L.addGroup(st);
    ok("a group with no slots is reported", codes(ghost).includes("groupOwnsNoSlots"));
}

// ── derived roles ────────────────────────────────────────────────────────────
console.log("derived roles");
{
    let st = L.initialState();
    const g = st.groups[0].id;
    eq("a group that sends and waits is an interrogator",
       L.subroleInitiatorCode(L.groupById(st, g)), "interrogator");
    st = L.setSendField(st, g, "reply", 0);
    eq("...and a pinger when it expects nothing back",
       L.subroleInitiatorCode(L.groupById(st, g)), "pinger");
    st = L.detachSend(st, g);
    eq("...and not an initiator at all with no request",
       L.subroleInitiatorCode(L.groupById(st, g)), "notInitiator");

    eq("no request handler means the device default",
       L.subroleTransponderCode(L.groupById(st, g)), "defaultTransponder");
    st = L.attachTrigger(st, g, "request");
    st = L.attachSection(st, g, "request", "recv");
    st = L.attachSection(st, g, "request", "send");
    eq("receiving and sending is a relay",
       L.subroleTransponderCode(L.groupById(st, g)), "relay");
}

// ── per-role staleness, measured in wire bytes ───────────────────────────────
// The two halves are applied separately, so "applied" and "changed since applied" are per
// role. The snapshot is what the device would RECEIVE, not how the pane is organised -- which
// is what lets the plan check offer a join without provoking a pointless re-apply.
console.log("per-role staleness");
{
    let st = L.initialState();
    const s = split(st, [0, 1]); st = s.st;
    const g = s.id;
    st = L.attachTrigger(st, g, "reply");
    ok("nothing is applied to begin with",
       !L.appliedOnce(st, "initiator") && !L.appliedOnce(st, "transponder"));
    ok("...so nothing is stale either", !L.anyStale(st));

    st = L.markApplied(st, "initiator");
    ok("applying the initiator half records only that half",
       L.appliedOnce(st, "initiator") && !L.appliedOnce(st, "transponder"));
    ok("...and it is not stale", !L.isStale(st, "initiator"));

    st = L.attachSection(st, g, "reply", "recv");
    ok("editing that half makes it stale", L.isStale(st, "initiator"));
    ok("...and the header notices", L.anyStale(st));
    ok("...while the unapplied half is not called stale", !L.isStale(st, "transponder"));

    let t = L.markApplied(st, "initiator");
    t = L.attachTrigger(t, g, "request");
    ok("editing the transponder half leaves the initiator half applied",
       !L.isStale(t, "initiator"));

    // Reorganising writes nothing new, so it must not read as a change. This is the promise
    // the plan check's join button makes.
    let d = L.initialState();
    const two = split(d, [4, 5]); d = two.st;               // duplicate settings
    d = L.markApplied(L.markApplied(d, "initiator"), "transponder");
    const sets = L.duplicateSets(d);
    const after = L.joinGroups(d, sets[0]);
    ok("a join changes no wire byte",
       L.ROLES.every((r) => j(L.applyWrites(d, r)) === j(L.applyWrites(after, r))));
    ok("...so it does not make the plan stale", !L.anyStale(after));

    // ...and a change Apply DOES carry still reads as stale, or the check would be useless.
    const edited = L.attachTrigger(after, after.groups[0].id, "reply");
    ok("a change to what Apply writes still reads as stale", L.isStale(edited, "initiator"));

    // Whereas the interrogation request is not part of the slot table at all -- it rides the
    // schedule, not Apply -- so editing it cannot make the slot table stale. True of the old
    // per-group snapshot too; asserted because "the badge did not light up" looks like a bug
    // until you know Apply never carried this field.
    const sendEdit = L.setSendField(after, after.groups[0].id, "fn", "bits");
    ok("editing the interrogation request does not make the slot table stale",
       !L.isStale(sendEdit, "initiator"));
    ok("...because it changes no byte Apply sends",
       L.ROLES.every((r) => j(L.applyWrites(after, r)) === j(L.applyWrites(sendEdit, r))));
}

// ── the frames Apply puts on the wire ────────────────────────────────────────
// Every field here becomes a byte in the frame Apply emits, and the device offers no
// read-back. Clicking Apply proves a frame was sent; only this proves it was the right one.
console.log("apply writes");
{
    // Transponder half of the seeded plan: baseline group 0,4-7 with a bare handler, data
    // group 1-3 receiving 16 bit and answering with FF 01.
    let st = L.defaults();
    const tr = L.applyWrites(st, "transponder");
    eq("one frame per slot", tr.length, 8);
    eq("every slot appears exactly once", tr.map((w) => w.cmd), ALL_SLOTS);
    ok("the transponder half writes only event 1", tr.every((e) => e.event === 1));
    eq("...configuring every slot both groups hold",
       tr.filter((e) => e.configured).map((e) => e.cmd), ALL_SLOTS);
    const data = tr.filter((e) => [1, 2, 3].indexOf(e.cmd) >= 0);
    ok("the data group's slots carry its receive length",
       data.every((e) => e.recvBits === 16 && e.recvFn === 1));
    ok("...and its answer payload", data.every((e) => e.sendHex === "FF 01" && e.sendFn === 1));
    const base = tr.filter((e) => [0, 4, 5, 6, 7].indexOf(e.cmd) >= 0);
    ok("the baseline group's slots carry a bare handler",
       base.every((e) => e.recvFn === 0 && e.sendFn === 0 && e.sendHex === ""));

    const ini = L.applyWrites(st, "initiator");
    ok("the initiator half writes only event 2", ini.every((e) => e.event === 2));
    eq("...for all eight slots regardless", ini.length, 8);
    eq("...configuring only the group that has a reply handler",
       ini.filter((e) => e.configured).map((e) => e.cmd), [1, 2, 3]);
}

// ── persistence ──────────────────────────────────────────────────────────────
console.log("persistence");
{
    let st = L.defaults();
    st = L.markApplied(st, "transponder");
    const back = L.deserialize(L.serialize(st));
    eq("a round trip preserves the groups",
       back.groups.map((g) => g.slots), st.groups.map((g) => g.slots));
    eq("...the nodes", back.nodes.length, st.nodes.length);
    eq("...and what was applied, per role", back.applied, st.applied);
    ok("...so the applied half is still not stale", !L.isStale(back, "transponder"));
    eq("...and the selection, by id not by position", back.activeId, st.activeId);

    ok("garbage yields a usable plan, not a broken one",
       L.deserialize("{{{").groups.length >= 1);
    eq("...still totally partitioned", partition(L.deserialize("{{{")), ALL_SLOTS);
    eq("an empty blob does the same", partition(L.deserialize("")), ALL_SLOTS);

    // Repairs, not trust: doubly-owned, out-of-range and missing slots all had to be
    // possible to write into a v5 blob, because v5 allowed them.
    const messy = JSON.stringify({
        v: 5, activeGroup: 0, nextId: 7, applied: {}, nodes: [],
        groups: [{ id: 1, slots: [0, 1, 1, 99], ini: { send: L.newSend(), reply: null },
                   tr: { request: null } },
                 { id: 2, slots: [1, 2], ini: { send: L.newSend(), reply: null },
                   tr: { request: null } }]
    });
    const fixed = L.deserialize(messy);
    eq("...the second group loses the slot the first already had", L.groupById(fixed, 2).slots, [2]);
    // Group 1 is at the defaults, so it IS the def group and the orphans land in it -- the
    // repeat, the out-of-range 99 and the never-mentioned 3..7 all resolved in one pass.
    eq("duplicate and out-of-range slots are cleaned up, and orphans join the def group",
       L.groupById(fixed, 1).slots, [0, 1, 3, 4, 5, 6, 7]);
    eq("...which is what defaultGroupOf reports", L.defaultGroupOf(fixed).id, 1);
    eq("...leaving a total partition", partition(fixed), ALL_SLOTS);

    // An empty group cannot survive a reload: nothing would reconstruct it.
    const withGhost = JSON.stringify({
        v: 6, activeId: 5, nextId: 9, applied: {}, nodes: [],
        groups: [{ id: 4, slots: ALL_SLOTS, ini: { send: L.newSend(), reply: null },
                   tr: { request: null } },
                 { id: 5, slots: [], ini: { send: L.newSend(), reply: null },
                   tr: { request: null } }]
    });
    const loaded = L.deserialize(withGhost);
    eq("a saved empty group is dropped on load", loaded.groups.length, 1);
    ok("...and the selection moves to something real", !!L.groupById(loaded, loaded.activeId));
}

// ── v5 blobs: the applied record survives a change of snapshot format ────────
// v5 recorded staleness per GROUP; v6 records it per wire byte. A v5 string can never equal a
// v6 one, so carrying it over verbatim would make every previously-applied plan claim
// "changed since applied" on first load -- a lie that costs an operator a needless write.
// It is decidable: if the loaded plan still matches the v5 record, nothing changed since.
console.log("v5 applied migration");
{
    const g1 = { id: 1, slots: [0, 1, 2, 3], ini: { send: L.newSend(), reply: null },
                 tr: { request: null } };
    const g2 = { id: 2, slots: [4, 5, 6, 7], ini: { send: L.newSend(), reply: null },
                 tr: { request: null } };
    // Exactly what v5 wrote for the transponder half: [slots, tr.request] per group.
    const v5record = JSON.stringify([[g1.slots, null], [g2.slots, null]]);
    const blob = (applied) => JSON.stringify({ v: 5, groups: [g1, g2], nodes: [], nextId: 3,
                                               activeGroup: 1, applied: applied });

    const same = L.deserialize(blob({ transponder: v5record }));
    ok("a v5 plan unchanged since Apply still counts as applied",
       L.appliedOnce(same, "transponder"));
    ok("...and is NOT stale despite the format change", !L.isStale(same, "transponder"));
    eq("...its record having been rewritten in the new format",
       same.applied.transponder, L.snapshot(same, "transponder"));
    ok("...while the half that was never applied stays unapplied",
       !L.appliedOnce(same, "initiator"));

    const changed = L.deserialize(blob({ transponder: JSON.stringify([[[9], null]]) }));
    ok("a v5 plan that really did change still reads as stale",
       L.isStale(changed, "transponder"));

    eq("v5's selection index becomes the id it pointed at", same.activeId, g2.id);
}

// ── v4 blobs: a real saved plan must survive the role/release removal ────────
console.log("v4 migration");
{
    const g1 = L.newGroup(2, [6]);
    g1.tr.request = L.newTrigger();
    const g2 = L.newGroup(3, [4, 5]);
    g2.tr.request = L.newTrigger();
    g2.tr.request.recv = { fmt: "bits", bits: 16 };
    g2.tr.request.send = { fmt: "bits", payload: "FF 01" };

    const v4 = JSON.stringify({
        v: 4, groups: [g1, g2], nodes: [{ id: 4, addr: 1, active: true, refs: [] }],
        nextId: 7, activeGroup: 0, role: "transponder",
        written: { "6:1": true, "4:1": true, "5:1": true },
        appliedOnce: true, appliedSnapshot: JSON.stringify([[g1.slots, g1.tr.request],
                                                           [g2.slots, g2.tr.request]])
    });

    const st = L.deserialize(v4);
    eq("the groups survive", L.groupById(st, 2).slots, [6]);
    ok("the payload survives", L.groupById(st, 3).tr.request.send.payload === "FF 01");
    ok("the old current role becomes that role's applied record",
       L.appliedOnce(st, "transponder"));
    ok("...and is not stale, because nothing changed", !L.isStale(st, "transponder"));
    ok("the half that was never written stays unapplied", !L.appliedOnce(st, "initiator"));
    ok("`written` and `role` are dropped, not carried",
       !("written" in st) && !("role" in st));
    ok("...and a re-save does not resurrect them",
       L.serialize(st).indexOf("written") < 0 && L.serialize(st).indexOf('"role"') < 0);
    eq("the slots the blob never mentioned are owned by the def group",
       L.defaultGroupOf(st).slots, [0, 1, 2, 3, 7]);
    eq("...so the migrated plan is totally partitioned", partition(st), ALL_SLOTS);
    eq("the migrated plan still writes all eight slots",
       L.applyFrames(st, "transponder"), 10);
}

// ── v3 blobs: the dead disposition field must not survive a load ─────────────
console.log("legacy blob repair");
{
    const legacy = JSON.stringify({
        v: 3, role: "transponder", activeGroup: 0, nextId: 9, written: {},
        appliedOnce: false, appliedSnapshot: "", nodes: [],
        groups: [{ id: 1, slots: [0, 1], ini: { send: null, reply: null },
                   tr: { request: { disposition: "off", recv: null, send: null,
                                    advOpen: false,
                                    adv: { eventAction: "Swap", cmdIdAction: "Incoming",
                                           cmdIdRepl: 0, addrAction: "Incoming",
                                           addrRepl: 0 } } } }]
    });
    const st = L.deserialize(legacy);
    const t = L.trigger(st, 1, "request");
    ok("the trigger survives", !!t);
    ok("disposition is stripped on load", !("disposition" in t));
    ok("and never reappears in a save", L.serialize(st).indexOf("disposition") < 0);
    eq("a payload-free transponder is a plain transponder",
       L.subroleTransponderCode(L.groupById(st, 1)), "transponder");
    eq("...and the slots it never mentioned are owned", partition(st), ALL_SLOTS);
}

console.log("");
console.log(`${pass} passed, ${fails.length} failed`);
for (const f of fails) console.log("  FAILED: " + f);
process.exit(fails.length ? 1 : 0);

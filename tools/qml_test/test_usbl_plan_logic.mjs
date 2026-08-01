// Behaviour tests for the USBL plan logic. No Qt, no window, no GPU.
//
//   node tools/qml_test/test_usbl_plan_logic.mjs
//
// Exit 0 = all pass. Every UI defect this feature shipped is a rule in here.

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

// The bar acts on the SELECTED group, so a test that names a group selects it first.
// This is not a convenience -- it is the model: there is no way to touch a slot without
// first being in the group that will own it, which is what killed accidental stealing.
function click(st, groupId, cmd) {
    const i = L.groupIndexById(st, groupId);
    if (i >= 0 && st.activeGroup !== i) st = L.setActiveGroup(st, i);
    return L.slotClick(st, cmd).state;
}

// ── immutability: the whole reason this module exists ────────────────────────
console.log("immutability");
{
    let st = L.emptyState();
    const before = j(st);
    const after = L.addGroup(st);
    ok("a mutator returns a NEW object", after !== st);
    eq("...and leaves the input untouched", j(st), before);

    let s2 = L.addGroup(L.emptyState());
    const g = s2.groups[0].id;
    const s3 = L.slotClick(s2, 4).state;
    ok("nested state is copied, not aliased", s3.groups[0] !== s2.groups[0]);
    ok("...so the old snapshot still reads the old slots",
       !L.hasSlot(L.groupById(s2, g), 4) && L.hasSlot(L.groupById(s3, g), 4));

    // A no-op must not fabricate a new identity: QML would re-render for nothing.
    ok("an impossible edit returns the SAME state", L.removeGroup(s2, 9999) === s2);
    ok("...and so does a step on a missing node", L.addStep(s2, 9999, g) === s2);
}

// ── slot ownership is exclusive ──────────────────────────────────
console.log("slot ownership");
{
    let st = L.addGroup(L.emptyState());
    const g1 = st.groups[0].id;
    // A new group claims the first FREE slot so it is usable immediately -- assert that
    // rather than assuming an empty group.
    eq("a new group claims the first free slot", L.groupById(st, g1).slots, [0]);

    st = click(st, g1, 1);
    eq("a group holds the slots given to it", L.groupById(st, g1).slots, [0, 1]);

    st = L.addGroup(st);
    const g2 = st.groups[1].id;
    eq("the next group claims the next free slot", L.groupById(st, g2).slots, [2]);

    st = click(st, g2, 2);
    eq("clicking its own slot releases it", L.groupById(st, g2).slots, []);
    eq("...and it returns to nobody, not to a previous owner",
       L.coverage(st)[2].groupId, -1);

    // Every slot is owned by at most one group, always -- including after a transfer,
    // which is release-then-assign and can therefore never double-own.
    st = click(st, g1, 5);
    st = click(st, g1, 5);          // release
    st = click(st, g2, 5);          // and hand it over
    const owners = L.coverage(st).filter((c) => c.groupId >= 0);
    ok("no slot is owned twice",
       new Set(owners.map((c) => c.cmd)).size === owners.length);
    eq("...the transfer landed", L.groupById(st, g2).slots, [5]);
    ok("...and left the old owner", L.groupById(st, g1).slots.indexOf(5) < 0);
}

// ── the group cap ────────────────────────────────────────────────────────────
// Eight slots, so eight groups at most: a ninth is guaranteed to own nothing. Enforced in
// the model, not just by hiding the chip, so a stale binding cannot get past it.
console.log("group cap");
{
    let st = L.emptyState();
    ok("an empty plan can take a group", L.canAddGroup(st));
    for (let i = 0; i < L.MAX_GROUPS; ++i) st = L.addGroup(st);
    eq("groups stop at MAX_GROUPS", st.groups.length, L.MAX_GROUPS);
    ok("...and the plan says so", !L.canAddGroup(st));

    const full = st;
    st = L.addGroup(st);
    ok("a ninth addGroup is refused, returning the SAME state", st === full);

    // Every slot is spoken for at the cap, so the last groups own nothing -- which is
    // exactly why they must still be selectable from the chips.
    eq("the cap is reached with every slot claimed",
       L.coverage(full).filter((c) => c.groupId < 0).length, 0);

    // Removing one frees a place again.
    const fewer = L.removeGroup(full, full.groups[0].id);
    ok("removing a group makes room", L.canAddGroup(fewer));

    // A blob from before the cap must be trimmed, not trusted.
    const groups = [];
    for (let i = 0; i < 12; ++i) groups.push(L.newGroup(i + 1, []));
    const over = L.deserialize(JSON.stringify({ v: 5, groups: groups, nodes: [],
                                                nextId: 20, activeGroup: 11, applied: {} }));
    eq("an over-long blob is trimmed to the cap", over.groups.length, L.MAX_GROUPS);
    ok("...and the selection is pulled back into range",
       over.activeGroup < over.groups.length);
}

// ── the four views the UI binds to must agree ────────────────────────────────
// The operator saw coverage, the group chips, the slot buttons and the counter all
// disagreeing. They can only agree if each is derived from the same state.
console.log("derived views agree");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    // Drop the auto-claimed slot so this block controls the whole set.
    for (const c of L.groupById(st, g).slots.slice()) st = click(st, g, c);
    st = click(st, g, 3);
    st = click(st, g, 4);
    st = click(st, g, 5);

    const gv = L.groupsView(st)[0];
    eq("groupsView label is the compact run form", gv.label, "3–5");
    eq("groupsView count matches the slot set", gv.count, L.groupById(st, g).slots.length);
    eq("activeSlotCount matches too", L.activeSlotCount(st), gv.count);
    for (const c of [3, 4, 5]) eq(`coverage marks slot ${c}`, L.coverage(st)[c].groupId, g);
    eq("coverage leaves the rest unowned",
       L.coverage(st).filter((c) => c.groupId < 0).map((c) => c.cmd), [0, 1, 2, 6, 7]);

    const noGaps = L.groupsView(st).every((v) => v.label === L.slotLabel(L.groupById(st, v.id)));
    ok("every chip label matches its group", noGaps);
}

// ── steps ────────────────────────────────────────────────────────────────────
console.log("steps");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    for (const c of L.groupById(st, g).slots.slice()) st = click(st, g, c);
    st = click(st, g, 2);
    st = click(st, g, 6);
    st = L.addNode(st);
    const n = st.nodes[0].id;

    eq("a fresh node has no steps", L.nodesView(st)[0].cmdCount, 0);

    st = L.addStep(st, n, g);
    st = L.addStep(st, n, g);
    eq("consecutive steps walk the group's slots",
       L.nodesView(st)[0].steps.map((s) => s.cmd), [2, 6]);

    st = L.addStep(st, n, g);
    eq("a third step repeats rather than inventing a slot",
       L.nodesView(st)[0].steps.map((s) => s.cmd), [2, 6, 2]);
    eq("cmdCount follows", L.nodesView(st)[0].cmdCount, 3);

    st = L.removeStep(st, n, 0);
    eq("removeStep drops the right one",
       L.nodesView(st)[0].steps.map((s) => s.cmd), [6, 2]);

    // Losing a slot must not leave a step pointing at it.
    st = click(st, g, 6);
    ok("steps are clamped to slots the group still owns",
       L.nodesView(st)[0].steps.every((s) => L.hasSlot(L.groupById(st, g), s.cmd)),
       j(L.nodesView(st)[0].steps));

    // Losing every slot drops the steps entirely.
    st = click(st, g, 2);
    eq("a group with no slots has no steps", L.nodesView(st)[0].cmdCount, 0);
}

// ── schedule ─────────────────────────────────────────────────────────────────
console.log("schedule");
{
    let st = L.addGroup(L.emptyState());
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

// ── frame accounting ─────────────────────────────────────────────
// Apply is total: eight frames for the eight slots, always, plus the transponder's two
// globals. That is what makes an empty plan applicable -- it resets the side to defaults
// -- and it is why release tracking no longer exists.
console.log("frame accounting");
{
    const empty = L.emptyState();
    eq("an empty plan still writes all eight slots as initiator",
       L.applyFrames(empty, "initiator"), 8);
    eq("...and eight plus the two globals as transponder",
       L.applyFrames(empty, "transponder"), 10);
    eq("...with nothing configured", L.configuredSlots(empty, "initiator"), 0);
    ok("...every frame being an all-default reset",
       L.applyWrites(empty, "initiator").every(
           (w) => !w.configured && w.recvFn === 0 && w.sendFn === 0
               && w.recvBits === 0 && w.sendHex === "" && w.eventAction === 0));

    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    st = L.slotClick(st, 1).state;                    // group now owns 2 slots (0 and 1)
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

// ── the one bar: select, release, assign ─────────────────────────────
// Clicking a slot used to STEAL it from whichever group held it, which is unusable in
// practice: you cannot tell before clicking whether you are about to take something.
console.log("slot clicks");
{
    let st = L.emptyState();
    let r = L.slotClick(st, 3);
    eq("clicking a free slot with no groups creates one", r.action, "create");
    st = r.state;
    eq("...holding exactly that slot", st.groups[0].slots, [3]);
    eq("...and selecting it", st.activeGroup, 0);

    r = L.slotClick(st, 5);
    eq("a free slot joins the selected group", r.action, "assign");
    st = r.state;
    eq("...in slot order", L.groupById(st, st.groups[0].id).slots, [3, 5]);

    r = L.slotClick(st, 3);
    eq("the selected group's own slot is released", r.action, "release");
    st = r.state;
    eq("...leaving the rest", st.groups[0].slots, [5]);
    eq("...and nobody holding it", L.ownerOf(st, 3), null);

    // A second group, then the rule that matters: no stealing.
    st = L.addGroup(st, [3]);
    const second = st.activeGroup;
    r = L.slotClick(st, 5);
    eq("another group's slot SELECTS that group, never steals", r.action, "select");
    st = r.state;
    ok("...so ownership is unchanged", L.ownerOf(st, 5).slots.indexOf(5) >= 0);
    eq("...and the selection moved", st.activeGroup, 0);
    ok("...away from the group we were in", st.activeGroup !== second);

    // Moving a slot therefore takes two explicit clicks, both in the bar.
    st = L.slotClick(st, 5).state;                    // release from its owner
    st = L.setActiveGroup(st, second);
    st = L.slotClick(st, 5).state;                    // claim for the other group
    eq("release-then-assign moves a slot in two clicks",
       L.groupById(st, st.groups[second].id).slots, [3, 5]);

    eq("clicking out of range does nothing", L.slotClick(st, 99).action, "none");
    ok("...and returns the same object", L.slotClick(st, 99).state === st);
}

// ── section and rewrite state ────────────────────────────────────────────────
// Every trigger goes out as one v6 USBLCmdConfig now; there is no struct to choose. What
// still has to be right is how many sections are attached and whether a rewrite is armed,
// because those become receiver_function / sender_function / the SendBack* actions.
console.log("sections and rewrites");
{
    let st = L.attachTrigger(L.addGroup(L.emptyState()), 2, "request");
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
}

// ── per-role staleness ─────────────────────────────────────────
// The two halves are applied separately, so "applied" and "changed since applied" are
// per role. Applying one must not claim anything about the other.
console.log("per-role staleness");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
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
    ok("...while the unapplied half is not called stale",
       !L.isStale(st, "transponder"));

    // Editing the OTHER half must not disturb the applied one.
    let t = L.markApplied(st, "initiator");
    t = L.attachTrigger(t, g, "request");
    ok("editing the transponder half leaves the initiator half applied",
       !L.isStale(t, "initiator"));
}

// ── the frames Apply puts on the wire ────────────────────────────────────────
// Every field here becomes a byte in an ID_USBL_CONTROL v6 USBLCmdConfig, and the device
// offers no read-back. Clicking Apply proves a frame was sent; only this proves it was
// the right one.
console.log("apply writes");
{
    // Transponder half of the seeded plan: baseline group 0,4-7 with a bare handler,
    // data group 1-3 receiving 16 bit and answering with FF 01.
    let st = L.defaults();
    const w = L.applyWrites(st, "transponder");
    eq("one frame per owned slot, no wildcard", w.length, 8);
    ok("every frame is the transponder event", w.every((e) => e.event === 1));
    ok("nothing is a release before the first apply", w.every((e) => !e.release));
    eq("...covering all eight slots",
       w.map((e) => e.cmd).sort((a, b) => a - b), [0, 1, 2, 3, 4, 5, 6, 7]);

    const bare = w.find((e) => e.cmd === 0);
    eq("a bare handler carries FunctionDefault both ways",
       [bare.recvFn, bare.recvBits, bare.sendFn, bare.sendHex], [0, 0, 0, ""]);

    const data = w.find((e) => e.cmd === 1);
    eq("a receiving+answering handler is FunctionBitArray both ways",
       [data.recvFn, data.recvBits, data.sendFn, data.sendHex], [1, 16, 1, "FF 01"]);
    eq("...with no rewrite armed",
       [data.eventAction, data.cmdIdAction, data.cmdIdRepl,
        data.addrAction, data.addrRepl], [0, 0, 0, 0, 0]);
    ok("all three data slots emit identical bytes",
       [2, 3].every((c) => {
           const e = w.find((x) => x.cmd === c);
           return e.recvFn === data.recvFn && e.recvBits === data.recvBits
               && e.sendFn === data.sendFn && e.sendHex === data.sendHex;
       }));

    // llgeo is 2 in USBLCmdConfig::Function -- 4 in the struct that no longer exists.
    const gid = st.groups[1].id;
    let geo = L.setSectionField(st, gid, "request", "send", "fmt", "llgeo");
    eq("position+azimuth is Function 2, not the removed struct's 4",
       L.applyWrites(geo).find((e) => e.cmd === 1).sendFn, 2);

    // Rewrite rules must reach the wire, not just the advanced panel.
    let rw = L.setAdvField(st, gid, "request", "cmdIdAction", "Replacement");
    rw = L.setAdvField(rw, gid, "request", "cmdIdRepl", 5);
    rw = L.setAdvField(rw, gid, "request", "eventAction", "Same");
    const e1 = L.applyWrites(rw).find((x) => x.cmd === 1);
    eq("a replacement rule reaches the frame",
       [e1.cmdIdAction, e1.cmdIdRepl, e1.eventAction], [1, 5, 1]);

    // Detaching a handler needs no bookkeeping: the slot is written every Apply, so it
    // reverts to defaults on its own. This is what replaced release tracking.
    let rel = L.markApplied(st, "transponder");
    eq("an applied plan re-emits the same eight frames",
       L.applyWrites(rel, "transponder").length, 8);
    rel = L.detachTrigger(rel, st.groups[0].id, "request");
    const after = L.applyWrites(rel, "transponder");
    eq("...and still eight after a handler is detached", after.length, 8);
    const freed = after.filter((e) => !e.configured);
    eq("the orphaned slots are now unconfigured",
       freed.map((e) => e.cmd).sort((a, b) => a - b), [0, 4, 5, 6, 7]);
    ok("...written as all-default, since there is no Disabled function",
       freed.every((e) => e.recvFn === 0 && e.sendFn === 0 && e.recvBits === 0
                       && e.sendHex === "" && e.eventAction === 0));
    ok("every slot appears exactly once",
       new Set(after.map((e) => e.cmd + ":" + e.event)).size === after.length);

    // The initiator half writes event 2 and must not touch the transponder's slots.
    const ini = L.applyWrites(L.defaults(), "initiator");
    ok("the initiator half writes only event 2", ini.every((e) => e.event === 2));
    eq("...for all eight slots regardless", ini.length, 8);
    eq("...configuring only the group that has a reply handler",
       ini.filter((e) => e.configured).map((e) => e.cmd), [1, 2, 3]);
}

// ── consistency rules ────────────────────────────────────────────────────────
console.log("consistency rules");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    const codes = (s) => L.issueCodes(s).map((i) => i.code);

    ok("unowned slots are reported", codes(st).includes("unownedSlots"));

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
}

// ── derived roles ────────────────────────────────────────────────────────────
console.log("derived roles");
{
    let st = L.addGroup(L.emptyState());
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

// ── persistence ──────────────────────────────────────────────────────────────
console.log("persistence");
{
    let st = L.addGroup(L.addNode(L.emptyState()));
    const g = st.groups[0].id;
    st = click(st, g, 7);
    st = L.attachTrigger(st, g, "request");
    st = L.markApplied(st, "transponder");

    const round = L.deserialize(L.serialize(st));
    eq("a round trip preserves the groups", j(round.groups), j(st.groups));
    eq("...the nodes", j(round.nodes), j(st.nodes));
    eq("...and what was applied, per role", j(round.applied), j(st.applied));
    ok("...so the applied half is still not stale", !L.isStale(round, "transponder"));

    eq("garbage yields defaults, not a broken plan",
       j(L.deserialize("}{ not json")), j(L.emptyState()));
    eq("an empty blob yields defaults", j(L.deserialize("")), j(L.emptyState()));

    // A blob claiming the same slot twice must be repaired, not trusted.
    const dup = JSON.stringify({ v: 3, nextId: 9, written: {},
        groups: [L.newGroup(1, [2, 2, 3]), L.newGroup(2, [3, 99, -1])], nodes: [] });
    const fixed = L.deserialize(dup);
    eq("duplicate slots are de-duplicated", fixed.groups[0].slots, [2, 3]);
    eq("...and the second group loses the stolen and out-of-range ones",
       fixed.groups[1].slots, []);
    const all = L.coverage(fixed).filter((c) => c.groupId >= 0).map((c) => c.cmd);
    ok("a repaired plan still has unique ownership", new Set(all).size === all.length);
}

// ── v4 blobs: a real saved plan must survive the role/release removal ────────
// This is the shape actually sitting in the operator's settings: one current role, one
// snapshot, and a `written` set. All three concepts are gone; the plan itself must not be.
console.log("v4 migration");
{
    const g1 = L.newGroup(2, [6]);
    g1.tr.request = L.newTrigger();
    const g2 = L.newGroup(3, [4, 5]);
    g2.tr.request = L.newTrigger();
    g2.tr.request.recv = { fmt: "bits", bits: 16 };
    g2.tr.request.send = { fmt: "bits", payload: "FF 01" };

    const applied = JSON.stringify([[g1.slots, g1.tr.request], [g2.slots, g2.tr.request]]);
    const v4 = JSON.stringify({
        v: 4, groups: [g1, g2], nodes: [{ id: 4, addr: 1, active: true, refs: [] }],
        nextId: 7, activeGroup: 0, role: "transponder",
        written: { "6:1": true, "4:1": true, "5:1": true },
        appliedOnce: true, appliedSnapshot: applied
    });

    const st = L.deserialize(v4);
    eq("the groups survive", st.groups.map((g) => g.slots), [[6], [4, 5]]);
    ok("the payload survives", st.groups[1].tr.request.send.payload === "FF 01");
    ok("the old current role becomes that role's applied record",
       L.appliedOnce(st, "transponder"));
    ok("...and is not stale, because nothing changed", !L.isStale(st, "transponder"));
    ok("the half that was never written stays unapplied",
       !L.appliedOnce(st, "initiator"));
    ok("`written` and `role` are dropped, not carried",
       !("written" in st) && !("role" in st));
    ok("...and a re-save does not resurrect them",
       L.serialize(st).indexOf("written") < 0 && L.serialize(st).indexOf('"role"') < 0);
    eq("the migrated plan still writes all eight slots",
       L.applyFrames(st, "transponder"), 10);
}

console.log("");
console.log(`${pass} passed, ${fails.length} failed`);
for (const f of fails) console.log("  FAILED: " + f);
process.exit(fails.length ? 1 : 0);

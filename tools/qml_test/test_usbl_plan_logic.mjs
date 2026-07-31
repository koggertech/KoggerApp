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
    const s3 = L.toggleSlot(s2, g, 4).state;
    ok("nested state is copied, not aliased", s3.groups[0] !== s2.groups[0]);
    ok("...so the old snapshot still reads the old slots",
       !L.hasSlot(L.groupById(s2, g), 4) && L.hasSlot(L.groupById(s3, g), 4));

    // A no-op must not fabricate a new identity: QML would re-render for nothing.
    ok("an impossible edit returns the SAME state", L.removeGroup(s2, 9999) === s2);
    ok("...and so does a step on a missing node", L.addStep(s2, 9999, g) === s2);
}

// ── slot ownership is exclusive ──────────────────────────────────────────────
console.log("slot ownership");
{
    let st = L.addGroup(L.emptyState());
    const g1 = st.groups[0].id;
    // A new group claims the first FREE slot so it is usable immediately -- assert that
    // rather than assuming an empty group.
    eq("a new group claims the first free slot", L.groupById(st, g1).slots, [0]);

    st = L.toggleSlot(st, g1, 1).state;
    eq("a group holds the slots given to it", L.groupById(st, g1).slots, [0, 1]);

    st = L.addGroup(st);
    const g2 = st.groups[1].id;
    eq("the next group claims the next free slot", L.groupById(st, g2).slots, [2]);
    for (const c of L.groupById(st, g2).slots.slice()) st = L.toggleSlot(st, g2, c).state;

    const r = L.toggleSlot(st, g2, 1);
    st = r.state;
    eq("taking a slot reports the group it came from", r.takenFrom, g1);
    eq("...removes it from that group", L.groupById(st, g1).slots, [0]);   // 1 was taken
    eq("...and gives it to the new one", L.groupById(st, g2).slots, [1]);

    st = L.toggleSlot(st, g2, 1).state;
    eq("toggling again releases it", L.groupById(st, g2).slots, []);
    eq("...and it returns to nobody, not to the old owner",
       L.coverage(st)[1].groupId, -1);

    // Every slot is owned by at most one group, always.
    st = L.toggleSlot(st, g1, 5).state;
    st = L.toggleSlot(st, g2, 5).state;
    const owners = L.coverage(st).filter((c) => c.groupId >= 0);
    ok("no slot is owned twice",
       new Set(owners.map((c) => c.cmd)).size === owners.length);
}

// ── the four views the UI binds to must agree ────────────────────────────────
// The operator saw coverage, the group chips, the slot buttons and the counter all
// disagreeing. They can only agree if each is derived from the same state.
console.log("derived views agree");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    // Drop the auto-claimed slot so this block controls the whole set.
    for (const c of L.groupById(st, g).slots.slice()) st = L.toggleSlot(st, g, c).state;
    st = L.toggleSlot(st, g, 3).state;
    st = L.toggleSlot(st, g, 4).state;
    st = L.toggleSlot(st, g, 5).state;

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
    for (const c of L.groupById(st, g).slots.slice()) st = L.toggleSlot(st, g, c).state;
    st = L.toggleSlot(st, g, 2).state;
    st = L.toggleSlot(st, g, 6).state;
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
    st = L.toggleSlot(st, g, 6).state;
    ok("steps are clamped to slots the group still owns",
       L.nodesView(st)[0].steps.every((s) => L.hasSlot(L.groupById(st, g), s.cmd)),
       j(L.nodesView(st)[0].steps));

    // Losing every slot drops the steps entirely.
    st = L.toggleSlot(st, g, 2).state;
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

// ── frame accounting ─────────────────────────────────────────────────────────
console.log("frame accounting");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    st = L.toggleSlot(st, g, 1).state;   // group now owns 2 slots (auto 0, plus 1)
    eq("initiator with no reply handler writes nothing", L.setupFrames(st), 0);

    st = L.attachTrigger(st, g, "reply");
    eq("one frame per slot once a reply handler exists",
       L.setupFrames(st), L.groupById(st, g).slots.length);

    let tr = L.setRole(st, "transponder");
    eq("transponder always writes the filter + enable pair on top",
       L.setupFrames(tr), 2);
    tr = L.attachTrigger(tr, g, "request");
    eq("...plus one per slot", L.setupFrames(tr), 2 + L.groupById(tr, g).slots.length);
}

// ── struct selection: the trap that cannot be seen on screen ─────────────────
console.log("struct selection");
{
    let st = L.attachTrigger(L.addGroup(L.emptyState()), 2, "request");
    const g = st.groups[0].id;
    st = L.attachTrigger(st, g, "request");
    let t = L.trigger(st, g, "request");
    eq("a bare trigger is a slot config", L.structOf(t), "USBLCmdSlotConfig");

    st = L.attachSection(st, g, "request", "recv");
    eq("one section still fits the slot config",
       L.structOf(L.trigger(st, g, "request")), "USBLCmdSlotConfig");

    st = L.attachSection(st, g, "request", "send");
    eq("two sections need the cmd config",
       L.structOf(L.trigger(st, g, "request")), "USBLCmdConfig");

    st = L.detachSection(st, g, "request", "send");
    eq("detaching drops back to the slot config",
       L.structOf(L.trigger(st, g, "request")), "USBLCmdSlotConfig");

    // A rewrite rule forces the wider struct even with a single section.
    st = L.setAdvField(st, g, "request", "cmdIdAction", "Replacement");
    eq("a rewrite rule forces the cmd config",
       L.structOf(L.trigger(st, g, "request")), "USBLCmdConfig");
    st = L.setAdvOpen(st, g, "request", false);
    eq("closing advanced resets the rules and the struct",
       L.structOf(L.trigger(st, g, "request")), "USBLCmdSlotConfig");
}

// ── release tracking: the device keeps what the host forgets ─────────────────
console.log("release tracking");
{
    let st = L.addGroup(L.emptyState());
    const g = st.groups[0].id;
    st = L.attachTrigger(st, g, "reply");
    eq("nothing to release before the first apply", L.releaseFrames(st), 0);

    st = L.markApplied(st);
    eq("applied plan is not stale", L.isStale(st), false);
    eq("...and has nothing to release", L.releaseFrames(st), 0);

    st = L.detachTrigger(st, g, "reply");
    ok("detaching a handler makes the plan stale", L.isStale(st));
    eq("...and queues the slot for switch-off", L.releaseFrames(st),
       L.groupById(st, g).slots.length);

    // The other role's writes must survive: releases are scoped to one event.
    let other = L.setRole(st, "transponder");
    eq("switching role does not release the other role's slots",
       L.releaseFrames(other), 0);
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

    st = L.setDisposition(st, g, "request", "silent");
    st = L.detachSection(st, g, "request", "recv");
    st = L.setSendField(st, g, "fn", "default");
    ok("an interrogator facing a silent transponder is critical",
       L.issueCodes(st).some((i) => i.code === "interrogatorVsSilent" && i.sev === "crit"));

    st = L.setSendField(st, g, "reply", 0);
    ok("a pinger expecting no reply clears that",
       !codes(st).includes("interrogatorVsSilent"));
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
    st = L.toggleSlot(st, g, 7).state;
    st = L.attachTrigger(st, g, "request");
    st = L.markApplied(L.setRole(st, "transponder"));

    const round = L.deserialize(L.serialize(st));
    eq("a round trip preserves the groups", j(round.groups), j(st.groups));
    eq("...the nodes", j(round.nodes), j(st.nodes));
    eq("...the role", round.role, st.role);
    eq("...and the write set", j(round.written), j(st.written));

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

console.log("");
console.log(`${pass} passed, ${fails.length} failed`);
for (const f of fails) console.log("  FAILED: " + f);
process.exit(fails.length ? 1 : 0);

// Behaviour tests for the acoustic-node interrogation cycle. No Qt, no window, no device.
//
//   node tools/qml_test/test_usbl_node_logic.mjs
//
// Exit 0 = all pass. Four things are being defended.
//
// First, the CYCLE. One interrogation is in flight at a time, so a window closes when the next
// request goes out or when its budget runs out -- and an unanswered window is a result, not a
// gap. Every way of getting that wrong either accuses a working beacon or exonerates a dead one,
// and nothing on the wire can be consulted to settle it: the verdict is entirely inference.
//
// Second, the GRANULARITY. The unit is a STEP, (node, cmd), not a node. Four commands on one node
// fail independently, and keying this by node made that unreadable -- the row went stale with no
// way to tell which command had gone unanswered. There is a section below for exactly that case.
//
// Third, the SINGLE-STEP path. One press of Step sends once and nothing follows it, so it is the
// case where the timeout is the only thing that can close the window. It regressed once already
// -- the operation axis keyed on "is the loop running", so a single Step never showed Waiting at
// all -- and the scenario below is that bug written down.
//
// Fourth, TIME INDEPENDENCE. A result badge must not change while nothing is being asked. Age is
// a separate control with a separate threshold.

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { readFileSync } from "node:fs";
import path from "node:path";

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));
const N = require(path.join(here, "..", "..", "qml", "app", "UsblNodeLogic.js"));
const F = require(path.join(here, "..", "..", "qml", "kqml_types", "UsblFieldLogic.js"));

let pass = 0;
const fails = [];
function ok(name, cond, detail) {
    if (cond) { pass++; console.log("  ok   " + name); }
    else { fails.push(name); console.log(`  FAIL ${name}${detail ? "   -- " + detail : ""}`); }
}
const j = (v) => JSON.stringify(v);
function eq(name, got, want) { ok(name, j(got) === j(want), `got ${j(got)}, want ${j(want)}`); }

const NOW = 1_000_000;
const STALE = F.STALE_MS;

function sol(addr, ageMs) {
    return {
        address: addr, distance: 10 + addr, azimuth: 90 + addr, elevation: -5,
        snr: 20 + addr, beaconLat: 59.9386, beaconLon: 30.3141, beaconDepth: 12.5,
        epochMs: NOW - ageMs, coordValid: true
    };
}
function node(id, addr, active) { return { id: id, addr: addr, active: active !== false }; }

// Two nodes, the shape a real schedule has: node 1 on address 1, node 2 on address 2.
const NODES = [node(1, 1), node(2, 2)];

// ── the answer window, per step ──────────────────────────────────────────────
console.log("the answer window");
{
    const p0 = N.initialPoll();
    ok("nothing is waiting to begin with", !N.isWaiting(p0, 1) && !N.isWaitingStep(p0, 1, 0));
    eq("...and nothing has resolved", N.stepResult(p0, 1, 0), "");

    const p1 = N.noteSent(p0, 1, 0, NOW);
    ok("interrogating (node 1, cmd 0) opens that step's window", N.isWaitingStep(p1, 1, 0));
    ok("...and the node reads as waiting", N.isWaiting(p1, 1));
    ok("...but its other commands do not", !N.isWaitingStep(p1, 1, 2));
    ok("...and neither does another node", !N.isWaiting(p1, 2));
    eq("...recording when", p1.sentAt, NOW);
    eq("...and accusing nothing yet", N.stepResult(p1, 1, 0), "");

    const p2 = N.noteReplyAddr(p1, NODES, 1);
    ok("its reply closes the window", !N.isWaitingStep(p2, 1, 0));
    eq("...and records the answer against that command", N.stepResult(p2, 1, 0), N.REPLIED);

    // Rule 1: the head is transmitting again, so a reply landing now cannot be attributed.
    const p3 = N.noteSent(N.noteSent(p0, 1, 0, NOW), 1, 2, NOW + 700);
    ok("moving on to cmd 2 opens cmd 2's window", N.isWaitingStep(p3, 1, 2));
    ok("...closes cmd 0's", !N.isWaitingStep(p3, 1, 0));
    eq("...and records cmd 0 as having missed", N.stepResult(p3, 1, 0), N.STALE);
    eq("...while cmd 2 has no verdict yet", N.stepResult(p3, 1, 2), "");

    // Across nodes, the same rule.
    const p4 = N.noteSent(N.noteSent(p0, 1, 0, NOW), 2, 0, NOW + 700);
    eq("moving on to another node also closes the previous step",
       N.stepResult(p4, 1, 0), N.STALE);
    ok("...and the new node is the one waiting", N.isWaiting(p4, 2));

    // Answered before its turn passed: no accusation.
    const p5 = N.noteSent(N.noteReplyAddr(N.noteSent(p0, 1, 0, NOW), NODES, 1), 1, 2, NOW + 700);
    eq("a step that answered before its turn passed keeps its answer",
       N.stepResult(p5, 1, 0), N.REPLIED);

    // The lone-step case. Nothing else ever takes its turn, so if re-interrogation did not count
    // as closing the window, a silent command would sit in Waiting forever.
    const solo = N.noteSent(N.noteSent(p0, 1, 0, NOW), 1, 0, NOW + 700);
    eq("re-interrogating a silent step records the miss", N.stepResult(solo, 1, 0), N.STALE);
    ok("...and reopens its window", N.isWaitingStep(solo, 1, 0));
    eq("...so the chip reads waiting, not stale, while it is out",
       N.stepCode(solo, 1, 0), N.WAITING);

    // Rule 2: one Step sends once, so the budget is the only thing that can close it.
    const timedOut = N.noteTimeout(N.noteSent(p0, 1, 3, NOW));
    ok("the budget running out closes the window", !N.isWaitingStep(timedOut, 1, 3));
    eq("...and records the miss against that command", N.stepResult(timedOut, 1, 3), N.STALE);
    eq("...leaving other commands untouched", N.stepResult(timedOut, 1, 0), "");

    const idleTimeout = N.noteTimeout(p0);
    ok("a timeout with nothing outstanding blames nothing", idleTimeout === p0);
}

// ── the step key ─────────────────────────────────────────────────────────────
console.log("step identity");
{
    ok("a step is identified by node and cmd together",
       N.stepKey(1, 2) !== N.stepKey(2, 1));
    ok("...and consistently", N.stepKey(1, 2) === N.stepKey(1, 2));
    // Node 1 cmd 12 vs node 11 cmd 2 -- a key built by concatenation without a separator
    // collides here, and the collision is silent.
    ok("keys cannot collide by concatenation", N.stepKey(1, 12) !== N.stepKey(11, 2));

    // addStep repeats a group's first slot once its slots run out, so one node can hold two
    // chips for the same cmd. They send identical bytes to the same address, so one verdict for
    // both is the truthful reading -- asserted so it stays a decision.
    const p = N.noteTimeout(N.noteSent(N.initialPoll(), 1, 0, NOW));
    eq("two chips for the same cmd on one node share one verdict",
       [N.stepCode(p, 1, 0), N.stepCode(p, 1, 0)], [N.STALE, N.STALE]);
}

// ── the poll state is replaced, never edited ─────────────────────────────────
console.log("immutability");
{
    const p0 = N.initialPoll();
    const p1 = N.noteSent(p0, 1, 0, NOW);
    ok("noteSent returns a new object", p1 !== p0);
    eq("...and leaves the old one alone", p0.waitKey, "");

    const p2 = N.noteSent(p1, 1, 2, NOW + 700);
    eq("the miss it records does not appear in the previous state", N.stepResult(p1, 1, 0), "");
    eq("...only in the new one", N.stepResult(p2, 1, 0), N.STALE);

    const p3 = N.noteReplyAddr(N.noteSent(p2, 1, 0, NOW + 1400), NODES, 1);
    eq("the old state still holds the miss", N.stepResult(p2, 1, 0), N.STALE);
    eq("...while the new one has the answer", N.stepResult(p3, 1, 0), N.REPLIED);
}

// ── replies are matched by ADDRESS, then attributed to the open step ─────────
console.log("attribution");
{
    const p0 = N.noteSent(N.initialPoll(), 1, 5, NOW);

    const other = N.noteReplyAddr(p0, NODES, 2);
    ok("a reply from a different address does not close the window",
       N.isWaitingStep(other, 1, 5));
    const unknown = N.noteReplyAddr(p0, NODES, 7);
    ok("a reply from an address no node holds changes nothing", unknown === p0);
    ok("...and a missing node list does not throw",
       N.noteReplyAddr(p0, null, 1) === p0);

    // Nothing outstanding: the solution still reaches the row's numbers through Dataset, but no
    // command can honestly be credited with it.
    const idle = N.noteReplyAddr(N.initialPoll(), NODES, 1);
    eq("a solution with no window open credits no command", idle.result, {});

    // Address 0 is a legal beacon. Any truthiness test on this path turns it into "no beacon".
    const zero = N.noteReplyAddr(N.noteSent(N.initialPoll(), 9, 0, NOW), [node(9, 0)], 0);
    eq("address 0 is matched", N.stepResult(zero, 9, 0), N.REPLIED);
    ok("...and its solution is found", N.entryFor({ "0": sol(0, 100) }, 0) !== null);

    // Two nodes on one address: the open window says which one it was, so the other is not
    // credited with an answer it never got.
    const shared = [node(1, 3), node(2, 3)];
    const both = N.noteReplyAddr(N.noteSent(N.initialPoll(), 2, 0, NOW), shared, 3);
    eq("only the node that was asked is credited", N.stepResult(both, 2, 0), N.REPLIED);
    eq("...not the other one on the same address", N.stepResult(both, 1, 0), "");
}

// ── what a command chip shows ────────────────────────────────────────────────
console.log("the command chips");
{
    let p = N.initialPoll();
    eq("never interrogated", N.stepCode(p, 1, 0), N.NONE);

    p = N.noteSent(p, 1, 0, NOW);
    eq("request out", N.stepCode(p, 1, 0), N.WAITING);
    eq("...and its neighbours are untouched", N.stepCode(p, 1, 1), N.NONE);

    p = N.noteReplyAddr(p, NODES, 1);
    eq("answered", N.stepCode(p, 1, 0), N.REPLIED);

    p = N.noteSent(p, 1, 0, NOW + 1000);
    eq("re-asked: waiting outranks the previous verdict", N.stepCode(p, 1, 0), N.WAITING);
    p = N.noteTimeout(p);
    eq("...and the budget running out turns it stale", N.stepCode(p, 1, 0), N.STALE);
    p = N.noteReplyAddr(N.noteSent(p, 1, 0, NOW + 2000), NODES, 1);
    eq("...then answering again heals it", N.stepCode(p, 1, 0), N.REPLIED);

    ok("the four chip states are distinct",
       new Set([N.WAITING, N.REPLIED, N.STALE, N.NONE]).size === 4);
}

// ── THE CASE THAT PROMPTED THIS: which command failed ───────────────────────
// One node, four commands, a full cycle in which only cmd 2 goes unanswered. Before the state
// was per-step the row went stale and nothing said which command it was.
console.log("scenario: one command out of four goes unanswered");
{
    const nodes = [node(1, 1)];
    const cmds = [0, 1, 2, 3];
    let poll = N.initialPoll();
    let t = NOW;

    for (const c of cmds) {
        poll = N.noteSent(poll, 1, c, t);
        if (c !== 2) poll = N.noteReplyAddr(poll, nodes, 1);   // cmd 2 never answers
        t += 700;
    }
    // The cycle moved on from cmd 2 when cmd 3 was sent, which is what recorded its miss.
    poll = N.noteTimeout(poll);

    eq("the chips name the failure exactly",
       cmds.map((c) => N.stepCode(poll, 1, c)),
       [N.REPLIED, N.REPLIED, N.STALE, N.REPLIED]);
    // cmd 3 was interrogated after cmd 2 and answered, so the NODE is answering -- the row says
    // so and the amber chip says which command is not.
    eq("the row follows the last exchange, not the worst",
       N.nodeReplyCode(poll, 1, sol(1, 100)), N.REPLIED);

    // Fix the beacon and run one more cycle: the row clears only once every command does.
    for (const c of cmds) {
        poll = N.noteReplyAddr(N.noteSent(poll, 1, c, t), nodes, 1);
        t += 700;
    }
    eq("every command answering clears every chip",
       cmds.map((c) => N.stepCode(poll, 1, c)),
       [N.REPLIED, N.REPLIED, N.REPLIED, N.REPLIED]);
    eq("...and the row stays Replied throughout", N.nodeReplyCode(poll, 1, sol(1, 100)), N.REPLIED);
}

// ── the row's badge: the LAST interrogation, not the worst one ──────────────
// Worst-case-over-all-commands was tried and is wrong twice over, and both failures are rules
// here. It calls a node stale whose last exchange succeeded -- which is not what the row is
// asked -- and it never clears, because a MUTED command that went stale before it was muted is
// never interrogated again and would condemn the row for the rest of the session.
console.log("the row badge");
{
    const p0 = N.initialPoll();

    eq("never interrogated and never heard from", N.nodeReplyCode(p0, 1, null), N.NONE);
    // Heard from without any interrogation of ours resolving: an unsolicited solution, or
    // another interrogator on the same address. True, and not a fault.
    eq("heard from, nothing of ours resolved",
       N.nodeReplyCode(p0, 1, sol(1, 100)), N.REPLIED);

    const oneOk = N.noteReplyAddr(N.noteSent(p0, 1, 0, NOW), NODES, 1);
    eq("its one interrogation answered", N.nodeReplyCode(oneOk, 1, null), N.REPLIED);

    const thenBad = N.noteTimeout(N.noteSent(oneOk, 1, 1, NOW + 700));
    eq("the next one missed, so the node reads stale",
       N.nodeReplyCode(thenBad, 1, null), N.STALE);

    // THE CORRECTION. cmd 1 is still stale as a command, and its chip still says so -- but the
    // node answered a moment ago, so the row must say Replied.
    const thenGood = N.noteReplyAddr(N.noteSent(thenBad, 1, 2, NOW + 1400), NODES, 1);
    eq("a later interrogation answering clears the row",
       N.nodeReplyCode(thenGood, 1, null), N.REPLIED);
    eq("...while the command that missed still says so",
       N.stepCode(thenGood, 1, 1), N.STALE);
    eq("...and the one that answered too", N.stepCode(thenGood, 1, 2), N.REPLIED);

    // The other half: a stale command that is then MUTED is never asked again, so nothing could
    // ever lift a worst-case verdict. The row follows the last real exchange instead.
    const muted = N.noteReplyAddr(N.noteSent(thenBad, 1, 0, NOW + 2000), NODES, 1);
    eq("a node whose stale command was muted still reads by its last exchange",
       N.nodeReplyCode(muted, 1, null), N.REPLIED);
    eq("...and the muted command keeps its own verdict for the chip",
       N.stepCode(muted, 1, 1), N.STALE);

    // Which command it was does not matter to the row -- only when.
    const otherNode = N.noteTimeout(N.noteSent(thenGood, 2, 0, NOW + 2100));
    eq("another node missing does not touch this one's badge",
       N.nodeReplyCode(otherNode, 1, null), N.REPLIED);
    eq("...and the other node reads its own", N.nodeReplyCode(otherNode, 2, null), N.STALE);

    ok("nodeReplyCode no longer needs the command list", N.nodeReplyCode.length === 3);
}

// ── interrogations asked for by hand ─────────────────────────────────────────
// The head cannot transmit while it is waiting, so a manual request takes its turn behind
// whatever window is open. Sending it straight away would abandon an answer still in the water
// and then blame the beacon for not sending it.
console.log("the manual queue");
{
    let q = [];
    ok("nothing is queued to begin with", !N.isQueued(q, 1, 0));
    eq("...and there is nothing to send", N.dequeueEmit(q).step, null);
    ok("dequeuing an empty queue does not throw", N.dequeueEmit(null).step === null);

    q = N.queueEmit(q, 1, 2);
    ok("asking for one queues it", N.isQueued(q, 1, 2));
    ok("...and only it", !N.isQueued(q, 1, 0) && !N.isQueued(q, 2, 2));

    // A double click is a double click, not an instruction to interrogate twice.
    const twice = N.queueEmit(q, 1, 2);
    eq("asking twice for the same step is one interrogation", twice.length, 1);

    q = N.queueEmit(q, 2, 0);
    eq("a different step queues separately", q.length, 2);

    // First asked, first sent: an operator who clicked three things expects them in that order.
    const first = N.dequeueEmit(q);
    eq("the first asked goes first", [first.step.nodeId, first.step.cmd], [1, 2]);
    eq("...and leaves the rest", first.queue.length, 1);
    const second = N.dequeueEmit(first.queue);
    eq("then the next", [second.step.nodeId, second.step.cmd], [2, 0]);
    eq("...and then nothing", N.dequeueEmit(second.queue).step, null);

    // Immutability, same rule as the poll state.
    ok("queueEmit returns a new array", N.queueEmit(q, 3, 3) !== q);
    eq("...and leaves the old one alone", q.length, 2);
    ok("dequeueEmit returns a new array", N.dequeueEmit(q).queue !== q);
    eq("...and does not shorten the old one", q.length, 2);
}

// ── the operation axis ───────────────────────────────────────────────────────
console.log("operation");
{
    eq("no window open", N.operationCode(true, false), N.IDLE);
    eq("window open", N.operationCode(true, true), N.WAITING);

    // A switch that visibly does nothing is worse than no switch.
    eq("switched off", N.operationCode(false, false), N.OFF);
    eq("switched off with a window somehow open -- still off",
       N.operationCode(false, true), N.OFF);

    ok("the three codes are distinct", new Set([N.OFF, N.IDLE, N.WAITING]).size === 3);
    // There is no Pinging state: emission is milliseconds and the protocol says nothing about
    // it, so the row pulses instead of claiming to observe the transmission.
    ok("nothing reports emission",
       [N.OFF, N.IDLE, N.WAITING].indexOf("pinging") < 0);
}

// ── results do not move with the clock ──────────────────────────────────────
console.log("time independence");
{
    // THE CORRECTION. An age-driven badge changes while the operator is not asking anything, and
    // there is no fact behind the change: "Replied" became "Stale" five seconds after the last
    // fix with the schedule stopped, announcing a beacon had gone quiet when nobody had spoken
    // to it. No function that produces a verdict takes a clock now, and this is what says so.
    ok("stepCode takes no time argument", N.stepCode.length === 3);
    ok("nodeReplyCode takes no time argument", N.nodeReplyCode.length === 3);

    const answered = N.noteReplyAddr(N.noteSent(N.initialPoll(), 1, 0, NOW), NODES, 1);
    let steady = true;
    for (const age of [0, 1000, STALE - 1, STALE, STALE + 1, 30000, 3600000]) {
        if (N.stepCode(answered, 1, 0) !== N.REPLIED) steady = false;
        if (N.nodeReplyCode(answered, 1, sol(1, age)) !== N.REPLIED) steady = false;
    }
    ok("an answered command stays Replied however old the fix gets", steady);

    const missed = N.noteTimeout(N.noteSent(N.initialPoll(), 1, 0, NOW));
    let steadyStale = true;
    for (const age of [0, 1000, STALE + 1, 3600000])
        if (N.nodeReplyCode(missed, 1, sol(1, age)) !== N.STALE) steadyStale = false;
    ok("...and a missed one stays Stale, rather than healing with time", steadyStale);
}

// ── age is a separate control ────────────────────────────────────────────────
console.log("the age chip");
{
    ok("a beacon that never answered is not aged -- there is no data to be old",
       !N.isAged(null, NOW, N.AGE_WARN_MS));
    ok("a fresh fix is not aged", !N.isAged(sol(1, 100), NOW, N.AGE_WARN_MS));
    ok("exactly at the threshold is not yet aged",
       !N.isAged(sol(1, N.AGE_WARN_MS), NOW, N.AGE_WARN_MS));
    ok("one millisecond past it is",
       N.isAged(sol(1, N.AGE_WARN_MS + 1), NOW, N.AGE_WARN_MS));
    ok("a long-gone fix is", N.isAged(sol(1, 600000), NOW, N.AGE_WARN_MS));
    ok("a missing threshold falls back to the default", N.isAged(sol(1, 600000), NOW, undefined));

    // The two indicators are orthogonal by construction: an aged fix can sit next to Replied
    // (nobody has asked lately) and a fresh one next to Stale (just asked, nothing came back).
    const missed = N.noteTimeout(N.noteSent(N.initialPoll(), 1, 0, NOW));
    ok("aged and Replied can hold at once",
       N.isAged(sol(1, 60000), NOW, N.AGE_WARN_MS)
       && N.nodeReplyCode(N.initialPoll(), 1, sol(1, 60000)) === N.REPLIED);
    ok("...and fresh with Stale",
       !N.isAged(sol(1, 100), NOW, N.AGE_WARN_MS)
       && N.nodeReplyCode(missed, 1, sol(1, 100)) === N.STALE);
}

// ── the cursor: where the cycle is ──────────────────────────────────────────
console.log("the cursor");
{
    const sched = [{ nodeId: 5, addr: 1, cmd: 0 }, { nodeId: 5, addr: 1, cmd: 2 },
                   { nodeId: 6, addr: 2, cmd: 0 }];

    eq("with nothing interrogated yet, the cursor is the next step's node",
       N.cursorNodeId(null, sched), 5);
    eq("once a step has run, it follows that step", N.cursorNodeId(sched[2], sched), 6);
    // The point of deriving it from the step rather than the poll state: a closed window
    // forgets, a position does not, so the frame stays where the cycle left it.
    eq("...and keeps following it after the answer window has closed",
       N.cursorNodeId(sched[2], sched), 6);

    eq("no schedule and nothing run: no cursor", N.cursorNodeId(null, []), -1);
    eq("a missing schedule does not throw", N.cursorNodeId(null, null), -1);
    eq("a malformed step falls back to the schedule", N.cursorNodeId({}, sched), 5);
    // A step for a node that has since been removed still reports that id: the row is gone, so
    // no frame is drawn, and the next advance moves the cursor. Inventing a fallback here would
    // move the frame somewhere the cycle is not.
    eq("a step naming a vanished node is reported as-is",
       N.cursorNodeId({ nodeId: 99 }, sched), 99);

    // The chip frame is the same idea one level down, and it must pick ONE chip -- two framed
    // chips on a row say the cycle is in two places.
    ok("the frame lands on the step that ran", N.isCursorStep(sched[1], sched, 5, 2));
    ok("...and on no other command of the same node", !N.isCursorStep(sched[1], sched, 5, 0));
    ok("...nor on the same command of another node", !N.isCursorStep(sched[1], sched, 6, 2));
    ok("before anything runs it lands on the first scheduled step",
       N.isCursorStep(null, sched, 5, 0) && !N.isCursorStep(null, sched, 5, 2));
    ok("with no schedule at all, nothing is framed", !N.isCursorStep(null, [], 5, 0));
    ok("...and a missing schedule does not throw", !N.isCursorStep(null, null, 5, 0));
}

// ── the budget ───────────────────────────────────────────────────────────────
console.log("the wait budget");
{
    eq("dwell plus grace", N.waitMs(700, 250), 950);
    eq("the default grace is used when none is given", N.waitMs(700), 700 + N.GRACE_MS);
    ok("the grace is smaller than a typical dwell -- it covers host latency, not a second "
       + "acoustic trip", N.GRACE_MS < 700);
    // A zero or nonsense dwell must not produce a window that closes instantly, which would
    // mark every step missed before any reply could arrive.
    eq("a zero dwell falls back", N.waitMs(0, 250), 950);
    eq("a missing dwell falls back", N.waitMs(undefined, 250), 950);
    eq("a negative grace falls back", N.waitMs(700, -5), 700 + N.GRACE_MS);
}

// ── one press of Step, start to finish ───────────────────────────────────────
// The regression that prompted the earlier rewrite: the loop is NOT running here, and every
// state below still has to be reached.
console.log("scenario: a single Step");
{
    const nodes = [node(1, 1)];
    const sched = [{ nodeId: 1, addr: 1, cmd: 0 }];
    let poll = N.initialPoll();
    let step = null;
    const row = (t, sols) => ({
        op: N.operationCode(nodes[0].active, N.isWaiting(poll, 1)),
        reply: N.nodeReplyCode(poll, 1, N.entryFor(sols, 1)),
        chip: N.stepCode(poll, 1, 0),
        framed: N.isCursorStep(step, sched, 1, 0),
        aged: N.isAged(N.entryFor(sols, 1), t, N.AGE_WARN_MS)
    });

    eq("before: idle, nothing heard, and already framed as the next step",
       row(NOW, {}), { op: N.IDLE, reply: N.NONE, chip: N.NONE, framed: true, aged: false });

    step = sched[0];
    poll = N.noteSent(poll, 1, 0, NOW);
    eq("Step pressed: waiting, on the row and on the chip",
       row(NOW + 10, {}),
       { op: N.WAITING, reply: N.NONE, chip: N.WAITING, framed: true, aged: false });

    // 133 ms round trip at 100 m.
    const answered = { "1": { epochMs: NOW + 133, distance: 100, azimuth: 12, snr: 24 } };
    poll = N.noteReplyAddr(poll, nodes, 1);
    eq("reply in: back to idle, it replied, and the frame stays",
       row(NOW + 140, answered),
       { op: N.IDLE, reply: N.REPLIED, chip: N.REPLIED, framed: true, aged: false });

    // Leave it alone for a minute. The BADGES must not budge; only the age chip may.
    eq("a minute later, untouched: still Replied, now aged",
       row(NOW + 60000, answered),
       { op: N.IDLE, reply: N.REPLIED, chip: N.REPLIED, framed: true, aged: true });

    // Ask again and get nothing.
    poll = N.noteSent(poll, 1, 0, NOW + 70000);
    eq("asked again: waiting, and the chip says so rather than flickering stale",
       row(NOW + 70010, answered),
       { op: N.WAITING, reply: N.REPLIED, chip: N.WAITING, framed: true, aged: true });

    poll = N.noteTimeout(poll);
    eq("budget out: idle, and now stale on both",
       row(NOW + 70960, answered),
       { op: N.IDLE, reply: N.STALE, chip: N.STALE, framed: true, aged: true });

    // And it recovers on the next answer rather than staying condemned.
    const again = { "1": { epochMs: NOW + 74000, distance: 101, azimuth: 12, snr: 23 } };
    poll = N.noteReplyAddr(N.noteSent(poll, 1, 0, NOW + 73900), nodes, 1);
    eq("answering again clears everything", row(NOW + 74010, again),
       { op: N.IDLE, reply: N.REPLIED, chip: N.REPLIED, framed: true, aged: false });
}

// ── the header count ─────────────────────────────────────────────────────────
console.log("summary");
{
    const sols = { "1": sol(1, 200), "2": sol(2, 60000) };
    const p0 = N.initialPoll();

    eq("no nodes at all", N.summary([], sols, p0),
       { total: 0, active: 0, replying: 0 });
    ok("a missing list does not throw", N.summary(null, sols, p0).total === 0);
    ok("a missing poll state does not throw", N.summary(NODES, sols, null).total === 2);

    // Outcome-based like the badges, so the header cannot disagree with the rows under it: node
    // 2's fix is a minute old, and with nobody having asked it anything that is not a failure.
    eq("both heard from, however old their fixes are", N.summary(NODES, sols, p0),
       { total: 2, active: 2, replying: 2 });

    // A miss drops it out of the count immediately -- that is the point of the count.
    eq("a node with a missed command stops counting as replying",
       N.summary(NODES, sols, N.noteTimeout(N.noteSent(p0, 1, 0, NOW))),
       { total: 2, active: 2, replying: 1 });

    // A node switched off on purpose must not read as a shortfall: it leaves the denominator as
    // well as the numerator.
    eq("a switched-off node is not counted as failing",
       N.summary([node(1, 1), node(2, 2, false)], sols, p0),
       { total: 2, active: 1, replying: 1 });
    eq("everything off", N.summary([node(1, 1, false)], sols, p0),
       { total: 1, active: 0, replying: 0 });

    eq("an address that never answered does not count",
       N.summary([node(1, 7)], sols, p0), { total: 1, active: 1, replying: 0 });
    eq("address 0 counts", N.summary([node(1, 0)], { "0": sol(0, 10) }, p0),
       { total: 1, active: 1, replying: 1 });
}

// ── age arithmetic ──────────────────────────────────────────────────────────
console.log("age");
{
    eq("no entry", N.ageMs(null, NOW), -1);
    eq("an entry with no timestamp counts as never", N.ageMs({ epochMs: 0 }, NOW), -1);
    eq("a fresh fix", N.ageMs(sol(1, 250), NOW), 250);
    // Host clock vs arrival order: a fix stamped in the future must read as brand new, not as a
    // negative age that formats as "-0.3 s ago".
    eq("a future timestamp clamps to zero", N.ageMs(sol(1, -500), NOW), 0);
}

// ── relationship to the widget module ───────────────────────────────────────
console.log("relationship to UsblFieldLogic");
{
    // These two modules answer DIFFERENT questions and are no longer expected to agree on a
    // verdict. A widget's `usblState` says whether a fix is usable, which is a fact about age; a
    // row's badge says what the last interrogations did, which is not. Asserting the old
    // agreement would now be asserting the bug back into place.
    const answered = N.noteReplyAddr(N.noteSent(N.initialPoll(), 4, 0, NOW), [node(4, 4)], 4);
    let independent = true;
    for (const a of [0, STALE - 1, STALE + 1, 600000])
        if (N.nodeReplyCode(answered, 4, sol(4, a)) !== N.REPLIED) independent = false;
    ok("the row badge is independent of the widgets' staleness rule", independent);
    ok("...and the widget's rule still moves with age, as it should",
       F.stateCode(sol(4, 100), NOW) === "tracking"
       && F.stateCode(sol(4, STALE + 1), NOW) === "stale");
    ok("the row warns later than a widget calls a fix unusable, and that is deliberate",
       N.AGE_WARN_MS > F.STALE_MS);

    // What they DO still have to agree on: the same beacon and the same arithmetic.
    let ageAgree = true;
    for (const a of [0, 1, 100, 999, STALE, STALE + 1, 30000, 600000])
        if (F.ageMs(sol(4, a), NOW) !== N.ageMs(sol(4, a), NOW)) ageAgree = false;
    ok("both measure age identically", ageAgree);

    const sols = {};
    for (let a = 0; a <= F.MAX_ADDR; ++a) sols[String(a)] = sol(a, 100);
    let pickAgree = true;
    for (let a = 0; a <= F.MAX_ADDR; ++a)
        if (j(F.pick(sols, a)) !== j(N.entryFor(sols, a))) pickAgree = false;
    ok("entryFor agrees with pick for every legal address", pickAgree);
    ok("...including the ones that are absent",
       N.entryFor({}, 3) === null && F.pick({}, 3) === null);
}

// ── the QML says every code, and says nothing else ───────────────────────────
// Source assertions, not behaviour: a code the row cannot name renders blank, and a word left
// behind after a rename is dead weight nobody notices. Both are invisible on screen.
console.log("the row's vocabulary and wiring");
{
    const src = readFileSync(path.join(here, "..", "..", "qml", "app", "UsblGroup.qml"), "utf8");

    const table = (name) => {
        const at = src.indexOf(`property var ${name}: ({`);
        ok(`${name} exists`, at >= 0);
        return at < 0 ? "" : src.slice(at, src.indexOf("})", at));
    };
    const keys = (name) => [...table(name).matchAll(/"([a-z]+)":/g)].map((m) => m[1]).sort();

    eq("every operation code has a word, and no word is orphaned",
       keys("_opText"), [N.IDLE, N.OFF, N.WAITING].sort());
    eq("every reply code has a word, and no word is orphaned",
       keys("_replyText"), [N.NONE, N.REPLIED, N.STALE].sort());

    ok("the row reads its state through the logic module",
       /import "UsblNodeLogic\.js" as Node/.test(src));

    // Every transition the reducer defines has to actually be driven from the UI, or the state
    // machine is correct and unreachable -- which is exactly how the single-Step bug shipped.
    ok("sending a request opens a window for that STEP",
       /Node\.noteSent\(\s*_poll,\s*s\.nodeId,\s*s\.cmd,/.test(src));
    ok("the budget running out closes it",   /Node\.noteTimeout\(/.test(src));
    ok("an arriving solution closes it",     /Node\.noteReplyAddr\(/.test(src));
    ok("...driven by the per-solution signal", /onLastUsblSolutionChanged/.test(src));
    ok("the window is timed by dwell plus grace", /Node\.waitMs\(/.test(src));

    // The operation axis must NOT be a function of whether the loop is running: one Step opens a
    // window with the loop stopped, and that is the case that regressed.
    ok("the operation axis does not consult the run timer",
       !/operationCode\([^)]*_runTimer/.test(src));
    // Nor may any verdict consult the clock.
    ok("no verdict is computed from the clock",
       !/nodeReplyCode\([^)]*_nowMs/.test(src) && !/stepCode\([^)]*_nowMs/.test(src));

    // Item 4 of the rework: the readout moved into the rows, so the standalone stat grid and the
    // global band are gone rather than merely hidden.
    ok("the global status band is gone", !/NO SOLUTION/.test(src));
    ok("the standalone stat grid is gone", !/component StatCell/.test(src));
    ok("the schedule strip is gone", !/Flickable/.test(src));

    // TWO MARKS ON THE ROW, TWO FACTS. The frame follows the cursor so it survives the window
    // closing; the background pulses only while a request is out.
    ok("the row frame follows the cursor, not the open window",
       /border\.color:\s*nodeCard\._isCursor/.test(src)
       && !/border\.color:\s*nodeCard\._waiting/.test(src));
    ok("...and a plain light-blue background follows the open window",
       /color:\s*nodeCard\._waiting[\s\S]{0,160}AppPalette\.accent\.r/.test(src));
    // NOTHING ON THIS PANE ANIMATES ON A LOOP. Two versions of the waiting mark did -- a fading
    // opacity, which took the frame with it, then an alternating fill -- and both were noise
    // against a sub-second window. A colour transition when the state changes is not that.
    ok("nothing fades, so no frame can vanish with it",
       !/Behavior on opacity/.test(src) && !/NumberAnimation[^}]*opacity/.test(src));
    ok("...and nothing blinks: no repeating timer drives an appearance",
       !/_pulse/.test(src) && !/loops:\s*Animation\.Infinite/.test(src));
    ok("age is its own control, highlighted past the warning threshold",
       /aged:\s*nodeCard\._aged/.test(src) && /Node\.AGE_WARN_MS/.test(src));

    // THREE MARKS ON A CHIP. Frame for position, fill for in-flight, fill for the result -- and
    // the frame must not be something the result can overwrite.
    ok("a chip knows its own step's state", /Node\.stepCode\(/.test(src));
    ok("...is framed when the cycle is on it", /Node\.isCursorStep\(/.test(src));
    ok("...and the frame outranks the result's outline",
       /border\.color:\s*_cur\s*\?\s*AppPalette\.accentBorder/.test(src));
    ok("the row badge aggregates its commands rather than guessing",
       /Node\.nodeReplyCode\(/.test(src));

    // Compact for scanning, extended for diagnosing -- and the two views must not diverge on
    // what a state means, which is why both read the same stepCode.
    ok("rows expand per node", /_toggleExpanded/.test(src) && /DisclosureIndicator/.test(src));
    ok("the compact view hides the extended one and vice versa",
       /visible:\s*!nodeCard\._open/.test(src) && /visible:\s*nodeCard\._open/.test(src));
    ok("a compact chip fires its command", /requestEmit\(nodeCard\._n\.id,\s*modelData\.cmd\)/.test(src));
    ok("...and no longer deletes it -- delete moved to the extended row",
       /Remove this command from the node/.test(src));
    ok("a compact chip shows whether the schedule includes it",
       /modelData\.on\s*\?/.test(src));
    ok("the extended row mutes a command without losing it",
       /plan\.toggleStep\(/.test(src));
    ok("the manual queue is drained, not just filled",
       /_drainManual\(\)/.test(src) && /Node\.dequeueEmit\(/.test(src));
    ok("a queued interrogation says so rather than looking dead",
       /_queued/.test(src) && /queued —/.test(src));
    // The collapsed header is the same fact from further away, so it must not have its own,
    // coarser vocabulary. It was a single boolean once, which made "gone stale" and "never heard
    // from" identical there and never showed a request in flight.
    ok("the header chips carry the full result state, not a boolean",
       /addrChip\._bad/.test(src) && /addrChip\._out/.test(src)
       && !/_live/.test(src));
    ok("...on the command chips' precedence: waiting outranks the last verdict",
       /Node\.isWaiting\(usblGroup\._poll, addrChip\.modelData\.id\)[\s\S]{0,80}"waiting"/.test(src));

    ok("received data is read only for the command the retained frame names",
       /_modemMatches\(/.test(src));
    ok("...and the pane says only the newest one is kept",
       /only the newest received payload is kept/.test(src));

    ok("the copy affordance routes through one seam", /function _copyCoordinate/.test(src));
    ok("...and says it is not wired up yet",
       /not wired up|no backend yet|backend later/i.test(src));
}

console.log("");
console.log(`${pass} passed, ${fails.length} failed`);
for (const f of fails) console.log("  FAILED: " + f);
process.exit(fails.length ? 1 : 0);

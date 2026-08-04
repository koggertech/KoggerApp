// Node row state: the interrogation cycle, what a row and its command chips report, and the
// count the group header carries. No QML types, no qsTr -- codes out, sentences in UsblGroup.qml.
//
// THE PHYSICS THIS ENCODES
//
// One acoustic interrogation is in flight at a time. The head transmits, the interrogation
// travels R/c (c ~ 1500 m/s), the remote node turns around after a fixed delay and replies, and
// the reply travels R/c back. So an answer is due after roughly
//
//     2R/c + turn-around
//
// -- 133 ms at 100 m, 1.3 s at a kilometre -- and until it arrives or its window closes, that
// interrogation and no other is the one being waited on.
//
// THE UNIT IS A STEP, NOT A NODE. A node with four command steps is interrogated four times per
// cycle, once per cmd, and they can fail independently: cmd 0 answers, cmd 2 does not. Keying
// this state by node made that unreadable -- the row went stale and there was no way to tell
// which command had gone unanswered. Everything below is keyed by (node, cmd), and the row's own
// badge is an aggregate over its steps.
//
// Three rules, and they are the whole module:
//
//   1. Interrogating anything else ends the previous step's window. The head is transmitting
//      again, so a reply landing now cannot be attributed -- an unanswered window is a MISS.
//      This applies to re-interrogating the SAME step too, or a lone silent one would sit in
//      Waiting forever because nothing else ever took its turn.
//   2. A window also closes on time, because nothing need follow it -- one Step sends once. The
//      budget is the operator's dwell plus a small allowance so a reply landing right at the
//      edge still counts.
//   3. A miss is a RESULT, not the absence of one. "I asked and heard nothing" is knowledge, and
//      it is knowledge immediately -- it must not wait for a fix to age out.
//
// WHICH COMMAND A REPLY ANSWERS IS INFERRED FROM WHICH REQUEST WAS OUTSTANDING, not from the
// frame. A solution carries a cmd_id on the wire, but Dataset's per-address cache does not keep
// it, and it does not need to: only one interrogation is in flight, so the open window names the
// command. A solution arriving with NO window open cannot be attributed to a command at all --
// it still refreshes the node's numbers, and it leaves every chip's verdict alone.
//
// NOTHING ON THE WIRE REPORTS A TIMEOUT. The driver's ping calls are fire-and-forget and the only
// inbound is a solution, so every "missed" verdict here is the host's own inference -- which is
// why the transitions are a reducer with tests rather than conditions scattered across bindings.
//
// THE REPLY AXIS DOES NOT MOVE WITH TIME. It reports the OUTCOME of the last interrogation and
// nothing else, so a row cannot change state while nothing is being asked of it. An earlier
// version folded age in, and Replied flipped to Stale five seconds after the last fix even with
// the schedule stopped -- announcing "this beacon has gone quiet" when the truth was "nobody
// asked it anything".
//
// Age is a separate fact, so it gets a separate indicator: `isAged` drives the row's age chip,
// which highlights past AGE_WARN_MS. Two questions, two controls, neither able to lie about the
// other. This does leave the row on a different threshold from UsblFieldLogic.STALE_MS, which
// decides whether a data WIDGET calls a fix usable -- a different question about the same number.
// See docs/KoggerApp-Docs/usbl-protocol.md.

// Operation -- whether an answer window is open.
var OFF     = "off";        // switched off: the schedule skips it, nothing is being asked
var IDLE    = "idle";       // no window open
var WAITING = "waiting";    // interrogated, answer not yet in and not yet given up on

// Result -- what the last interrogation did.
var REPLIED = "replied";
var STALE   = "stale";
var NONE    = "none";       // never resolved; an em dash, not a fault

// How long past the dwell a reply still counts. It covers host-side latency -- link, parse,
// Dataset update, binding -- not another acoustic trip: when the schedule is running the next
// interrogation closes the window anyway (rule 1), so this only decides the single-Step case and
// the last step before Stop.
var GRACE_MS = 250;

// When the age chip starts asking to be looked at. Not a state change and not a fault: the
// numbers on the row are simply this old, and past this you should know it without doing
// arithmetic on "14:32:07".
var AGE_WARN_MS = 10000;

function waitMs(dwellMs, graceMs) {
    var d = (typeof dwellMs === "number" && dwellMs > 0) ? dwellMs : 700;
    var g = (typeof graceMs === "number" && graceMs >= 0) ? graceMs : GRACE_MS;
    return d + g;
}

// A step's identity.
//
// (node, cmd) rather than (node, position in the list): two chips for the SAME cmd on one node --
// which addStep produces once a group's slots run out -- send identical bytes to the same
// address, so one verdict for both is the truthful reading, not a limitation.
function stepKey(nodeId, cmd) { return String(nodeId) + ":" + String(cmd); }

// Which step's window is open, when it opened, and the last verdict for every step that has ever
// resolved. Replaced wholesale on every transition, never edited in place -- same rule as the
// plan state, and for the same reason.
// `result` is per step and `last` is per node: the verdict of whichever of a node's steps
// resolved most recently. Both are needed and neither is derivable from the other -- `result`
// has no ordering, and a per-node verdict cannot say which command it came from.
function initialPoll() {
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0, result: {}, last: {} };
}

function _copyResult(r) {
    var out = {};
    for (var k in (r || {})) if (r[k]) out[k] = r[k];
    return out;
}

// A request just went out to (nodeId, cmd).
//
// Any window still open at this moment closed unresolved -- a reply would have cleared it via
// noteReplyAddr -- so it is a miss. The NEW step's own previous verdict is deliberately left
// alone: it stands until this window resolves, so a chip does not flicker to Stale every time it
// is re-asked.
function noteSent(poll, nodeId, cmd, nowMs) {
    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    if (poll.waitKey) {
        result[poll.waitKey] = STALE;
        last[poll.waitNodeId] = STALE;
    }
    return { waitKey: stepKey(nodeId, cmd), waitNodeId: nodeId, waitCmd: cmd,
             sentAt: nowMs, result: result, last: last };
}

// The window closed with nothing in it.
function noteTimeout(poll) {
    if (!poll.waitKey) return poll;
    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    result[poll.waitKey] = STALE;
    last[poll.waitNodeId] = STALE;
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0,
             result: result, last: last };
}

// A solution arrived for `addr`. Addresses, not node ids, because that is what a solution
// carries; the open window then says which node and which command it answers.
//
// A solution for some other address, or one arriving with no window open, resolves nothing: there
// is no honest way to say which command it belongs to. It still reaches the row's numbers, which
// come from Dataset directly.
function noteReplyAddr(poll, nodes, addr) {
    if (!poll.waitKey) return poll;
    var open = null;
    for (var i = 0; i < (nodes ? nodes.length : 0); ++i)
        if (nodes[i].id === poll.waitNodeId) { open = nodes[i]; break; }
    if (!open || open.addr !== addr) return poll;

    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    result[poll.waitKey] = REPLIED;
    last[poll.waitNodeId] = REPLIED;
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0,
             result: result, last: last };
}

// ── interrogations asked for by hand ─────────────────────────────────────────
//
// A queue and not a flag, because the operator can ask for several before any of them goes out.
// It exists at all because THE HEAD CANNOT TRANSMIT WHILE IT IS WAITING: a manual request has to
// take its turn behind whatever window is open, exactly as a scheduled one does. Sending it
// immediately would abandon an answer that is still in the water and blame the beacon for it.
//
// Ordering is first-asked, first-sent. Asking twice for the same step is one interrogation, not
// two -- a double click is a double click, not an instruction.
function queueEmit(q, nodeId, cmd) {
    var out = (q || []).slice();
    for (var i = 0; i < out.length; ++i)
        if (out[i].nodeId === nodeId && out[i].cmd === cmd) return out;
    out.push({ nodeId: nodeId, cmd: cmd });
    return out;
}
// Returns { queue, step } with step null when there was nothing to send.
function dequeueEmit(q) {
    var out = (q || []).slice();
    if (!out.length) return { queue: out, step: null };
    var head = out.shift();
    return { queue: out, step: head };
}
function isQueued(q, nodeId, cmd) {
    for (var i = 0; i < (q ? q.length : 0); ++i)
        if (q[i].nodeId === nodeId && q[i].cmd === cmd) return true;
    return false;
}

// ── reads ────────────────────────────────────────────────────────────────────
function isWaitingStep(poll, nodeId, cmd) {
    return !!poll && poll.waitKey === stepKey(nodeId, cmd);
}
// Node level: a row is waiting when any of its steps is. Only one can be, globally.
function isWaiting(poll, nodeId) { return !!poll && poll.waitNodeId === nodeId; }

function stepResult(poll, nodeId, cmd) {
    if (!poll || !poll.result) return "";
    return poll.result[stepKey(nodeId, cmd)] || "";
}

// What one command chip shows. Waiting outranks the previous verdict, which is why a chip reads
// "in progress" rather than "stale" the moment it is re-asked.
function stepCode(poll, nodeId, cmd) {
    if (isWaitingStep(poll, nodeId, cmd)) return WAITING;
    var r = stepResult(poll, nodeId, cmd);
    return r ? r : NONE;
}

// OFF wins: a row whose switch is off must not report activity, or the switch looks ignored.
function operationCode(active, waiting) {
    if (!active) return OFF;
    return waiting ? WAITING : IDLE;
}

// The row's badge: the outcome of this node's MOST RECENT interrogation, whichever command that
// was. The chips carry the per-command detail.
//
// Worst-case-over-all-commands was tried and is wrong twice over. It says "stale" about a node
// whose last exchange succeeded, which is not what the operator is asking the row -- the row asks
// "is this node answering me", and the latest exchange is the answer. And it never clears: a MUTED
// command that went stale before it was muted is never interrogated again, so it would condemn the
// row for the rest of the session with no way to lift it.
//
// Falling back to the entry covers a node that has been heard from without any interrogation of
// ours resolving -- an unsolicited solution, or another interrogator on the same address.
function nodeReplyCode(poll, nodeId, entry) {
    var r = (poll && poll.last) ? poll.last[nodeId] : null;
    if (r) return r;
    return (entry && entry.epochMs) ? REPLIED : NONE;
}

// Whether the row's age chip should ask to be noticed. Never-answered is NOT aged -- there is no
// data to be old, and the reply badge already says so.
function isAged(entry, nowMs, warnMs) {
    var ms = ageMs(entry, nowMs);
    if (ms < 0) return false;
    return ms > ((typeof warnMs === "number" && warnMs >= 0) ? warnMs : AGE_WARN_MS);
}

// The row the cycle is ON: the last node interrogated, or -- before anything has been asked --
// the one the next Step would interrogate. It keeps its frame after the answer window closes,
// which is the whole point: with the cycle strip gone, this is the only thing that says where in
// the schedule you are.
//
// Derived from the CURRENT STEP rather than from the poll state, because the poll state forgets
// (a window closes) and the position does not.
function cursorNodeId(curStep, schedule) {
    if (curStep && typeof curStep.nodeId === "number") return curStep.nodeId;
    if (schedule && schedule.length) return schedule[0].nodeId;
    return -1;
}
// The same, for one command chip: which chip carries the frame.
function isCursorStep(curStep, schedule, nodeId, cmd) {
    var s = (curStep && typeof curStep.nodeId === "number") ? curStep
          : ((schedule && schedule.length) ? schedule[0] : null);
    if (!s) return false;
    return s.nodeId === nodeId && s.cmd === cmd;
}

function entryFor(solutions, addr) {
    var s = solutions || {};
    var e = s[String(addr)];
    return e ? e : null;
}

// -1 means "never answered", which is a different thing from "answered a long time ago" and has
// to stay distinguishable from it all the way to the badge.
function ageMs(entry, nowMs) {
    if (!entry || !entry.epochMs) return -1;
    return Math.max(0, nowMs - entry.epochMs);
}

// What the group header reports whether it is open or collapsed. Outcome-based like the badges,
// so the count cannot drift away from the rows it is counting -- with the schedule stopped it
// describes the last cycle, which is the only thing there is to describe.
//
// Only ACTIVE nodes are counted. A switched-off node is not expected to answer, so counting it
// would turn a deliberate choice into a shortfall.
function summary(nodes, solutions, poll) {
    var total = 0, active = 0, replying = 0;
    for (var i = 0; i < (nodes ? nodes.length : 0); ++i) {
        var n = nodes[i];
        ++total;
        if (!n.active) continue;
        ++active;
        if (nodeReplyCode(poll, n.id, entryFor(solutions, n.addr)) === REPLIED) ++replying;
    }
    return { total: total, active: active, replying: replying };
}

// Node consumes this via module.exports; QML sees the top-level functions and ignores it.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        OFF: OFF, IDLE: IDLE, WAITING: WAITING,
        REPLIED: REPLIED, STALE: STALE, NONE: NONE,
        GRACE_MS: GRACE_MS, AGE_WARN_MS: AGE_WARN_MS,
        waitMs: waitMs, stepKey: stepKey, initialPoll: initialPoll,
        queueEmit: queueEmit, dequeueEmit: dequeueEmit, isQueued: isQueued,
        noteSent: noteSent, noteTimeout: noteTimeout, noteReplyAddr: noteReplyAddr,
        isWaiting: isWaiting, isWaitingStep: isWaitingStep,
        stepResult: stepResult, stepCode: stepCode,
        operationCode: operationCode, nodeReplyCode: nodeReplyCode,
        isAged: isAged, cursorNodeId: cursorNodeId, isCursorStep: isCursorStep,
        entryFor: entryFor, ageMs: ageMs, summary: summary
    };
}

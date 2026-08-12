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

// How many interrogations a node has to miss IN A ROW before it stops being a dropped ping and
// starts being a beacon that is gone.
//
// COUNTED, NOT TIMED, and that is the whole point. The first version escalated on the age of the
// last FIX, which cannot work: Dataset caches a solution per address on arrival whether or not a
// window was open to attribute it to, so a reply that lands after its window closed still
// refreshes the age. A node answering just too late to be counted therefore missed every
// interrogation while its age reset every time -- permanently MISSED and never LOST.
//
// A count also fixes the other half. It advances only when we ASK, so a stopped schedule cannot
// escalate a row on its own, and the threshold means the same thing at any dwell: three refusals,
// not "long enough that somebody should have answered".
var LOST_MISSES = 3;

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
//
// `asked` is per node as well, and it is a DIFFERENT fact from `last`: which command was most
// recently SENT to that node, resolved or not. The two name different commands for as long as a
// request is out -- `last` still carries the previous outcome while `asked` names what is in
// flight -- and both readings are true at once. A row's reply badge wants the first; a chip
// saying which command is being asked wants the second.
//
// It is written in ONE place, the send, and its state is then `stepCode` of it. That is what
// makes "waiting outranks the previous verdict" fall out of the existing rules instead of being
// re-implemented for the chip.
// `misses` is the consecutive-miss streak per node: incremented wherever a window closes
// unresolved, reset to zero by an answer. It is the escalation to LOST, and it is a COUNT rather
// than a clock -- see LOST_MISSES.
function initialPoll() {
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0,
             result: {}, last: {}, asked: {}, misses: {}, repliedAt: {} };
}

function _copyResult(r) {
    var out = {};
    for (var k in (r || {})) if (r[k]) out[k] = r[k];
    return out;
}

// NOT _copyResult: these values are cmd NUMBERS and cmd 0 is a real command -- the implicit step
// a node with no refs contributes, which is the commonest one there is. A truthiness filter drops
// it and the chip then reports "never asked" about the default case.
function _copyAsked(a) {
    var out = {};
    for (var k in (a || {})) if (typeof a[k] === "number") out[k] = a[k];
    return out;
}
// Same rule as _copyAsked and for the same reason: a streak of 0 is a real value -- it is what
// "this node is answering" looks like -- and a truthiness filter would drop it.
function _copyMisses(m) {
    var out = {};
    for (var k in (m || {})) if (typeof m[k] === "number") out[k] = m[k];
    return out;
}
function _bumpMiss(misses, nodeId) {
    var n = (typeof misses[nodeId] === "number") ? misses[nodeId] : 0;
    misses[nodeId] = n + 1;
}

// A request just went out to (nodeId, cmd).
//
// Any window still open at this moment closed unresolved -- a reply would have cleared it via
// noteReplyAddr -- so it is a miss. The NEW step's own previous verdict is deliberately left
// alone: it stands until this window resolves, so a chip does not flicker to Stale every time it
// is re-asked.
function noteSent(poll, nodeId, cmd, nowMs) {
    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    var asked = _copyAsked(poll.asked), misses = _copyMisses(poll.misses);
    if (poll.waitKey) {
        result[poll.waitKey] = STALE;
        last[poll.waitNodeId] = STALE;
        _bumpMiss(misses, poll.waitNodeId);
    }
    asked[nodeId] = cmd;
    return { waitKey: stepKey(nodeId, cmd), waitNodeId: nodeId, waitCmd: cmd,
             sentAt: nowMs, result: result, last: last, asked: asked, misses: misses,
             repliedAt: _copyMisses(poll.repliedAt) };
}

// The window closed with nothing in it.
function noteTimeout(poll) {
    if (!poll.waitKey) return poll;
    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    var misses = _copyMisses(poll.misses);
    result[poll.waitKey] = STALE;
    last[poll.waitNodeId] = STALE;
    _bumpMiss(misses, poll.waitNodeId);
    // `asked` is carried, not cleared: which command was last asked does not stop being true
    // because it went unanswered -- that IS the thing the chip has to report.
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0,
             result: result, last: last, asked: _copyAsked(poll.asked), misses: misses,
             repliedAt: _copyMisses(poll.repliedAt) };
}

// A solution arrived for `addr`. Addresses, not node ids, because that is what a solution
// carries; the open window then says which node and which command it answers.
//
// A solution for some other address, or one arriving with no window open, resolves nothing: there
// is no honest way to say which command it belongs to. It still reaches the row's numbers, which
// come from Dataset directly.
function noteReplyAddr(poll, nodes, addr, nowMs) {
    if (!poll.waitKey) return poll;
    var open = null;
    for (var i = 0; i < (nodes ? nodes.length : 0); ++i)
        if (nodes[i].id === poll.waitNodeId) { open = nodes[i]; break; }
    if (!open || open.addr !== addr) return poll;

    var result = _copyResult(poll.result), last = _copyResult(poll.last);
    var misses = _copyMisses(poll.misses), repliedAt = _copyMisses(poll.repliedAt);
    result[poll.waitKey] = REPLIED;
    last[poll.waitNodeId] = REPLIED;
    // An answer clears the streak outright. One good exchange is the end of a fault, not a
    // decrement of it -- a node that answers is answering.
    misses[poll.waitNodeId] = 0;
    // WHEN this node last answered US. Stamped here and nowhere else, because this is the only
    // place a reply is attributed to a node -- see replyAgeMs.
    if (typeof nowMs === "number") repliedAt[poll.waitNodeId] = nowMs;
    return { waitKey: "", waitNodeId: -1, waitCmd: -1, sentAt: 0,
             result: result, last: last, asked: _copyAsked(poll.asked),
             misses: misses, repliedAt: repliedAt };
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

// How many interrogations this node has missed in a row. Zero means the last one answered.
function missStreak(poll, nodeId) {
    var m = (poll && poll.misses) ? poll.misses[nodeId] : undefined;
    return (typeof m === "number") ? m : 0;
}

// Whether a node has stopped being a dropped ping and started being a beacon that is gone.
// Takes no clock, deliberately: a stopped schedule must not be able to escalate a row, and the
// age of the last fix is not evidence about this at all (see LOST_MISSES).
function isLost(poll, nodeId, threshold) {
    var t = (typeof threshold === "number" && threshold > 0) ? threshold : LOST_MISSES;
    return missStreak(poll, nodeId) >= t;
}

// Which command was last SENT to this node, -1 if none ever was. Its state is stepCode of it, so
// the chip reads Waiting while the request is out and the verdict once the window resolves --
// the same precedence every other mark on the pane follows.
//
// -1 rather than 0 for "never", because cmd 0 is a real command.
function lastAskedCmd(poll, nodeId) {
    var a = (poll && poll.asked) ? poll.asked[nodeId] : undefined;
    return (typeof a === "number") ? a : -1;
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
//
// This is the age of the FIX -- when a solution for this address last arrived. It is what a data
// widget wants (is this number usable), and it is NOT what a node row wants; see replyAgeMs.
function ageMs(entry, nowMs) {
    if (!entry || !entry.epochMs) return -1;
    return Math.max(0, nowMs - entry.epochMs);
}

// HOW LONG SINCE THIS NODE ANSWERED US -- which is not the same as how old its numbers are, and
// the row wants the first.
//
// Dataset stamps a solution's arrival per ADDRESS, unconditionally: a reply that lands after its
// window closed is scored a miss and still refreshes that stamp. So a node answering just too
// late for the dwell showed a miss beside an age that reset on every one of them. The age was
// telling the truth about the wrong question.
//
// Stamped instead where a reply is ATTRIBUTED, in noteReplyAddr. Falling back to the fix covers
// the node that has been heard from without any interrogation of ours resolving -- an unsolicited
// solution, or another interrogator on the same address -- which is the same fallback
// nodeReplyCode makes, and for the same reason.
function replyAgeMs(poll, nodeId, entry, nowMs) {
    var t = (poll && poll.repliedAt) ? poll.repliedAt[nodeId] : undefined;
    if (typeof t === "number") return Math.max(0, nowMs - t);
    return ageMs(entry, nowMs);
}

// Whether the row's age chip should ask to be noticed, on the same number the row shows.
function isReplyAged(poll, nodeId, entry, nowMs, warnMs) {
    var ms = replyAgeMs(poll, nodeId, entry, nowMs);
    if (ms < 0) return false;
    return ms > ((typeof warnMs === "number" && warnMs >= 0) ? warnMs : AGE_WARN_MS);
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

// ── one row, composed once ───────────────────────────────────────────────────
//
// The pane and the on-scene panel draw the same node from the same four marks, so the
// composition lives here rather than in either of them. Two surfaces computing "what is node 2
// doing" separately is two chances to answer differently on one screen, and the wrong one is
// invisible until an operator trusts it.
//
// The row is a NODE, not a step. Distance and SNR are cached per address (Dataset::usblSolutions),
// so per-step rows would repeat one node's numbers down its commands; the per-command detail
// survives as `lastCmd`.
//
// Codes and numbers out. Units, words and colours are the caller's, as everywhere else here.
function panelRows(nodes, solutions, poll, nowMs, curStep, schedule) {
    var out = [];
    // Where the cycle is, resolved once for the whole list rather than per row.
    var cursor = cursorNodeId(curStep, schedule);
    for (var i = 0; i < (nodes ? nodes.length : 0); ++i) {
        var n = nodes[i];
        var entry = entryFor(solutions, n.addr);
        var cmd = lastAskedCmd(poll, n.id);
        out.push({
            nodeId: n.id,
            addr: n.addr,
            active: n.active !== false,
            // Off outranks an open window, or the switch looks ignored.
            op: operationCode(n.active !== false, isWaiting(poll, n.id)),
            // The last RESOLVED outcome. Deliberately not the same question as the chip below.
            reply: nodeReplyCode(poll, n.id, entry),
            // The last command ASKED, and its own state. "" when nothing ever was, so the caller
            // draws no chip rather than a chip about nothing.
            lastCmd: cmd,
            lastCmdState: cmd < 0 ? "" : stepCode(poll, n.id, cmd),
            // Where the cycle is: the node last interrogated, or -- before anything has been
            // asked -- the one the next Step will take. It outlives the answer window closing,
            // which is the point: it is the only thing that says where in the schedule you are.
            // Composed here rather than in the panel, like everything else a row says.
            cursor: cursor >= 0 && cursor === n.id,
            // The escalation: consecutive misses, and whether they have gone past the threshold.
            missStreak: missStreak(poll, n.id),
            lost: isLost(poll, n.id),
            entry: entry,
            // Time since this node ANSWERED, not since its numbers landed -- see replyAgeMs.
            ageMs: replyAgeMs(poll, n.id, entry, nowMs),
            aged: isReplyAged(poll, n.id, entry, nowMs, AGE_WARN_MS)
        });
    }
    return out;
}

// ── what the map is told about the plan ──────────────────────────────────────
//
// The beacons drawn on the scene are the plan's nodes, so the plan has to reach the layer. This
// is that message, and it is deliberately NOT derived from panelRows.
//
// A row carries ages and verdicts, so it is recomposed on every clock tick. The map's node list
// changes only when the operator EDITS the plan. Binding the push to rows would hand the scene an
// identical list once a second, and on the other side of that call each one costs a full
// re-projection of every beacon's remembered history.
//
// So: addresses and switches, nothing that moves with time. Colour is grafted on by the caller
// from DataFieldCatalog, where the one address palette lives -- a second table down in C++ is a
// second thing to forget to change, and the colour's whole job is that pane, panel and map agree.
//
// Two nodes CAN share an address. They are one beacon and one track, so the first wins; the
// caller must not draw the same beacon twice with two different switches.
function mapSpec(nodes) {
    var out = [], seen = {};
    for (var i = 0; i < (nodes ? nodes.length : 0); ++i) {
        var n = nodes[i];
        if (!n || typeof n.addr !== "number") continue;
        if (seen[n.addr]) continue;
        seen[n.addr] = true;
        out.push({ addr: n.addr, active: n.active !== false });
    }
    return out;
}

// Node consumes this via module.exports; QML sees the top-level functions and ignores it.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        OFF: OFF, IDLE: IDLE, WAITING: WAITING,
        REPLIED: REPLIED, STALE: STALE, NONE: NONE,
        GRACE_MS: GRACE_MS, AGE_WARN_MS: AGE_WARN_MS, LOST_MISSES: LOST_MISSES,
        missStreak: missStreak, isLost: isLost,
        waitMs: waitMs, stepKey: stepKey, initialPoll: initialPoll,
        queueEmit: queueEmit, dequeueEmit: dequeueEmit, isQueued: isQueued,
        noteSent: noteSent, noteTimeout: noteTimeout, noteReplyAddr: noteReplyAddr,
        isWaiting: isWaiting, isWaitingStep: isWaitingStep,
        stepResult: stepResult, stepCode: stepCode, lastAskedCmd: lastAskedCmd,
        operationCode: operationCode, nodeReplyCode: nodeReplyCode,
        isAged: isAged, replyAgeMs: replyAgeMs, isReplyAged: isReplyAged,
        cursorNodeId: cursorNodeId, isCursorStep: isCursorStep,
        entryFor: entryFor, ageMs: ageMs, summary: summary, panelRows: panelRows,
        mapSpec: mapSpec
    };
}

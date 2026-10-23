.pragma library

var F_HZ = 30000;
var T1_MS = 1024 * 1000 / F_HZ;
var T2_MS = 512 * 1000 / F_HZ;
var H0_MS = 6 * T1_MS;
var SOUND_SPEED_MPS = 1450;
var REPLY_MARGIN_MS = 450;
var STEP_FLOOR_MS = 400;
var PING_TAIL_MS = 250;

/** On-air length of a request or response carrying `bits` data bits. */
function airMs(bits) {
    var b = Math.max(0, bits | 0);
    return T1_MS + 2 * T2_MS + Math.ceil(b / 3) * T2_MS;
}

/** Responder hold before its reply, for a receive slot of `bits`. */
function holdMs(bits) {
    var b = Math.max(0, bits | 0);
    return H0_MS + Math.max(0, Math.floor((b + 5) / 6) - 2) * T1_MS;
}

/** Two-way travel time at the fixed sound speed. */
function travelMs(rangeM) {
    var r = (typeof rangeM === "number" && rangeM > 0) ? rangeM : 0;
    return 2 * r / SOUND_SPEED_MPS * 1000;
}

function oneWayMs(rangeM) { return travelMs(rangeM) / 2; }

/**
 * Shortest safe step for one exchange.
 * spec: { kind: "ranging"|"ping", maxRangeM, reqBits, respBits, followUp: null|{ bits, replyBits } }
 * The no-data ranging case equals the app's Dwell floor (2R/1450 + 450 ms, at least 400 ms).
 */
function requiredMs(spec) {
    var s = spec || {};
    var req = s.reqBits | 0, resp = s.respBits | 0;
    if (s.kind === "ping")
        return Math.max(STEP_FLOOR_MS, Math.ceil(airMs(req) + PING_TAIL_MS));
    var t = travelMs(s.maxRangeM)
          + REPLY_MARGIN_MS
          + (holdMs(req) - H0_MS)
          + (airMs(resp) - airMs(0));
    if (s.followUp) {
        var fb = s.followUp.bits | 0, fr = s.followUp.replyBits | 0;
        t += travelMs(s.maxRangeM) + H0_MS + holdMs(fb) + (airMs(fr) - airMs(0));
    }
    return Math.max(STEP_FLOOR_MS, Math.ceil(t));
}

/** Parts of requiredMs for the inspector, in ms. */
function breakdown(spec) {
    var s = spec || {};
    if (s.kind === "ping")
        return [ { key: "air", ms: airMs(s.reqBits) }, { key: "tail", ms: PING_TAIL_MS } ];
    var out = [
        { key: "travel", ms: travelMs(s.maxRangeM) },
        { key: "base", ms: REPLY_MARGIN_MS },
        { key: "reqData", ms: holdMs(s.reqBits) - H0_MS },
        { key: "respData", ms: airMs(s.respBits) - airMs(0) }
    ];
    if (s.followUp)
        out.push({ key: "followUp",
                   ms: travelMs(s.maxRangeM) + H0_MS + holdMs(s.followUp.bits)
                       + (airMs(s.followUp.replyBits) - airMs(0)) });
    return out;
}

/**
 * Event times of one step relative to its start, for drawing.
 * spec as requiredMs plus plannedRangeM (where the target is drawn), blindAfterRequestMs,
 * blindAfterResponseMs (target).
 */
function stepEvents(spec) {
    var s = spec || {};
    var drawR = (typeof s.plannedRangeM === "number" && s.plannedRangeM > 0)
              ? s.plannedRangeM : (s.maxRangeM || 0);
    var one = oneWayMs(drawR);
    var req = s.reqBits | 0, resp = s.respBits | 0;
    var ev = {
        reqTx: 0,
        reqTxEnd: airMs(req),
        reqAtTarget: one,
        reqAtTargetEnd: one + airMs(req),
        ping: s.kind === "ping",
        replyTx: 0, replyTxEnd: 0, replyAtHead: 0, replyAtHeadEnd: 0,
        windowFrom: 0, windowTo: 0,
        blindHead: [0, s.blindAfterRequestMs || 0],
        blindTarget: null,
        followUp: null
    };
    if (s.kind !== "ping") {
        var hold = holdMs(req);
        ev.replyTx = one + hold;
        ev.replyTxEnd = ev.replyTx + airMs(resp);
        ev.replyAtHead = 2 * one + hold;
        ev.replyAtHeadEnd = ev.replyAtHead + airMs(resp);
        ev.windowFrom = hold;
        ev.windowTo = hold + travelMs(s.maxRangeM);
        ev.blindTarget = [ev.replyTx, ev.replyTx + (s.blindAfterResponseMs || 0)];
        if (s.followUp) {
            var fb = s.followUp.bits | 0, fr = s.followUp.replyBits | 0;
            var ftx = ev.replyAtHead + H0_MS;
            var fhold = holdMs(fb);
            ev.followUp = {
                tx: ftx, txEnd: ftx + airMs(fb),
                atTarget: ftx + one, atTargetEnd: ftx + one + airMs(fb),
                replyTx: ftx + one + fhold, replyTxEnd: ftx + one + fhold + airMs(fr),
                replyAtHead: ftx + 2 * one + fhold, replyAtHeadEnd: ftx + 2 * one + fhold + airMs(fr),
                windowFrom: ftx + fhold, windowTo: ftx + fhold + travelMs(s.maxRangeM)
            };
        }
    }
    return ev;
}

/**
 * Durations of a cycle. specs: [{ requiredMs, fixedMs|null, on }].
 * mode "auto": each step its own required time; "uniform": the longest required time for all;
 * "period": every step `periodMs`, stretched to its own required time when that is longer.
 * The extra margin applies to "auto" and "uniform" only.
 * A fixed duration is kept as entered, even when shorter than required (validation reports it).
 */
function cycleDurations(specs, mode, extraMarginMs, periodMs) {
    var extra = Math.max(0, extraMarginMs | 0), longest = 0, i;
    var period = Math.max(0, periodMs | 0);
    for (i = 0; i < specs.length; ++i)
        if (specs[i].on !== false) longest = Math.max(longest, specs[i].requiredMs);
    var out = [], start = 0;
    for (i = 0; i < specs.length; ++i) {
        var s = specs[i];
        var auto = mode === "period" ? Math.max(period, s.requiredMs)
                 : (mode === "uniform" ? longest : s.requiredMs) + extra;
        var d = (typeof s.fixedMs === "number" && s.fixedMs > 0) ? s.fixedMs : auto;
        out.push({ startMs: start, durationMs: d, autoMs: auto, on: s.on !== false });
        if (s.on !== false) start += d;
    }
    return { steps: out, totalMs: start };
}

/** Max range that covers `rangeM` with a 20 % margin, rounded up to 10 m. */
function fitRangeM(rangeM) {
    var r = (typeof rangeM === "number" && rangeM > 0) ? rangeM : 0;
    return Math.max(10, Math.ceil(r * 1.2 / 10) * 10);
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        F_HZ: F_HZ, T1_MS: T1_MS, T2_MS: T2_MS,
        H0_MS: H0_MS, SOUND_SPEED_MPS: SOUND_SPEED_MPS, REPLY_MARGIN_MS: REPLY_MARGIN_MS,
        STEP_FLOOR_MS: STEP_FLOOR_MS, PING_TAIL_MS: PING_TAIL_MS,
        airMs: airMs, holdMs: holdMs, travelMs: travelMs, oneWayMs: oneWayMs,
        requiredMs: requiredMs, breakdown: breakdown, stepEvents: stepEvents,
        cycleDurations: cycleDurations, fitRangeM: fitRangeM
    };
}

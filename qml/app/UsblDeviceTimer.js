.pragma library

// The head's own interrogation timer (ID_USBL_SCHEDULE): what the app last asked of it, held
// against what the head last reported. Codes out, sentences in QML.
//
// THE LINK DROPS. Over Wi-Fi it does so often, for reasons neither the app nor the head controls.
// The head keeps its timer through that -- only a stop, a cmd-only trigger or a reset ends it --
// so nothing here takes silence for an answer: a request stands until a status read AFTER it
// says how it went, and a timer the head reports running is running whoever started it.
//
// Run and stop are idempotent on the head, so a frame the link may have lost is simply sent again.

var POLL_MS = 1500;
var STALE_MS = 5000;
var ANSWER_MARGIN_MS = 300;
var RUN_TRIES = 3;
var STOP_TRIES = 5;

function wantRun(count, nowMs) {
    return { state: "run", count: count, sentMs: nowMs, tries: 1, confirmed: false };
}

function wantStop(nowMs) {
    return { state: "stop", count: 0, sentMs: nowMs, tries: 1, confirmed: false };
}

function hasStatus(status) { return !!status && status.atMs > 0; }

function isFresh(status, nowMs) { return hasStatus(status) && nowMs - status.atMs <= STALE_MS; }

function silentMs(status, nowMs) { return hasStatus(status) ? Math.max(0, nowMs - status.atMs) : -1; }

function _answered(want, status) { return hasStatus(status) && status.atMs > want.sentMs + ANSWER_MARGIN_MS; }

function _again(want, nowMs) {
    return { state: want.state, count: want.count, sentMs: nowMs, tries: want.tries + 1, confirmed: false };
}

/**
 * One poll of one head. Returns { want, send: "" | "run" | "stop", event }, event one of
 * "" | "started" | "stopped" | "notStarted" | "notStopped" | "stoppedOnDevice".
 * With no status read since the last frame nothing changes: a dropped link leaves the request standing.
 */
function reconcile(want, status, nowMs) {
    if (!want || !_answered(want, status)) return { want: want, send: "", event: "" };
    if (want.state === "stop") {
        if (!status.running) return { want: null, send: "", event: "stopped" };
        if (want.tries >= STOP_TRIES) return { want: null, send: "", event: "notStopped" };
        return { want: _again(want, nowMs), send: "stop", event: "" };
    }
    if (status.running) {
        if (want.confirmed) return { want: want, send: "", event: "" };
        return { want: { state: "run", count: want.count, sentMs: want.sentMs, tries: want.tries, confirmed: true },
                 send: "", event: "started" };
    }
    if (want.confirmed) return { want: null, send: "", event: "stoppedOnDevice" };
    if (want.tries >= RUN_TRIES) return { want: null, send: "", event: "notStarted" };
    return { want: _again(want, nowMs), send: "run", event: "" };
}

/** Whether the head counts as interrogating: as asked, or as it last said when nothing is asked. */
function isRunning(want, status) {
    if (want) return want.state === "run";
    return hasStatus(status) && !!status.running;
}

/**
 * What a status line says, as a code: "starting" | "stopping" | "running" | "stopped" | "unknown",
 * with noLink when the head has not answered for STALE_MS -- the code is then the last known state.
 */
function stateOf(want, status, nowMs) {
    var noLink = hasStatus(status) ? !isFresh(status, nowMs) : !!want;
    var code;
    if (want && want.state === "stop") code = "stopping";
    else if (want && !want.confirmed) code = "starting";
    else if (want || (hasStatus(status) && status.running)) code = "running";
    else code = hasStatus(status) ? "stopped" : "unknown";
    return { code: code, noLink: noLink, silentMs: silentMs(status, nowMs) };
}

// Node consumes this via module.exports; QML sees the top-level functions and ignores it.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        POLL_MS: POLL_MS, STALE_MS: STALE_MS, ANSWER_MARGIN_MS: ANSWER_MARGIN_MS,
        RUN_TRIES: RUN_TRIES, STOP_TRIES: STOP_TRIES,
        wantRun: wantRun, wantStop: wantStop, hasStatus: hasStatus, isFresh: isFresh, silentMs: silentMs,
        reconcile: reconcile, isRunning: isRunning, stateOf: stateOf
    };
}

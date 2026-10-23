.pragma library
.import "NodesModel.js" as Model
.import "NodesPayload.js" as Payload
.import "NodesTiming.js" as Timing
.import "NodesCompile.js" as Compile

var RANGING_REQUEST_BITS_MAX = 162;
var SEV_ORDER = { error: 0, warning: 1, info: 2 };

function _p(code, sev, refs, detail, fix) {
    var o = { code: code, sev: sev, lifeline: -1, exchange: -1, step: -1, detail: detail || {}, fix: fix || null };
    for (var k in (refs || {})) o[k] = refs[k];
    return o;
}

function _answers(l) { return l.respond !== "off"; }

/**
 * Static problems of a network. compiled: NodesCompile.compile(st).
 * Returns [{ code, sev, lifeline, exchange, step, detail, fix: {action, ...} | null }], errors first.
 */
function check(st, compiled) {
    var out = [], i, k;
    var c = compiled || Compile.compile(st);
    var f = st.formats;

    if (!st.lifelines.length) out.push(_p("emptyNetwork", "info"));

    var filters = {};
    for (i = 0; i < st.lifelines.length; ++i) {
        var l = st.lifelines[i];
        filters[l.id] = Compile.filterOf(st, l);
        if (l.kind !== "external" && !l.binding) out.push(_p("unbound", "info", { lifeline: l.id }, {}, { action: "bind" }));
        if (l.respond && typeof l.respond.windowS === "number")
            out.push(_p("windowExpires", "warning", { lifeline: l.id }, { windowS: l.respond.windowS }, { action: "respondAlways" }));
        if (l.executor && l.executor.mode === "periodic")
            out.push(_p("periodicUnavailable", "warning", { lifeline: l.id }, {}, { action: "executorHost", lifeline: l.id }));
    }

    var listenerOf = {};
    for (i = 0; i < st.exchanges.length; ++i)
        for (k = 0; k < st.exchanges[i].listeners.length; ++k) listenerOf[st.exchanges[i].listeners[k].lifeline] = true;
    for (i = 0; i < st.lifelines.length; ++i) {
        var ll = st.lifelines[i];
        if (ll.kind === "external") continue;
        if ((_answers(ll) || listenerOf[ll.id]) && !filters[ll.id].length)
            out.push(_p("deafLifeline", "error", { lifeline: ll.id }, {}, { action: "filterAuto" }));
    }

    var targeted = {};
    for (i = 0; i < st.exchanges.length; ++i) {
        var ex = st.exchanges[i];
        var addr = Model.targetAddress(st, ex);
        var tgt = Model.targetLifeline(st, ex);
        var ini = Model.lifeline(st, ex.from);
        var refs = { exchange: ex.id };
        if (ex.to && typeof ex.to.lifeline === "number" && tgt && typeof tgt.address !== "number")
            out.push(_p("targetHasNoAddress", "error", { exchange: ex.id, lifeline: tgt.id }, {}, { action: "setAddress", lifeline: tgt.id }));
        if (addr < 0 || addr >= Model.ADDRESS_COUNT) {
            if (!(tgt && typeof tgt.address !== "number"))
                out.push(_p("addressOutOfRange", "error", refs, { address: addr }));
            continue;
        }
        targeted[addr] = true;
        if (ex.kind === "ranging" && tgt && !_answers(tgt))
            out.push(_p("targetUnanswered", "error", { exchange: ex.id, lifeline: tgt.id }, {}, { action: "respondAlways", lifeline: tgt.id }));
        if (tgt && Array.isArray(tgt.filter) && filters[tgt.id].indexOf(addr) < 0)
            out.push(_p("targetUnheard", "error", { exchange: ex.id, lifeline: tgt.id }, { address: addr }, { action: "addToFilter", lifeline: tgt.id, address: addr }));
        if (!tgt) out.push(_p("noResponderInNetwork", "warning", refs, { address: addr }));
        if (ini && ini.kind === "external") out.push(_p("externalInitiator", "info", refs));

        var reqBits = Payload.bitsOf(ex.request.data, f);
        var respBits = Payload.bitsOf(ex.response.data, f);
        if (ex.kind === "ranging" && reqBits > RANGING_REQUEST_BITS_MAX)
            out.push(_p("requestTooLongForReply", "error", refs, { bits: reqBits, max: RANGING_REQUEST_BITS_MAX }, { action: "makePing" }));
        if (reqBits > Payload.MAX_BITS) out.push(_p("payloadTooLong", "error", refs, { which: "request", bits: reqBits }));
        if (respBits > Payload.MAX_BITS) out.push(_p("payloadTooLong", "error", refs, { which: "response", bits: respBits }));
        if (ex.followUp) {
            var fb = Payload.bitsOf(ex.followUp.data, f), frb = Payload.bitsOf(ex.followUp.reply.data, f);
            if (fb > RANGING_REQUEST_BITS_MAX) out.push(_p("requestTooLongForReply", "error", refs, { bits: fb, max: RANGING_REQUEST_BITS_MAX, which: "followUp" }));
            if (frb > Payload.MAX_BITS) out.push(_p("payloadTooLong", "error", refs, { which: "followUpReply", bits: frb }));
            if (ex.followUp.data.type === "position") {
                out.push(_p("llGeoNeedsNav", "info", refs));
                out.push(_p("llGeoUnvalidated", "info", refs));
            }
            if (ex.followUp.data.type === "none") out.push(_p("followUpEmpty", "warning", refs, {}, { action: "removeFollowUp" }));
        }
        var dts = [ex.request.data, ex.response.data];
        if (ex.followUp) { dts.push(ex.followUp.data); dts.push(ex.followUp.reply.data); }
        for (k = 0; k < dts.length; ++k)
            if (dts[k] && dts[k].type === "format" && !Payload.formatById(f, dts[k].format))
                out.push(_p("formatMissing", "error", refs));
        if (ex.rewrite && ex.rewrite.asRequest) out.push(_p("replyAsRequest", "warning", refs, {}, { action: "clearRewrite" }));

        if (ex.kind === "ranging") {
            var range = Model.effectiveRangeM(st, ex);
            if (tgt && typeof tgt.plannedRangeM === "number" && tgt.plannedRangeM > range)
                out.push(_p("plannedBeyondMax", "error", { exchange: ex.id, lifeline: tgt.id },
                            { plannedM: tgt.plannedRangeM, maxM: range },
                            { action: "fitRange", exchange: ex.id, rangeM: Timing.fitRangeM(tgt.plannedRangeM) }));
            else if (range < 50)
                out.push(_p("maxRangeShort", "info", refs, { maxM: range }));
        }

        for (k = 0; k < ex.listeners.length; ++k) {
            var L = Model.lifeline(st, ex.listeners[k].lifeline);
            if (L && _answers(L) && filters[L.id].indexOf(addr) >= 0)
                out.push(_p("listenerWouldAnswer", "error", { exchange: ex.id, lifeline: L.id }, { address: addr }, { action: "respondOff", lifeline: L.id }));
        }
    }

    for (var a in targeted) {
        var na = Number(a), answering = [];
        for (i = 0; i < st.lifelines.length; ++i) {
            var x = st.lifelines[i];
            if (x.kind !== "external" && _answers(x) && filters[x.id].indexOf(na) >= 0) answering.push(x.id);
        }
        if (answering.length > 1)
            out.push(_p("addressCollision", "error", { lifeline: answering[1] }, { address: na, lifelines: answering }, { action: "freeAddress", lifeline: answering[1] }));
    }

    for (i = 0; i < c.problems.length; ++i)
        out.push(_p(c.problems[i].code, "error", { exchange: c.problems[i].exchange }, {}, { action: "cmdAuto", exchange: c.problems[i].exchange }));

    var perInitiator = {}, enabledCount = 0;
    for (i = 0; i < c.steps.length; ++i) {
        var s = c.steps[i];
        if (!s.enabled) continue;
        ++enabledCount;
        perInitiator[s.initiatorId] = (perInitiator[s.initiatorId] || 0) + 1;
        var stp = Model.step(st, s.stepId);
        if (stp && typeof stp.durationMs === "number") {
            if (stp.durationMs < s.requiredMs)
                out.push(_p("stepTooShort", "error", { step: s.stepId, exchange: s.exchangeId },
                            { durationMs: stp.durationMs, requiredMs: s.requiredMs }, { action: "stepAuto", step: s.stepId }));
            else if (stp.durationMs > 2 * s.requiredMs)
                out.push(_p("stepMuchLonger", "info", { step: s.stepId }, { durationMs: stp.durationMs, requiredMs: s.requiredMs }, { action: "stepAuto", step: s.stepId }));
        }
        if (s.executor === "pps" && s.requiredMs > 1000)
            out.push(_p("ppsStepVsPeriod", "info", { step: s.stepId }, { periods: Math.ceil(s.requiredMs / 1000) }));
    }
    if (st.exchanges.length && !enabledCount) out.push(_p("noSteps", "info"));
    for (var id in perInitiator) {
        var h = Model.lifeline(st, Number(id));
        if (h && h.executor.mode === "periodic" && perInitiator[id] !== 1)
            out.push(_p("periodicNeedsOneStep", "error", { lifeline: h.id }, { steps: perInitiator[id] }, { action: "executorHost", lifeline: h.id }));
    }
    var boundHeads = 0;
    var inis = Model.initiators(st);
    for (i = 0; i < inis.length; ++i) if (inis[i].binding) ++boundHeads;
    if (boundHeads > 1) out.push(_p("multiHeadLive", "warning"));

    out.sort(function (p, q) { return SEV_ORDER[p.sev] - SEV_ORDER[q.sev]; });
    return out;
}

/**
 * Problems that need live data. solutions: Dataset.usblSolutions (keyed by address string).
 */
function liveCheck(st, compiled, solutions) {
    var out = [], sols = solutions || {};
    for (var i = 0; i < st.exchanges.length; ++i) {
        var ex = st.exchanges[i];
        if (ex.kind !== "ranging") continue;
        var addr = Model.targetAddress(st, ex);
        var e = sols[String(addr)];
        if (!e || typeof e.distance !== "number" || isNaN(e.distance)) continue;
        var range = Model.effectiveRangeM(st, ex);
        if (e.distance < 0)
            out.push(_p("negativeRange", "warning", { exchange: ex.id }, { distance: e.distance }));
        else if (e.distance > 0.9 * range)
            out.push(_p("measuredNearMax", "warning", { exchange: ex.id }, { distance: e.distance, maxM: range },
                        { action: "fitRange", exchange: ex.id, rangeM: Timing.fitRangeM(e.distance) }));
    }
    return out;
}

/** Problems of pairing lifelines with connected devices. assign: NodesModel.assignDevices(st, ...). */
function deviceCheck(st, assign) {
    var out = [];
    for (var i = 0; i < st.lifelines.length; ++i) {
        var l = st.lifelines[i], n = assign ? assign.ambiguous[l.id] : 0;
        if (n) out.push(_p("sharedSerial", "warning", { lifeline: l.id }, { serial: l.binding.serial, count: n }, { action: "bind" }));
    }
    return out;
}

/** Worst severity of the problems touching a lifeline, exchange or step: "error" | "warning" | "info" | "". */
function worstFor(problems, key, id) {
    var best = "";
    for (var i = 0; i < (problems ? problems.length : 0); ++i) {
        var p = problems[i];
        if (p[key] !== id) continue;
        if (best === "" || SEV_ORDER[p.sev] < SEV_ORDER[best]) best = p.sev;
    }
    return best;
}

function counts(problems) {
    var o = { error: 0, warning: 0, info: 0 };
    for (var i = 0; i < (problems ? problems.length : 0); ++i) o[problems[i].sev] += 1;
    return o;
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        RANGING_REQUEST_BITS_MAX: RANGING_REQUEST_BITS_MAX,
        check: check, liveCheck: liveCheck, deviceCheck: deviceCheck, worstFor: worstFor, counts: counts
    };
}

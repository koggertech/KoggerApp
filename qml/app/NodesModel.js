.pragma library

var KIND = "kogger.nodes";
var FORMAT_VERSION = 1;
var ADDRESS_COUNT = 8;
var CMD_COUNT = 8;
var MIN_RANGE_M = 1;
var MAX_RANGE_M = 2000;
var DEFAULT_RANGE_M = 300;
var MIN_PERIOD_MS = 100;
var MAX_PERIOD_MS = 3600000;
var DEFAULT_PERIOD_MS = 1000;
var STEP_MODES = ["auto", "uniform", "period"];
var LIFELINE_KINDS = ["head", "beacon", "external"];
var DATA_TYPES = ["none", "bytes", "bits", "text", "format", "position"];

var _KNOWN_KEYS = ["formatVersion", "kind", "name", "notes", "maxRangeM", "transmit",
                   "lifelines", "formats", "exchanges", "cycle", "view", "nextId"];

function clone(v) { return JSON.parse(JSON.stringify(v)); }

function _next(st) { st.nextId = (st.nextId | 0) + 1; return st.nextId; }

function defaultMonitor() {
    return { blindAfterRequestMs: 400, blindAfterResponseMs: 400, hearResponsesWhenIdle: false };
}

function clampPeriod(ms) {
    var v = Number(ms);
    if (!isFinite(v)) v = DEFAULT_PERIOD_MS;
    return Math.max(MIN_PERIOD_MS, Math.min(MAX_PERIOD_MS, Math.round(v)));
}

function clampRange(m) {
    var v = Number(m);
    if (!isFinite(v)) v = DEFAULT_RANGE_M;
    return Math.max(MIN_RANGE_M, Math.min(MAX_RANGE_M, Math.round(v)));
}

/** An empty network: no lifelines, default Max range. */
function emptyState(name) {
    return {
        formatVersion: FORMAT_VERSION, kind: KIND,
        name: name || "", notes: "",
        maxRangeM: DEFAULT_RANGE_M, transmit: "immediately",
        lifelines: [], formats: [], exchanges: [],
        cycle: { steps: [], stepMode: "auto", extraMarginMs: 0, periodMs: DEFAULT_PERIOD_MS, repeat: true },
        view: { order: [], msPerPx: 4 },
        nextId: 0, extra: {}
    };
}

function newLifeline(st, kind, address) {
    var k = LIFELINE_KINDS.indexOf(kind) >= 0 ? kind : "beacon";
    var id = _next(st);
    return {
        id: id,
        label: k === "head" ? "Head" : (k === "external" ? "External" : "Beacon"),
        kind: k,
        address: (k === "head") ? null : (typeof address === "number" ? address : freeAddress(st)),
        respond: k === "beacon" ? "always" : "off",
        filter: "auto",
        monitor: defaultMonitor(),
        executor: { mode: "host", periodMs: 1000 },
        plannedRangeM: null,
        active: true,
        binding: null
    };
}

function newExchange(st, fromId, toId, kind) {
    return {
        id: _next(st), label: "",
        from: fromId, to: { lifeline: toId },
        kind: kind === "ping" ? "ping" : "ranging",
        maxRange: "network",
        cmd: "auto", cmdResolved: null,
        request: { data: { type: "none" } },
        response: { data: { type: "none" } },
        followUp: null, rewrite: null, listeners: []
    };
}

function newStep(st, exchangeId) {
    return { id: _next(st), exchange: exchangeId, on: true, durationMs: "auto" };
}

// ── lookups ──────────────────────────────────────────────────────────────────
function byId(list, id) {
    for (var i = 0; i < (list ? list.length : 0); ++i) if (list[i].id === id) return list[i];
    return null;
}
function indexById(list, id) {
    for (var i = 0; i < (list ? list.length : 0); ++i) if (list[i].id === id) return i;
    return -1;
}
function lifeline(st, id) { return byId(st.lifelines, id); }
function exchange(st, id) { return byId(st.exchanges, id); }
function step(st, id) { return byId(st.cycle.steps, id); }

function usedAddresses(st) {
    var out = {};
    for (var i = 0; i < st.lifelines.length; ++i) {
        var l = st.lifelines[i];
        if (typeof l.address === "number") out[l.address] = true;
    }
    return out;
}

function freeAddress(st) {
    var used = usedAddresses(st);
    for (var a = 1; a < ADDRESS_COUNT; ++a) if (!used[a]) return a;
    return used[0] ? 1 : 0;
}

/** Address an exchange targets, or -1. */
function targetAddress(st, ex) {
    if (!ex || !ex.to) return -1;
    if (typeof ex.to.address === "number") return ex.to.address;
    var l = lifeline(st, ex.to.lifeline);
    return (l && typeof l.address === "number") ? l.address : -1;
}

function targetLifeline(st, ex) {
    if (!ex || !ex.to) return null;
    if (typeof ex.to.lifeline === "number") return lifeline(st, ex.to.lifeline);
    var a = ex.to.address, i;
    for (i = 0; i < st.lifelines.length; ++i)
        if (st.lifelines[i].address === a && st.lifelines[i].respond !== "off" && st.lifelines[i].id !== ex.from)
            return st.lifelines[i];
    for (i = 0; i < st.lifelines.length; ++i) {
        var l = st.lifelines[i];
        if (l.id !== ex.from && l.respond !== "off" && Array.isArray(l.filter) && l.filter.indexOf(a) >= 0) return l;
    }
    return null;
}

/** Max range an exchange's requests carry, in metres; 0 for a ping. */
function effectiveRangeM(st, ex) {
    if (!ex || ex.kind === "ping") return 0;
    return (typeof ex.maxRange === "number") ? clampRange(ex.maxRange) : clampRange(st.maxRangeM);
}

function initiators(st) {
    var seen = {}, out = [];
    for (var i = 0; i < st.exchanges.length; ++i) {
        var id = st.exchanges[i].from;
        if (seen[id]) continue;
        var l = lifeline(st, id);
        if (l) { seen[id] = true; out.push(l); }
    }
    return out;
}

function isInitiator(st, id) {
    for (var i = 0; i < st.exchanges.length; ++i) if (st.exchanges[i].from === id) return true;
    return false;
}

function orderedLifelines(st) {
    var out = [], seen = {}, i;
    var order = (st.view && st.view.order) ? st.view.order : [];
    for (i = 0; i < order.length; ++i) {
        var l = lifeline(st, order[i]);
        if (l && !seen[l.id]) { seen[l.id] = true; out.push(l); }
    }
    for (i = 0; i < st.lifelines.length; ++i)
        if (!seen[st.lifelines[i].id]) out.push(st.lifelines[i]);
    return out;
}

function bindingOf(st, id) {
    var l = lifeline(st, id);
    return l ? l.binding : null;
}

function lifelineBySerial(st, serial) {
    for (var i = 0; i < st.lifelines.length; ++i) {
        var b = st.lifelines[i].binding;
        if (b && b.serial === serial) return st.lifelines[i];
    }
    return null;
}

// ── network mutators: state in, new state out ────────────────────────────────
function setNetworkField(st, field, value) {
    var n = clone(st);
    switch (field) {
    case "name": n.name = String(value || ""); break;
    case "notes": n.notes = String(value || ""); break;
    case "maxRangeM": n.maxRangeM = clampRange(value); break;
    case "transmit": n.transmit = value === "pps" ? "pps" : "immediately"; break;
    case "stepMode": n.cycle.stepMode = STEP_MODES.indexOf(value) >= 0 ? value : "auto"; break;
    case "periodMs": n.cycle.periodMs = clampPeriod(value); break;
    case "extraMarginMs": n.cycle.extraMarginMs = Math.max(0, Math.min(10000, Math.round(Number(value) || 0))); break;
    case "repeat": n.cycle.repeat = !!value; break;
    case "msPerPx": n.view.msPerPx = Math.max(0.5, Math.min(40, Number(value) || 4)); break;
    default: return st;
    }
    return n;
}

function addLifeline(st, kind, address) {
    var n = clone(st);
    var l = newLifeline(n, kind, address);
    n.lifelines.push(l);
    n.view.order = orderedLifelines(n).map(function (x) { return x.id; });
    return { state: n, id: l.id };
}

function removeLifeline(st, id) {
    var n = clone(st);
    if (indexById(n.lifelines, id) < 0) return st;
    n.lifelines = n.lifelines.filter(function (l) { return l.id !== id; });
    var gone = {};
    n.exchanges = n.exchanges.filter(function (e) {
        var drop = e.from === id || (e.to && e.to.lifeline === id);
        if (drop) gone[e.id] = true;
        return !drop;
    });
    for (var i = 0; i < n.exchanges.length; ++i)
        n.exchanges[i].listeners = n.exchanges[i].listeners.filter(function (x) { return x.lifeline !== id; });
    n.cycle.steps = n.cycle.steps.filter(function (s) { return !gone[s.exchange]; });
    n.view.order = n.view.order.filter(function (x) { return x !== id; });
    return n;
}

function setLifelineField(st, id, field, value) {
    var n = clone(st), l = lifeline(n, id);
    if (!l) return st;
    switch (field) {
    case "label": l.label = String(value || ""); break;
    case "kind":
        if (LIFELINE_KINDS.indexOf(value) < 0) return st;
        l.kind = value;
        if (value === "external") l.binding = null;
        break;
    case "address":
        l.address = (value === null || value === undefined) ? null
                  : Math.max(0, Math.min(ADDRESS_COUNT - 1, Math.round(Number(value))));
        break;
    case "respond":
        if (value === "off" || value === "always") l.respond = value;
        else if (value && typeof value.windowS === "number")
            l.respond = { windowS: Math.max(1, Math.round(value.windowS)) };
        else return st;
        break;
    case "filter":
        if (value === "auto") l.filter = "auto";
        else if (Array.isArray(value)) l.filter = _cleanAddrs(value);
        else return st;
        break;
    case "plannedRangeM":
        l.plannedRangeM = (value === null || value === undefined || value === "") ? null
                        : Math.max(0, Math.min(MAX_RANGE_M * 2, Number(value) || 0));
        break;
    case "active": l.active = !!value; break;
    case "executorMode":
        if (["host", "pps", "periodic"].indexOf(value) < 0) return st;
        l.executor.mode = value;
        break;
    case "periodMs": l.executor.periodMs = Math.max(400, Math.min(600000, Math.round(Number(value) || 1000))); break;
    case "blindAfterRequestMs": l.monitor.blindAfterRequestMs = Math.max(0, Math.min(2000, Math.round(Number(value) || 0))); break;
    case "blindAfterResponseMs": l.monitor.blindAfterResponseMs = Math.max(0, Math.min(2000, Math.round(Number(value) || 0))); break;
    case "hearResponsesWhenIdle": l.monitor.hearResponsesWhenIdle = !!value; break;
    default: return st;
    }
    return n;
}

function _cleanAddrs(arr) {
    var seen = {}, out = [];
    for (var i = 0; i < arr.length && out.length < ADDRESS_COUNT; ++i) {
        var a = Math.round(Number(arr[i]));
        if (!isFinite(a) || a < 0 || a >= ADDRESS_COUNT || seen[a]) continue;
        seen[a] = true; out.push(a);
    }
    return out.sort(function (x, y) { return x - y; });
}

/** Whether two bindings name one board: by UID when it has one, else by serial and the port it was bound on. */
function sameDevice(a, b) {
    if (!a || !b || a.serial !== b.serial || (a.uid || "") !== (b.uid || "")) return false;
    return !!a.uid || (a.at || "") === (b.at || "");
}

/**
 * binding: { serial, uid, devName, fw, at } or null; `at` is the port the device was bound on.
 * A device binds to at most one lifeline.
 */
function bindLifeline(st, id, binding) {
    var n = clone(st), l = lifeline(n, id);
    if (!l || l.kind === "external") return st;
    if (binding) {
        for (var i = 0; i < n.lifelines.length; ++i) {
            var o = n.lifelines[i];
            if (o.id !== id && sameDevice(o.binding, binding)) o.binding = null;
        }
        l.binding = { serial: binding.serial, uid: binding.uid || "",
                      devName: binding.devName || "", fw: binding.fw || "", at: binding.at || "" };
    } else {
        l.binding = null;
    }
    return n;
}

/**
 * Pairs bound lifelines with connected devices, one to one. devs: [{ serial, uid, at }].
 * A binding fits a device by serial, and by UID when the binding has one. Among fitting devices
 * the one on the bound port wins, then list order.
 * Returns { owner: [lifeline id or -1, per device], deviceOf: { lifelineId: device index },
 * ambiguous: { lifelineId: fitting devices } } — ambiguous lists lifelines matched by order alone.
 */
function assignDevices(st, devs) {
    var owner = devs.map(function () { return -1; }), deviceOf = {}, ambiguous = {};
    function fits(b, d) { return b.serial === d.serial && (!b.uid || b.uid === d.uid); }
    for (var pass = 0; pass < 2; ++pass) {
        for (var i = 0; i < st.lifelines.length; ++i) {
            var l = st.lifelines[i], b = l.binding;
            if (!b || deviceOf[l.id] !== undefined) continue;
            var pick = -1, n = 0;
            for (var k = 0; k < devs.length; ++k) {
                if (!fits(b, devs[k])) continue;
                n += 1;
                if (pick < 0 && owner[k] < 0 && (pass > 0 || (!!b.at && devs[k].at === b.at))) pick = k;
            }
            if (pick < 0) continue;
            owner[pick] = l.id;
            deviceOf[l.id] = pick;
            if (pass > 0 && n > 1) ambiguous[l.id] = n;
        }
    }
    return { owner: owner, deviceOf: deviceOf, ambiguous: ambiguous };
}

function moveLifeline(st, id, toIndex) {
    var n = clone(st);
    var order = orderedLifelines(n).map(function (x) { return x.id; });
    var from = order.indexOf(id);
    if (from < 0) return st;
    order.splice(from, 1);
    var to = Math.max(0, Math.min(order.length, toIndex));
    order.splice(to, 0, id);
    n.view.order = order;
    return n;
}

// ── exchanges ────────────────────────────────────────────────────────────────
/** Adds an exchange and one step running it. Returns { state, exchangeId, stepId }. */
function addExchange(st, fromId, toId, kind, atStepIndex) {
    if (!lifeline(st, fromId) || !lifeline(st, toId) || fromId === toId)
        return { state: st, exchangeId: -1, stepId: -1 };
    var n = clone(st);
    var e = newExchange(n, fromId, toId, kind);
    n.exchanges.push(e);
    var s = newStep(n, e.id);
    var at = (typeof atStepIndex === "number") ? Math.max(0, Math.min(n.cycle.steps.length, atStepIndex))
                                               : n.cycle.steps.length;
    n.cycle.steps.splice(at, 0, s);
    return { state: n, exchangeId: e.id, stepId: s.id };
}

function removeExchange(st, id) {
    var n = clone(st);
    if (indexById(n.exchanges, id) < 0) return st;
    n.exchanges = n.exchanges.filter(function (e) { return e.id !== id; });
    n.cycle.steps = n.cycle.steps.filter(function (s) { return s.exchange !== id; });
    return n;
}

function setExchangeField(st, id, field, value) {
    var n = clone(st), e = exchange(n, id);
    if (!e) return st;
    switch (field) {
    case "label": e.label = String(value || ""); break;
    case "kind": e.kind = value === "ping" ? "ping" : "ranging"; break;
    case "maxRange": e.maxRange = (value === "network") ? "network" : clampRange(value); break;
    case "cmd":
        e.cmd = (value === "auto") ? "auto" : Math.max(0, Math.min(CMD_COUNT - 1, Math.round(Number(value))));
        break;
    case "from":
        if (!lifeline(n, value) || (e.to && e.to.lifeline === value)) return st;
        e.from = value;
        break;
    case "toLifeline":
        if (!lifeline(n, value) || value === e.from) return st;
        e.to = { lifeline: value };
        break;
    case "toAddress":
        e.to = { address: Math.max(0, Math.min(ADDRESS_COUNT - 1, Math.round(Number(value)))) };
        break;
    default: return st;
    }
    return n;
}

/** which: "request" | "response" | "followUp" | "followUpReply". */
function setExchangeData(st, id, which, data) {
    var n = clone(st), e = exchange(n, id);
    if (!e) return st;
    var d = _cleanData(data);
    switch (which) {
    case "request":
        if (d.type === "position") return st;
        e.request.data = d;
        break;
    case "response":
        if (d.type === "position") return st;
        e.response.data = d;
        break;
    case "followUp":
        if (!e.followUp) e.followUp = _newFollowUp();
        e.followUp.data = d;
        break;
    case "followUpReply":
        if (!e.followUp) e.followUp = _newFollowUp();
        if (d.type === "position") return st;
        e.followUp.reply = { data: d };
        break;
    default: return st;
    }
    return n;
}

function _newFollowUp() {
    return { data: { type: "position", source: "headNav" }, reply: { data: { type: "none" } },
             cmd: "auto", cmdResolved: null };
}

function setFollowUp(st, id, on) {
    var n = clone(st), e = exchange(n, id);
    if (!e) return st;
    e.followUp = on ? (e.followUp || _newFollowUp()) : null;
    if (on && e.kind === "ping") e.kind = "ranging";
    return n;
}

function setFollowUpCmd(st, id, cmd) {
    var n = clone(st), e = exchange(n, id);
    if (!e || !e.followUp) return st;
    e.followUp.cmd = (cmd === "auto") ? "auto" : Math.max(0, Math.min(CMD_COUNT - 1, Math.round(Number(cmd))));
    return n;
}

/** rewrite: null or { address: n|null, cmd: n|null, asRequest: bool }. */
function setRewrite(st, id, rewrite) {
    var n = clone(st), e = exchange(n, id);
    if (!e) return st;
    if (!rewrite) { e.rewrite = null; return n; }
    var a = (typeof rewrite.address === "number") ? Math.max(0, Math.min(ADDRESS_COUNT - 1, Math.round(rewrite.address))) : null;
    var c = (typeof rewrite.cmd === "number") ? Math.max(0, Math.min(CMD_COUNT - 1, Math.round(rewrite.cmd))) : null;
    e.rewrite = { address: a, cmd: c, asRequest: !!rewrite.asRequest };
    if (a === null && c === null && !e.rewrite.asRequest) e.rewrite = null;
    return n;
}

function setListener(st, id, lifelineId, on, hearResponses) {
    var n = clone(st), e = exchange(n, id);
    if (!e || !lifeline(n, lifelineId) || lifelineId === e.from) return st;
    if (e.to && e.to.lifeline === lifelineId) return st;
    e.listeners = e.listeners.filter(function (x) { return x.lifeline !== lifelineId; });
    if (on) e.listeners.push({ lifeline: lifelineId, hearResponses: !!hearResponses });
    return n;
}

function _cleanData(d) {
    if (!d || DATA_TYPES.indexOf(d.type) < 0 || d.type === "none") return { type: "none" };
    switch (d.type) {
    case "bytes": {
        var hex = String(d.hex || "").replace(/[^0-9a-fA-F]/g, "").toUpperCase();
        if (hex.length % 2) hex = hex.substring(0, hex.length - 1);
        var out = { type: "bytes", hex: hex };
        if (typeof d.bits === "number" && d.bits > 0 && d.bits < hex.length * 4) out.bits = Math.round(d.bits);
        return out;
    }
    case "bits": return { type: "bits", binary: String(d.binary || "").replace(/[^01]/g, "") };
    case "text": return { type: "text", ascii: String(d.ascii || "").replace(/[^\x20-\x7E]/g, "") };
    case "format": return { type: "format", format: d.format, values: clone(d.values || {}) };
    case "position": return { type: "position", source: "headNav" };
    }
    return { type: "none" };
}

// ── steps ────────────────────────────────────────────────────────────────────
function addStep(st, exchangeId, atIndex) {
    if (!exchange(st, exchangeId)) return { state: st, stepId: -1 };
    var n = clone(st), s = newStep(n, exchangeId);
    var at = (typeof atIndex === "number") ? Math.max(0, Math.min(n.cycle.steps.length, atIndex))
                                           : n.cycle.steps.length;
    n.cycle.steps.splice(at, 0, s);
    return { state: n, stepId: s.id };
}

function removeStep(st, id) {
    var n = clone(st);
    var before = n.cycle.steps.length;
    n.cycle.steps = n.cycle.steps.filter(function (s) { return s.id !== id; });
    return n.cycle.steps.length === before ? st : n;
}

function moveStep(st, id, toIndex) {
    var n = clone(st), i = indexById(n.cycle.steps, id);
    if (i < 0) return st;
    var s = n.cycle.steps.splice(i, 1)[0];
    var to = Math.max(0, Math.min(n.cycle.steps.length, toIndex));
    n.cycle.steps.splice(to, 0, s);
    return n;
}

function setStepField(st, id, field, value) {
    var n = clone(st), s = step(n, id);
    if (!s) return st;
    switch (field) {
    case "on": s.on = !!value; break;
    case "durationMs":
        s.durationMs = (value === "auto" || value === null) ? "auto"
                     : Math.max(100, Math.min(600000, Math.round(Number(value) || 0)));
        break;
    default: return st;
    }
    return n;
}

function duplicateStep(st, id) {
    var i = indexById(st.cycle.steps, id);
    if (i < 0) return { state: st, stepId: -1 };
    var r = addStep(st, st.cycle.steps[i].exchange, i + 1);
    var s = step(r.state, r.stepId);
    s.durationMs = st.cycle.steps[i].durationMs;
    s.on = st.cycle.steps[i].on;
    return r;
}

// ── payload formats ──────────────────────────────────────────────────────────
function addFormat(st, name) {
    var n = clone(st);
    var f = { id: _next(n), name: name || ("Format " + (n.formats.length + 1)), kind: "fields",
              fields: [ { name: "code", bits: 8, type: "uint" } ] };
    n.formats.push(f);
    return { state: n, id: f.id };
}

function setFormat(st, id, format) {
    var n = clone(st), i = indexById(n.formats, id);
    if (i < 0 || !format) return st;
    var fields = [];
    var src = Array.isArray(format.fields) ? format.fields : [];
    for (var k = 0; k < src.length; ++k) {
        var f = src[k];
        var type = ["uint", "int", "bool"].indexOf(f.type) >= 0 ? f.type : "uint";
        var out = { name: String(f.name || ("f" + k)), bits: type === "bool" ? 1 : Math.max(1, Math.min(64, f.bits | 0)), type: type };
        if (typeof f.scale === "number" && f.scale !== 0 && f.scale !== 1) out.scale = f.scale;
        if (typeof f.offset === "number" && f.offset !== 0) out.offset = f.offset;
        if (f.unit) out.unit = String(f.unit);
        fields.push(out);
    }
    n.formats[i] = { id: id, name: String(format.name || n.formats[i].name), kind: "fields", fields: fields };
    return n;
}

function removeFormat(st, id) {
    var n = clone(st);
    if (indexById(n.formats, id) < 0) return st;
    n.formats = n.formats.filter(function (f) { return f.id !== id; });
    var clear = function (d) { return (d && d.type === "format" && d.format === id) ? { type: "none" } : d; };
    for (var i = 0; i < n.exchanges.length; ++i) {
        var e = n.exchanges[i];
        e.request.data = clear(e.request.data);
        e.response.data = clear(e.response.data);
        if (e.followUp) { e.followUp.data = clear(e.followUp.data); e.followUp.reply.data = clear(e.followUp.reply.data); }
    }
    return n;
}

/** Writes the allocator's choices back so the next allocation prefers them. */
function recordResolvedCmds(st, resolved) {
    var n = clone(st), changed = false;
    for (var i = 0; i < n.exchanges.length; ++i) {
        var e = n.exchanges[i], r = resolved[e.id];
        if (!r) continue;
        if (typeof r.cmd === "number" && e.cmdResolved !== r.cmd) { e.cmdResolved = r.cmd; changed = true; }
        if (e.followUp && typeof r.followCmd === "number" && e.followUp.cmdResolved !== r.followCmd) {
            e.followUp.cmdResolved = r.followCmd; changed = true;
        }
    }
    return changed ? n : st;
}

// ── persistence ──────────────────────────────────────────────────────────────
/** JSON text of a network. options.withBindings defaults to true. */
function serialize(st, options) {
    var withBindings = !(options && options.withBindings === false);
    var o = {};
    var extra = st.extra || {};
    for (var k in extra) o[k] = extra[k];
    o.formatVersion = FORMAT_VERSION;
    o.kind = KIND;
    o.name = st.name; o.notes = st.notes;
    o.maxRangeM = st.maxRangeM; o.transmit = st.transmit;
    o.lifelines = st.lifelines.map(function (l) {
        var c = clone(l);
        if (!withBindings) c.binding = null;
        return c;
    });
    o.formats = clone(st.formats);
    o.exchanges = clone(st.exchanges);
    o.cycle = clone(st.cycle);
    o.view = clone(st.view);
    o.nextId = st.nextId;
    return JSON.stringify(o, null, 2);
}

/**
 * Parses a network. Returns { state, problems: [{code, detail}], readOnly, error }.
 * error is set (and state null) only when the text is not a network at all.
 */
function deserialize(text) {
    var o, problems = [];
    try { o = JSON.parse(text); }
    catch (e) { return { state: null, problems: [], readOnly: false, error: { code: "notJson", detail: String(e) } }; }
    if (!o || typeof o !== "object" || o.kind !== KIND)
        return { state: null, problems: [], readOnly: false, error: { code: "notNodes", detail: "" } };
    var readOnly = typeof o.formatVersion === "number" && o.formatVersion > FORMAT_VERSION;
    var st = emptyState(String(o.name || ""));
    st.notes = String(o.notes || "");
    st.maxRangeM = clampRange(typeof o.maxRangeM === "number" ? o.maxRangeM : DEFAULT_RANGE_M);
    st.transmit = o.transmit === "pps" ? "pps" : "immediately";
    st.nextId = (typeof o.nextId === "number") ? o.nextId : 0;
    for (var k in o) if (_KNOWN_KEYS.indexOf(k) < 0) st.extra[k] = o[k];

    var maxId = 0;
    var bump = function (id) { if (typeof id === "number" && id > maxId) maxId = id; };
    var seenIds = {};
    var takeId = function (id) {
        if (typeof id !== "number" || seenIds[id]) return -1;
        seenIds[id] = true; bump(id); return id;
    };

    var lls = Array.isArray(o.lifelines) ? o.lifelines : [];
    for (var i = 0; i < lls.length; ++i) {
        var src = lls[i];
        if (!src || typeof src !== "object") continue;
        var id = takeId(src.id);
        if (id < 0) { problems.push({ code: "lifelineDropped", detail: String(src.label || i) }); continue; }
        var l = newLifeline({ nextId: 0, lifelines: [] }, src.kind, typeof src.address === "number" ? src.address : null);
        l.id = id;
        l.label = String(src.label || l.label);
        l.kind = LIFELINE_KINDS.indexOf(src.kind) >= 0 ? src.kind : "beacon";
        l.address = (typeof src.address === "number" && src.address >= 0 && src.address < ADDRESS_COUNT) ? Math.round(src.address) : null;
        if (typeof src.address === "number" && l.address === null)
            problems.push({ code: "addressDropped", detail: l.label });
        l.respond = (src.respond === "off" || src.respond === "always") ? src.respond
                  : (src.respond && typeof src.respond.windowS === "number") ? { windowS: Math.max(1, Math.round(src.respond.windowS)) }
                  : (l.kind === "beacon" ? "always" : "off");
        l.filter = Array.isArray(src.filter) ? _cleanAddrs(src.filter) : "auto";
        var m = src.monitor || {};
        l.monitor = {
            blindAfterRequestMs: typeof m.blindAfterRequestMs === "number" ? m.blindAfterRequestMs : 400,
            blindAfterResponseMs: typeof m.blindAfterResponseMs === "number" ? m.blindAfterResponseMs : 400,
            hearResponsesWhenIdle: !!m.hearResponsesWhenIdle
        };
        var ex = src.executor || {};
        l.executor = { mode: ["host", "pps", "periodic"].indexOf(ex.mode) >= 0 ? ex.mode : "host",
                       periodMs: typeof ex.periodMs === "number" ? ex.periodMs : 1000 };
        l.plannedRangeM = typeof src.plannedRangeM === "number" ? src.plannedRangeM : null;
        l.active = src.active !== false;
        l.binding = (src.binding && typeof src.binding.serial === "number" && l.kind !== "external")
                  ? { serial: src.binding.serial, uid: String(src.binding.uid || ""),
                      devName: String(src.binding.devName || ""), fw: String(src.binding.fw || ""),
                      at: String(src.binding.at || "") }
                  : null;
        st.lifelines.push(l);
    }

    var fs = Array.isArray(o.formats) ? o.formats : [];
    for (var f = 0; f < fs.length; ++f) {
        var fid = takeId(fs[f] && fs[f].id);
        if (fid < 0) continue;
        st.formats.push({ id: fid, name: "", kind: "fields", fields: [] });
        st = setFormat(st, fid, fs[f]);
    }

    var exs = Array.isArray(o.exchanges) ? o.exchanges : [];
    for (var x = 0; x < exs.length; ++x) {
        var se = exs[x];
        if (!se || typeof se !== "object") continue;
        var eid = takeId(se.id);
        if (eid < 0 || !lifeline(st, se.from)) { problems.push({ code: "exchangeDropped", detail: String(se.label || x) }); continue; }
        var e = newExchange({ nextId: 0 }, se.from, -1, se.kind);
        e.id = eid;
        e.label = String(se.label || "");
        if (se.to && typeof se.to.lifeline === "number" && lifeline(st, se.to.lifeline) && se.to.lifeline !== se.from)
            e.to = { lifeline: se.to.lifeline };
        else if (se.to && typeof se.to.address === "number" && se.to.address >= 0 && se.to.address < ADDRESS_COUNT)
            e.to = { address: Math.round(se.to.address) };
        else { problems.push({ code: "exchangeDropped", detail: e.label || String(eid) }); continue; }
        e.maxRange = (typeof se.maxRange === "number") ? clampRange(se.maxRange) : "network";
        e.cmd = (typeof se.cmd === "number" && se.cmd >= 0 && se.cmd < CMD_COUNT) ? Math.round(se.cmd) : "auto";
        e.cmdResolved = (typeof se.cmdResolved === "number") ? se.cmdResolved : null;
        e.request = { data: _cleanData(se.request && se.request.data) };
        if (e.request.data.type === "position") e.request.data = { type: "none" };
        e.response = { data: _cleanData(se.response && se.response.data) };
        if (e.response.data.type === "position") e.response.data = { type: "none" };
        if (se.followUp && typeof se.followUp === "object") {
            e.followUp = _newFollowUp();
            e.followUp.data = _cleanData(se.followUp.data);
            e.followUp.reply = { data: _cleanData(se.followUp.reply && se.followUp.reply.data) };
            if (e.followUp.reply.data.type === "position") e.followUp.reply.data = { type: "none" };
            e.followUp.cmd = (typeof se.followUp.cmd === "number") ? se.followUp.cmd : "auto";
            e.followUp.cmdResolved = (typeof se.followUp.cmdResolved === "number") ? se.followUp.cmdResolved : null;
            if (e.kind === "ping") e.kind = "ranging";
        }
        if (se.rewrite && typeof se.rewrite === "object") {
            var tmp = { exchanges: [e], nextId: 0 };
            e = exchange(setRewrite(tmp, e.id, se.rewrite), e.id);
        }
        var lsn = Array.isArray(se.listeners) ? se.listeners : [];
        for (var q = 0; q < lsn.length; ++q)
            if (lsn[q] && lifeline(st, lsn[q].lifeline) && lsn[q].lifeline !== e.from)
                e.listeners.push({ lifeline: lsn[q].lifeline, hearResponses: !!lsn[q].hearResponses });
        st.exchanges.push(e);
    }

    var cyc = o.cycle || {};
    st.cycle.stepMode = STEP_MODES.indexOf(cyc.stepMode) >= 0 ? cyc.stepMode : "auto";
    st.cycle.extraMarginMs = Math.max(0, cyc.extraMarginMs | 0);
    st.cycle.periodMs = typeof cyc.periodMs === "number" ? clampPeriod(cyc.periodMs) : DEFAULT_PERIOD_MS;
    st.cycle.repeat = cyc.repeat !== false;
    var steps = Array.isArray(cyc.steps) ? cyc.steps : [];
    for (var s = 0; s < steps.length; ++s) {
        var ss = steps[s];
        var sid = takeId(ss && ss.id);
        if (sid < 0 || !exchange(st, ss.exchange)) { problems.push({ code: "stepDropped", detail: String(s) }); continue; }
        st.cycle.steps.push({ id: sid, exchange: ss.exchange, on: ss.on !== false,
                              durationMs: (typeof ss.durationMs === "number" && ss.durationMs > 0) ? ss.durationMs : "auto" });
    }

    var v = o.view || {};
    st.view.order = Array.isArray(v.order) ? v.order.filter(function (id) { return !!lifeline(st, id); }) : [];
    st.view.msPerPx = typeof v.msPerPx === "number" ? v.msPerPx : 4;
    if (st.nextId < maxId) st.nextId = maxId;
    return { state: st, problems: problems, readOnly: readOnly, error: null };
}

/** Stable hash of a network's content, for the dirty mark. */
function contentHash(st) {
    var s = serialize(st, { withBindings: true });
    var h = 0x811c9dc5;
    for (var i = 0; i < s.length; ++i) {
        h ^= s.charCodeAt(i);
        h = (h + ((h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24))) >>> 0;
    }
    return ("00000000" + h.toString(16)).slice(-8);
}

// ── templates ────────────────────────────────────────────────────────────────
function _head(n, label) {
    var h = newLifeline(n, "head");
    h.label = label || "Head";
    n.lifelines.push(h);
    return h;
}
function _beacon(n, addr, label, plannedM) {
    var b = newLifeline(n, "beacon", addr);
    b.label = label || ("Beacon " + addr);
    if (typeof plannedM === "number") b.plannedRangeM = plannedM;
    n.lifelines.push(b);
    return b;
}
function _exchange(n, from, to, kind) {
    var e = newExchange(n, from.id, to.id, kind);
    n.exchanges.push(e);
    n.cycle.steps.push(newStep(n, e.id));
    return e;
}
function _finish(n) {
    n.view.order = n.lifelines.map(function (l) { return l.id; });
    return n;
}

/**
 * Generated networks. kind: ranging | telemetry | command | position | pinger | swarm |
 * bench | benchDelayed. p: { addresses: [..], rangeM, maxRangeM, count, extraRangeM }.
 */
function template(kind, p) {
    var opt = p || {};
    var addrs = (Array.isArray(opt.addresses) && opt.addresses.length) ? _cleanAddrs(opt.addresses) : [1, 2];
    var n = emptyState(opt.name || "");
    if (typeof opt.maxRangeM === "number") n.maxRangeM = clampRange(opt.maxRangeM);
    var planned = (typeof opt.rangeM === "number") ? opt.rangeM : null;
    var h, i, b, e;
    switch (kind) {
    case "telemetry":
        n.name = n.name || "Telemetry";
        h = _head(n);
        var fmt = { id: _next(n), name: "Status", kind: "fields",
                    fields: [ { name: "code", bits: 8, type: "uint" },
                              { name: "battery", bits: 8, type: "uint", scale: 0.1, unit: "V" } ] };
        n.formats.push(fmt);
        for (i = 0; i < addrs.length; ++i) {
            b = _beacon(n, addrs[i], null, planned);
            e = _exchange(n, h, b, "ranging");
            e.response.data = { type: "format", format: fmt.id, values: { code: 1, battery: 12.0 } };
        }
        return _finish(n);
    case "command":
        n.name = n.name || "Command with acknowledge";
        h = _head(n);
        b = _beacon(n, addrs[0], null, planned);
        e = _exchange(n, h, b, "ranging");
        e.label = "Command";
        e.request.data = { type: "bytes", hex: "5A" };
        e.response.data = { type: "bytes", hex: "A5" };
        return _finish(n);
    case "position":
        n.name = n.name || "Position transfer";
        h = _head(n);
        for (i = 0; i < addrs.length; ++i) {
            b = _beacon(n, addrs[i], null, planned);
            e = _exchange(n, h, b, "ranging");
            e.followUp = _newFollowUp();
        }
        return _finish(n);
    case "pinger":
        n.name = n.name || "Pinger and listeners";
        h = _head(n, "Pinger");
        var target = newLifeline(n, "beacon", addrs[0]);
        target.label = "Listener " + addrs[0];
        target.respond = "off";
        n.lifelines.push(target);
        e = _exchange(n, h, target, "ping");
        for (i = 1; i < addrs.length; ++i) {
            var lst = newLifeline(n, "beacon", addrs[i]);
            lst.label = "Listener " + addrs[i];
            lst.respond = "off";
            n.lifelines.push(lst);
            e.listeners.push({ lifeline: lst.id, hearResponses: false });
        }
        return _finish(n);
    case "swarm":
        n.name = n.name || "Swarm: interrogators, one transponder";
        var usv = newLifeline(n, "beacon", 0);
        usv.label = "USV";
        usv.filter = addrs.slice();
        n.lifelines.push(usv);
        for (i = 0; i < addrs.length; ++i) {
            var auv = newLifeline(n, "head");
            auv.label = "AUV " + (i + 1);
            auv.address = addrs[i];
            auv.filter = addrs.slice();
            auv.monitor.hearResponsesWhenIdle = true;
            n.lifelines.push(auv);
            e = newExchange(n, auv.id, usv.id, "ranging");
            e.to = { address: addrs[i] };
            n.exchanges.push(e);
            n.cycle.steps.push(newStep(n, e.id));
        }
        n.cycle.stepMode = "uniform";
        return _finish(n);
    case "bench":
        n.name = n.name || "Bench pair";
        n.maxRangeM = 20;
        h = _head(n);
        b = _beacon(n, addrs[0], null, 1);
        _exchange(n, h, b, "ranging");
        return _finish(n);
    case "benchDelayed": {
        var extraM = (typeof opt.extraRangeM === "number" && opt.extraRangeM > 0) ? opt.extraRangeM : 300;
        n.name = n.name || "Bench pair, delayed beacon";
        n.maxRangeM = clampRange(extraM + 100);
        h = _head(n);
        b = _beacon(n, addrs[0], null, extraM);
        _exchange(n, h, b, "ranging");
        return _finish(n);
    }
    default:
        n.name = n.name || "Ranging";
        h = _head(n);
        for (i = 0; i < addrs.length; ++i) {
            b = _beacon(n, addrs[i], null, planned);
            _exchange(n, h, b, "ranging");
        }
        return _finish(n);
    }
}

/** A head bound to `binding` ranging `addresses`; for Quick interrogate. */
function quickNetwork(binding, addresses, maxRangeM, label) {
    var n = template("ranging", { addresses: addresses, maxRangeM: maxRangeM,
                                  name: "Quick — SN " + (binding ? binding.serial : "?") });
    if (binding) n = bindLifeline(n, n.lifelines[0].id, binding);
    if (label) n.lifelines[0].label = label;
    return n;
}

/** Adds a head bound to `binding` with ranging exchanges into an existing network. */
function addQuickHead(st, binding, addresses) {
    var n = clone(st);
    var h = newLifeline(n, "head");
    h.label = "Head SN " + (binding ? binding.serial : "?");
    n.lifelines.push(h);
    var addrs = _cleanAddrs(addresses || []);
    for (var i = 0; i < addrs.length; ++i) {
        var t = null;
        for (var k = 0; k < n.lifelines.length; ++k)
            if (n.lifelines[k].address === addrs[i] && n.lifelines[k].id !== h.id) { t = n.lifelines[k]; break; }
        if (!t) t = _beacon(n, addrs[i]);
        _exchange(n, h, t, "ranging");
    }
    n = bindLifeline(n, h.id, binding);
    n.view.order = orderedLifelines(n).map(function (x) { return x.id; });
    return { state: n, headId: h.id };
}

// ── migration from the command plan (UsblPlanLogic state, blob v8) ──────────
function _bytesData(hex) {
    var h = String(hex || "").replace(/[^0-9a-fA-F]/g, "");
    return h.length >= 2 ? { type: "bytes", hex: h.substring(0, h.length - (h.length % 2)).toUpperCase() } : { type: "none" };
}

/**
 * Converts a deserialized command plan into a network.
 * plan: UsblPlanLogic state; sched: { dwellMs, fireOnPps }. The old Dwell becomes the cycle's period.
 * Returns { state, notes: [codes] }.
 */
function fromPlan(plan, sched) {
    var n = emptyState("Migrated plan");
    var notes = [];
    if (!plan || !Array.isArray(plan.nodes)) return { state: n, notes: ["emptyPlan"] };
    if (typeof plan.maxRangeMm === "number" && plan.maxRangeMm > 0) n.maxRangeM = clampRange(plan.maxRangeMm / 1000);
    n.transmit = (sched && sched.fireOnPps) ? "pps" : "immediately";
    var h = _head(n);
    var groupOf = function (id) {
        for (var i = 0; i < (plan.groups || []).length; ++i) if (plan.groups[i].id === id) return plan.groups[i];
        return null;
    };
    for (var k = 0; k < plan.nodes.length; ++k) {
        var node = plan.nodes[k];
        if (node.fromFile) continue;
        var addr = (typeof node.addr === "number" && node.addr >= 0 && node.addr < ADDRESS_COUNT) ? node.addr : null;
        if (addr === null) { notes.push("addressOutOfRange"); continue; }
        var b = newLifeline(n, "beacon", addr);
        b.label = "Beacon " + addr;
        b.active = !!node.active;
        n.lifelines.push(b);
        var refs = (node.refs && node.refs.length) ? node.refs : [{ group: -1, cmd: 0, on: true }];
        var seen = {};
        for (var r = 0; r < refs.length; ++r) {
            var ref = refs[r], g = groupOf(ref.group);
            var key = String(ref.group) + ":" + String(ref.cmd);
            var e = seen[key];
            if (!e) {
                e = newExchange(n, h.id, b.id, "ranging");
                e.cmd = (typeof ref.cmd === "number") ? ref.cmd : 0;
                var snd = g && g.ini ? g.ini.send : null;
                if (snd) {
                    if (snd.reply === 0) e.kind = "ping";
                    else if (typeof snd.reply === "number" && snd.reply > 0) e.maxRange = clampRange(snd.reply / 1000);
                    if (snd.fn === "bits") e.request.data = _bytesData(snd.payload);
                    else if (snd.fn === "llgeo") notes.push("pingPositionDropped");
                }
                var rq = g && g.tr ? g.tr.request : null;
                if (rq && rq.send) {
                    if (rq.send.fmt === "bits") e.response.data = _bytesData(rq.send.payload);
                    else notes.push("responsePositionDropped");
                }
                if (rq && rq.adv) {
                    var rw = { address: rq.adv.addrAction === "Replacement" ? rq.adv.addrRepl : null,
                               cmd: rq.adv.cmdIdAction === "Replacement" ? rq.adv.cmdIdRepl : null,
                               asRequest: rq.adv.eventAction === "Same" };
                    if (rw.address !== null || rw.cmd !== null || rw.asRequest) e.rewrite = rw;
                }
                var rs = g && g.ini ? g.ini.reply : null;
                if (rs && rs.send) {
                    e.followUp = _newFollowUp();
                    e.followUp.data = rs.send.fmt === "llgeo" ? { type: "position", source: "headNav" } : _bytesData(rs.send.payload);
                    if (rs.adv && rs.adv.cmdIdAction === "Replacement") e.followUp.cmd = rs.adv.cmdIdRepl;
                    else notes.push("followUpWithoutReplacement");
                    if (e.kind === "ping") e.kind = "ranging";
                }
                n.exchanges.push(e);
                seen[key] = e;
            }
            var s = newStep(n, e.id);
            s.on = ref.on !== false;
            n.cycle.steps.push(s);
        }
    }
    _finish(n);
    var dwell = sched && typeof sched.dwellMs === "number" ? sched.dwellMs : 0;
    if (dwell > 0) {
        n.cycle.stepMode = "period";
        n.cycle.periodMs = clampPeriod(dwell);
    }
    return { state: n, notes: notes };
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        KIND: KIND, FORMAT_VERSION: FORMAT_VERSION, ADDRESS_COUNT: ADDRESS_COUNT, CMD_COUNT: CMD_COUNT,
        MIN_RANGE_M: MIN_RANGE_M, MAX_RANGE_M: MAX_RANGE_M, DEFAULT_RANGE_M: DEFAULT_RANGE_M,
        MIN_PERIOD_MS: MIN_PERIOD_MS, MAX_PERIOD_MS: MAX_PERIOD_MS, DEFAULT_PERIOD_MS: DEFAULT_PERIOD_MS,
        STEP_MODES: STEP_MODES, clampPeriod: clampPeriod,
        LIFELINE_KINDS: LIFELINE_KINDS, DATA_TYPES: DATA_TYPES,
        clone: clone, clampRange: clampRange, emptyState: emptyState, defaultMonitor: defaultMonitor,
        byId: byId, indexById: indexById, lifeline: lifeline, exchange: exchange, step: step,
        usedAddresses: usedAddresses, freeAddress: freeAddress, targetAddress: targetAddress,
        targetLifeline: targetLifeline, effectiveRangeM: effectiveRangeM, initiators: initiators,
        isInitiator: isInitiator, orderedLifelines: orderedLifelines, bindingOf: bindingOf,
        lifelineBySerial: lifelineBySerial,
        setNetworkField: setNetworkField, addLifeline: addLifeline, removeLifeline: removeLifeline,
        setLifelineField: setLifelineField, bindLifeline: bindLifeline, moveLifeline: moveLifeline,
        sameDevice: sameDevice, assignDevices: assignDevices,
        addExchange: addExchange,
        removeExchange: removeExchange, setExchangeField: setExchangeField,
        setExchangeData: setExchangeData, setFollowUp: setFollowUp, setFollowUpCmd: setFollowUpCmd,
        setRewrite: setRewrite, setListener: setListener,
        addStep: addStep, removeStep: removeStep, moveStep: moveStep, setStepField: setStepField,
        duplicateStep: duplicateStep,
        addFormat: addFormat, setFormat: setFormat, removeFormat: removeFormat,
        recordResolvedCmds: recordResolvedCmds,
        serialize: serialize, deserialize: deserialize, contentHash: contentHash,
        template: template, quickNetwork: quickNetwork, addQuickHead: addQuickHead, fromPlan: fromPlan
    };
}

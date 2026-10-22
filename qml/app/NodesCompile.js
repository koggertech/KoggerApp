.pragma library
.import "NodesModel.js" as Model
.import "NodesPayload.js" as Payload
.import "NodesTiming.js" as Timing

var EVENT_ON_REQUEST = 1;
var EVENT_ON_RESPONSE = 2;
var V3_ALWAYS = 4294967295;
var V3_WINDOW_MAX_US = 4294967294;
var TRIGGER_FOREVER_US = 4294967295;
var ADDR_DISABLED = 255;
var SOURCE_CMD_ONLY = 0;
var SOURCE_REGULAR = 4;

function defaultSlot(event, cmd) {
    return { cmd: cmd, event: event, configured: false,
             recvFn: 0, recvBits: 0, sendFn: 0, sendHex: "", sendBits: 0,
             eventAction: 0, cmdIdAction: 0, cmdIdRepl: 0, addrAction: 0, addrRepl: 0 };
}

function _sig(cfg) {
    return [cfg.recvFn, cfg.recvBits, cfg.sendFn, cfg.sendHex, cfg.sendBits, cfg.eventAction,
            cfg.cmdIdAction, cfg.cmdIdRepl, cfg.addrAction, cfg.addrRepl].join("|");
}

function _recv(data, formats) {
    var fn = Payload.wireFunction(data);
    return { fn: fn, bits: fn === 1 ? Payload.bitsOf(data, formats) : 0 };
}

function _send(data, formats) {
    var fn = Payload.wireFunction(data);
    if (fn !== 1) return { fn: fn, hex: "", bits: 0 };
    var enc = Payload.encode(data, formats);
    return { fn: 1, hex: enc.hex, bits: enc.bits };
}

function _slot(event, cmd, parts) {
    var s = defaultSlot(event, cmd);
    s.configured = true;
    for (var k in parts) s[k] = parts[k];
    return s;
}

/** Slot claims one exchange makes for a cmd pair: [{ lifeline, event, cmd, cfg }]. */
function claimsFor(st, ex, c, c2) {
    var out = [], f = st.formats;
    var tgt = Model.targetLifeline(st, ex);
    var reqData = ex.request.data, respData = ex.response.data;
    var ranging = ex.kind !== "ping";
    var rq = _recv(reqData, f), rs = _send(respData, f), rr = _recv(respData, f);

    if (tgt && tgt.id !== ex.from) {
        var rw = ex.rewrite || {};
        var tcfg = { recvFn: rq.fn, recvBits: rq.bits,
                     sendFn: ranging ? rs.fn : 0, sendHex: ranging ? rs.hex : "", sendBits: ranging ? rs.bits : 0,
                     eventAction: rw.asRequest ? 1 : 0,
                     cmdIdAction: typeof rw.cmd === "number" ? 1 : 0, cmdIdRepl: typeof rw.cmd === "number" ? rw.cmd : 0,
                     addrAction: typeof rw.address === "number" ? 1 : 0, addrRepl: typeof rw.address === "number" ? rw.address : 0 };
        out.push({ lifeline: tgt.id, event: EVENT_ON_REQUEST, cmd: c, cfg: _slot(EVENT_ON_REQUEST, c, tcfg) });
    }
    if (ranging) {
        var icfg = { recvFn: rr.fn, recvBits: rr.bits };
        if (ex.followUp && typeof c2 === "number") {
            var fs = _send(ex.followUp.data, f);
            icfg.sendFn = fs.fn; icfg.sendHex = fs.hex; icfg.sendBits = fs.bits;
            icfg.cmdIdAction = 1; icfg.cmdIdRepl = c2;
        }
        out.push({ lifeline: ex.from, event: EVENT_ON_RESPONSE, cmd: c, cfg: _slot(EVENT_ON_RESPONSE, c, icfg) });
    }
    for (var i = 0; i < ex.listeners.length; ++i) {
        var L = ex.listeners[i];
        out.push({ lifeline: L.lifeline, event: EVENT_ON_REQUEST, cmd: c,
                   cfg: _slot(EVENT_ON_REQUEST, c, { recvFn: rq.fn, recvBits: rq.bits }) });
        if (L.hearResponses && ranging)
            out.push({ lifeline: L.lifeline, event: EVENT_ON_RESPONSE, cmd: c,
                       cfg: _slot(EVENT_ON_RESPONSE, c, { recvFn: rr.fn, recvBits: rr.bits }) });
    }
    if (ex.followUp && typeof c2 === "number" && ranging) {
        var fr = _recv(ex.followUp.data, f);
        var fRecvBits = fr.fn === 2 ? 0 : fr.bits;
        var frs = _send(ex.followUp.reply.data, f), frr = _recv(ex.followUp.reply.data, f);
        if (tgt && tgt.id !== ex.from)
            out.push({ lifeline: tgt.id, event: EVENT_ON_REQUEST, cmd: c2,
                       cfg: _slot(EVENT_ON_REQUEST, c2, { recvFn: fr.fn, recvBits: fRecvBits,
                                                          sendFn: frs.fn, sendHex: frs.hex, sendBits: frs.bits }) });
        out.push({ lifeline: ex.from, event: EVENT_ON_RESPONSE, cmd: c2,
                   cfg: _slot(EVENT_ON_RESPONSE, c2, { recvFn: frr.fn, recvBits: frr.bits }) });
    }
    return out;
}

function _key(l, ev, c) { return l + "/" + ev + "/" + c; }

function _compatible(table, claims) {
    var local = {};
    for (var i = 0; i < claims.length; ++i) {
        var cl = claims[i], k = _key(cl.lifeline, cl.event, cl.cmd), sg = _sig(cl.cfg);
        if (local[k] !== undefined && local[k] !== sg) return false;
        local[k] = sg;
        if (table[k] && table[k].sig !== sg) return false;
    }
    return true;
}

function _shares(table, claims) {
    for (var i = 0; i < claims.length; ++i)
        if (table[_key(claims[i].lifeline, claims[i].event, claims[i].cmd)]) return true;
    return false;
}

function _commitClaims(table, claims, exId) {
    for (var i = 0; i < claims.length; ++i) {
        var cl = claims[i], k = _key(cl.lifeline, cl.event, cl.cmd);
        if (!table[k]) table[k] = { sig: _sig(cl.cfg), cfg: cl.cfg, lifeline: cl.lifeline,
                                    event: cl.event, cmd: cl.cmd, exchanges: [] };
        if (table[k].exchanges.indexOf(exId) < 0) table[k].exchanges.push(exId);
    }
}

function _range() { var out = []; for (var c = 0; c < Model.CMD_COUNT; ++c) out.push(c); return out; }

/**
 * Allocates cmd ids and builds every slot claim.
 * Order of preference: the pinned or previously resolved pair, then the lowest pair that shares
 * an identical slot already in use, then the lowest free pair.
 * Returns { resolved: {exId: {cmd, followCmd, conflict}}, table, problems: [{code, exchange}] }.
 */
function allocate(st) {
    var table = {}, resolved = {}, problems = [];
    for (var i = 0; i < st.exchanges.length; ++i) {
        var ex = st.exchanges[i];
        var follow = !!(ex.followUp && ex.kind !== "ping");
        var pinC = typeof ex.cmd === "number" ? ex.cmd : null;
        var pinF = follow && typeof ex.followUp.cmd === "number" ? ex.followUp.cmd : null;
        var prefC = pinC !== null ? pinC : (typeof ex.cmdResolved === "number" ? ex.cmdResolved : null);
        var prefF = follow ? (pinF !== null ? pinF
                                            : (typeof ex.followUp.cmdResolved === "number" ? ex.followUp.cmdResolved : null))
                           : null;
        var pick = null;

        var followOptions = function (c, preferred) {
            if (!follow) return [null];
            if (pinF !== null) return [pinF];
            var out = [];
            if (preferred !== null && preferred !== c) out.push(preferred);
            var all = _range();
            for (var k = 0; k < all.length; ++k) if (all[k] !== c && out.indexOf(all[k]) < 0) out.push(all[k]);
            return out;
        };

        if (prefC !== null) {
            var fo = followOptions(prefC, prefF);
            for (var a = 0; a < fo.length && !pick; ++a) {
                if (follow && fo[a] === prefC) continue;
                var cl = claimsFor(st, ex, prefC, fo[a]);
                if (_compatible(table, cl)) pick = { c: prefC, c2: fo[a], claims: cl };
            }
        }
        for (var pass = 0; pass < 2 && !pick && pinC === null; ++pass) {
            var cs = _range();
            for (var b = 0; b < cs.length && !pick; ++b) {
                var fo2 = followOptions(cs[b], prefF);
                for (var d = 0; d < fo2.length && !pick; ++d) {
                    if (follow && fo2[d] === cs[b]) continue;
                    var cl2 = claimsFor(st, ex, cs[b], fo2[d]);
                    if (!_compatible(table, cl2)) continue;
                    if (pass === 0 && !_shares(table, cl2)) continue;
                    pick = { c: cs[b], c2: fo2[d], claims: cl2 };
                }
            }
        }
        if (!pick) {
            var fc = pinC !== null ? pinC : 0;
            var fc2 = follow ? (pinF !== null ? pinF : (fc + 1) % Model.CMD_COUNT) : null;
            problems.push({ code: (pinC !== null || pinF !== null) ? "cmdConflict" : "tooManyCmds", exchange: ex.id });
            resolved[ex.id] = { cmd: fc, followCmd: fc2, conflict: true };
            continue;
        }
        _commitClaims(table, pick.claims, ex.id);
        resolved[ex.id] = { cmd: pick.c, followCmd: pick.c2, conflict: false };
    }
    return { resolved: resolved, table: table, problems: problems };
}

function _v3(l) {
    if (l.respond === "always") return V3_ALWAYS;
    if (l.respond && typeof l.respond.windowS === "number")
        return Math.min(V3_WINDOW_MAX_US, Math.max(1, Math.round(l.respond.windowS)) * 1000000);
    return 0;
}

/** Addresses the lifeline's request filter holds (before 0xFF padding). */
function filterOf(st, l) {
    if (Array.isArray(l.filter)) return l.filter.slice();
    var set = {}, out = [];
    var add = function (a) { if (typeof a === "number" && a >= 0 && a < Model.ADDRESS_COUNT && !set[a]) { set[a] = true; out.push(a); } };
    if (l.respond !== "off" && typeof l.address === "number") add(l.address);
    for (var i = 0; i < st.exchanges.length; ++i) {
        var ex = st.exchanges[i];
        for (var k = 0; k < ex.listeners.length; ++k)
            if (ex.listeners[k].lifeline === l.id) add(Model.targetAddress(st, ex));
        if (ex.to && ex.to.lifeline === l.id) add(l.address);
    }
    return out.sort(function (x, y) { return x - y; });
}

function _hash(s) {
    var h = 0x811c9dc5;
    for (var i = 0; i < s.length; ++i) {
        h ^= s.charCodeAt(i);
        h = (h + ((h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24))) >>> 0;
    }
    return ("00000000" + h.toString(16)).slice(-8);
}

/** Complete write set of one lifeline, in wire order. */
function deviceState(st, l, table) {
    var slots = [];
    var ev, c;
    for (ev = EVENT_ON_REQUEST; ev <= EVENT_ON_RESPONSE; ++ev)
        for (c = 0; c < Model.CMD_COUNT; ++c) {
            var t = table[_key(l.id, ev, c)];
            slots.push(t ? t.cfg : defaultSlot(ev, c));
        }
    var filt = filterOf(st, l);
    var v4 = filt.slice(0, 8);
    while (v4.length < 8) v4.push(ADDR_DISABLED);
    var hearIdle = !!l.monitor.hearResponsesWhenIdle;
    for (var i = 0; i < st.exchanges.length && !hearIdle; ++i)
        for (var k = 0; k < st.exchanges[i].listeners.length; ++k)
            if (st.exchanges[i].listeners[k].lifeline === l.id && st.exchanges[i].listeners[k].hearResponses) hearIdle = true;
    var v7 = { suppressSelfResponseUs: Math.round(l.monitor.blindAfterResponseMs * 1000),
               suppressSelfRequestUs: Math.round(l.monitor.blindAfterRequestMs * 1000),
               receiveResponseInIdle: hearIdle };
    var v3 = _v3(l);
    var writes = [];
    writes.push({ kind: "v0", ver: 0, source: SOURCE_CMD_ONLY, periodUs: 0 });
    for (var s = 0; s < slots.length; ++s) {
        var w = { kind: "v6", ver: 6 };
        for (var key in slots[s]) w[key] = slots[s][key];
        writes.push(w);
    }
    writes.push({ kind: "v7", ver: 7, suppressSelfResponseUs: v7.suppressSelfResponseUs,
                  suppressSelfRequestUs: v7.suppressSelfRequestUs, receiveResponseInIdle: v7.receiveResponseInIdle });
    writes.push({ kind: "v4", ver: 4, addresses: filt.slice(), raw: v4 });
    writes.push({ kind: "v3", ver: 3, timeoutUs: v3 });
    var fp = _hash(JSON.stringify(writes));
    return { lifelineId: l.id, slots: slots, filter: filt, v4: v4, v3: v3, v7: v7,
             writes: writes, fingerprint: fp,
             configuredSlots: slots.filter(function (x) { return x.configured; }).length };
}

function _specFor(st, ex) {
    var f = st.formats;
    var spec = { kind: ex.kind, maxRangeM: Model.effectiveRangeM(st, ex),
                 reqBits: Payload.bitsOf(ex.request.data, f),
                 respBits: ex.kind === "ping" ? 0 : Payload.bitsOf(ex.response.data, f),
                 followUp: null };
    if (ex.followUp && ex.kind !== "ping")
        spec.followUp = { bits: Payload.bitsOf(ex.followUp.data, f),
                          replyBits: Payload.bitsOf(ex.followUp.reply.data, f) };
    return spec;
}

/** Timing spec of an exchange, for NodesTiming. */
function timingSpec(st, ex) { return _specFor(st, ex); }

/** Required step time of an exchange (ms); also the migration's measure. */
function requiredFor(st, ex) { return Timing.requiredMs(_specFor(st, ex)); }

function _triggerUs(st, initiator) {
    var mode = initiator && initiator.executor ? initiator.executor.mode : "host";
    if (mode === "pps") return TRIGGER_FOREVER_US;
    if (mode === "periodic") return 0;
    return st.transmit === "pps" ? TRIGGER_FOREVER_US : 0;
}

/** Target id the engine's reply attribution keys on: the lifeline, or a synthetic id for an address. */
function nodeKeyFor(st, ex) {
    var t = Model.targetLifeline(st, ex);
    if (t) return t.id;
    return 1000000 + Math.max(0, Model.targetAddress(st, ex));
}

/**
 * The whole network compiled.
 * { resolved, devices: {id: deviceState}, steps: [...], totalMs, problems, targets: [{id, addr, active, lifeline}] }
 */
function compile(st) {
    var alloc = allocate(st);
    var devices = {};
    for (var i = 0; i < st.lifelines.length; ++i) {
        var l = st.lifelines[i];
        if (l.kind === "external") continue;
        devices[l.id] = deviceState(st, l, alloc.table);
    }
    var specs = [], rows = [];
    for (var s = 0; s < st.cycle.steps.length; ++s) {
        var stp = st.cycle.steps[s];
        var ex = Model.exchange(st, stp.exchange);
        if (!ex) continue;
        var ini = Model.lifeline(st, ex.from);
        var spec = _specFor(st, ex);
        var req = Timing.requiredMs(spec);
        var r = alloc.resolved[ex.id] || { cmd: 0, followCmd: null };
        var enc = Payload.encode(ex.request.data, st.formats);
        var tgt = Model.targetLifeline(st, ex);
        var activeTarget = !tgt || tgt.active !== false;
        specs.push({ requiredMs: req, fixedMs: typeof stp.durationMs === "number" ? stp.durationMs : null,
                     on: stp.on !== false && activeTarget && !!ini && ini.active !== false });
        rows.push({
            stepId: stp.id, exchangeId: ex.id, index: s,
            initiatorId: ex.from, targetId: tgt ? tgt.id : -1, nodeId: nodeKeyFor(st, ex),
            addr: Model.targetAddress(st, ex), cmd: r.cmd, followCmd: ex.followUp ? r.followCmd : null,
            kind: ex.kind, replyMm: Model.effectiveRangeM(st, ex) * 1000,
            fn: enc.bits > 0 ? 1 : 0, payloadHex: enc.hex, payloadBits: enc.bits,
            triggerUs: _triggerUs(st, ini), requiredMs: req, spec: spec,
            executor: ini && ini.executor ? ini.executor.mode : "host",
            on: stp.on !== false, targetActive: activeTarget
        });
    }
    var cyc = Timing.cycleDurations(specs, st.cycle.stepMode, st.cycle.extraMarginMs, st.cycle.periodMs);
    for (var k = 0; k < rows.length; ++k) {
        var d = cyc.steps[k].durationMs;
        if (rows[k].executor === "pps") d = Math.max(1000, Math.ceil(d / 1000) * 1000);
        rows[k].durationMs = d;
        rows[k].autoMs = cyc.steps[k].autoMs;
        rows[k].startMs = cyc.steps[k].startMs;
        rows[k].enabled = cyc.steps[k].on;
    }
    var total = 0;
    for (var t = 0; t < rows.length; ++t) if (rows[t].enabled) { rows[t].startMs = total; total += rows[t].durationMs; }

    var targets = [], seen = {};
    for (var e = 0; e < st.exchanges.length; ++e) {
        var x = st.exchanges[e], key = nodeKeyFor(st, x);
        if (seen[key]) continue;
        seen[key] = true;
        var tl = Model.targetLifeline(st, x);
        targets.push({ id: key, addr: Model.targetAddress(st, x), active: !tl || tl.active !== false,
                       lifeline: tl ? tl.id : -1, refs: [] });
    }
    for (var q = 0; q < st.lifelines.length; ++q) {
        var ll = st.lifelines[q];
        if (ll.kind === "head" || seen[ll.id] || typeof ll.address !== "number") continue;
        seen[ll.id] = true;
        targets.push({ id: ll.id, addr: ll.address, active: ll.active !== false, lifeline: ll.id, refs: [] });
    }
    return { resolved: alloc.resolved, table: alloc.table, devices: devices, steps: rows,
             totalMs: total, problems: alloc.problems, targets: targets };
}

function _dataLeg(out, ex, leg, data, formats, lifelineId, event, cmd, addr) {
    if (Payload.wireFunction(data) !== 1 || typeof cmd !== "number" || cmd < 0) return;
    var enc = Payload.encode(data, formats);
    if (enc.bits <= 0) return;
    out.push({ exchangeId: ex.id, leg: leg, lifelineId: lifelineId, event: event, cmd: cmd, addr: addr,
               hex: enc.hex, bits: enc.bits, data: data });
}

/**
 * Every place a bit-array payload is decoded, so a received ModemSolution can be attributed:
 * [{ exchangeId, leg: request|response|followUp|followUpReply, lifelineId, event, cmd, addr, hex, bits, data }].
 * `addr` and `cmd` are those of the decoded frame, which is what the device reports;
 * `hex`/`bits` are the configured payload.
 */
function dataLegs(st, compiled) {
    var out = [], f = st.formats;
    for (var i = 0; i < st.exchanges.length; ++i) {
        var ex = st.exchanges[i];
        var r = compiled.resolved[ex.id];
        if (!r) continue;
        var tgt = Model.targetLifeline(st, ex);
        var hasTarget = !!tgt && tgt.id !== ex.from;
        var addr = Model.targetAddress(st, ex);
        var ranging = ex.kind !== "ping";
        var rw = ex.rewrite || {};
        var replyAddr = typeof rw.address === "number" ? rw.address : addr;
        var replyCmd = typeof rw.cmd === "number" ? rw.cmd : r.cmd;
        if (hasTarget) _dataLeg(out, ex, "request", ex.request.data, f, tgt.id, EVENT_ON_REQUEST, r.cmd, addr);
        for (var k = 0; k < ex.listeners.length; ++k) {
            var L = ex.listeners[k];
            _dataLeg(out, ex, "request", ex.request.data, f, L.lifeline, EVENT_ON_REQUEST, r.cmd, addr);
            if (ranging && L.hearResponses)
                _dataLeg(out, ex, "response", ex.response.data, f, L.lifeline, EVENT_ON_RESPONSE, replyCmd, replyAddr);
        }
        if (!ranging) continue;
        _dataLeg(out, ex, "response", ex.response.data, f, ex.from, EVENT_ON_RESPONSE, replyCmd, replyAddr);
        if (ex.followUp && typeof r.followCmd === "number") {
            if (hasTarget) _dataLeg(out, ex, "followUp", ex.followUp.data, f, tgt.id, EVENT_ON_REQUEST, r.followCmd, replyAddr);
            _dataLeg(out, ex, "followUpReply", ex.followUp.reply.data, f, ex.from, EVENT_ON_RESPONSE, r.followCmd, replyAddr);
        }
    }
    return out;
}

/** Data legs a device on `lifelineId` reports with this event, frame address and cmd. */
function legsAt(legs, lifelineId, event, addr, cmd) {
    return legs.filter(function (g) {
        return g.lifelineId === lifelineId && g.event === event && g.addr === addr && g.cmd === cmd;
    });
}

/** Steps an initiator runs, in cycle order, enabled only. */
function stepsOf(compiled, initiatorId) {
    var out = [];
    for (var i = 0; i < compiled.steps.length; ++i) {
        var s = compiled.steps[i];
        if (!s.enabled) continue;
        if (initiatorId === undefined || initiatorId === null || s.initiatorId === initiatorId) out.push(s);
    }
    return out;
}

/** Frames of a lifeline as display rows for the Frames view. */
function framesView(dev) {
    var out = [];
    if (!dev) return out;
    for (var i = 0; i < dev.writes.length; ++i) {
        var w = dev.writes[i], text;
        switch (w.kind) {
        case "v0": text = "TriggerControl source=" + w.source + (w.periodUs ? " period=" + w.periodUs + " us" : ""); break;
        case "v3": text = "TransponderEnable timeout=" + (w.timeoutUs === V3_ALWAYS ? "always" : w.timeoutUs + " us"); break;
        case "v4": text = "AddressFilter [" + w.raw.map(function (a) { return a === ADDR_DISABLED ? "FF" : String(a); }).join(", ") + "]"; break;
        case "v7": text = "Monitor selfResp=" + w.suppressSelfResponseUs + " us selfReq=" + w.suppressSelfRequestUs
                        + " us idle=" + (w.receiveResponseInIdle ? 1 : 0); break;
        default:
            text = "CmdConfig cmd=" + w.cmd + " ev=" + (w.event === 1 ? "request" : "response")
                 + (w.configured ? "" : " (default)")
                 + (w.recvFn ? " recv=" + w.recvFn + "/" + w.recvBits : "")
                 + (w.sendFn ? " send=" + w.sendFn + (w.sendHex ? "/" + w.sendHex + " (" + w.sendBits + " bit)" : "") : "")
                 + (w.cmdIdAction ? " cmd→" + w.cmdIdRepl : "")
                 + (w.addrAction ? " addr→" + w.addrRepl : "")
                 + (w.eventAction ? " sameEvent" : "");
        }
        out.push({ index: i, kind: w.kind, ver: w.ver, text: text, configured: w.kind !== "v6" || w.configured });
    }
    return out;
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        EVENT_ON_REQUEST: EVENT_ON_REQUEST, EVENT_ON_RESPONSE: EVENT_ON_RESPONSE,
        V3_ALWAYS: V3_ALWAYS, TRIGGER_FOREVER_US: TRIGGER_FOREVER_US, ADDR_DISABLED: ADDR_DISABLED,
        SOURCE_CMD_ONLY: SOURCE_CMD_ONLY, SOURCE_REGULAR: SOURCE_REGULAR,
        defaultSlot: defaultSlot, claimsFor: claimsFor, allocate: allocate, filterOf: filterOf,
        deviceState: deviceState, timingSpec: timingSpec, requiredFor: requiredFor,
        nodeKeyFor: nodeKeyFor, compile: compile, dataLegs: dataLegs, legsAt: legsAt,
        stepsOf: stepsOf, framesView: framesView
    };
}

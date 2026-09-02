.pragma library

// Scan configuration: what it is, what it counts to, what has changed since it was sent, and
// what the firmware would refuse. No QML types and no qsTr -- codes and numbers out, words in
// StandBadgeRow.qml, so the vocabulary exists once.
//
// Stateless library: one shared instance for every importer. The node suites read this file
// through tools/qml_test/load_qml_js.mjs, which strips the directive above.

var ORDER_AZ_EL = "azEl"
var ORDER_EL_AZ = "elAz"

var LIMIT_POS = 2000000000
var LIMIT_FIRES = 9999
var LIMIT_CYCLES = 9999
var LIMIT_MS = 60000

function defaultConfig() {
    return { order: ORDER_AZ_EL,
             innerStart: 0, innerEnd: 0, innerStep: 100,
             outerStart: 0, outerEnd: 0, outerStep: 100,
             reverse: false, continuous: false,
             fires: 1, cycles: 1, settleMs: 100, postFireMs: 0 }
}

function _int(v, fallback) {
    var n = Math.round(Number(v))
    return isFinite(n) ? n : fallback
}

function _clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }

function normalizeConfig(raw) {
    var d = defaultConfig()
    if (!raw || typeof raw !== "object")
        return d

    var c = {
        order: (raw.order === ORDER_EL_AZ) ? ORDER_EL_AZ : ORDER_AZ_EL,
        innerStart: _clamp(_int(raw.innerStart, d.innerStart), -LIMIT_POS, LIMIT_POS),
        innerEnd:   _clamp(_int(raw.innerEnd,   d.innerEnd),   -LIMIT_POS, LIMIT_POS),
        innerStep:  _clamp(_int(raw.innerStep,  d.innerStep),  -LIMIT_POS, LIMIT_POS),
        outerStart: _clamp(_int(raw.outerStart, d.outerStart), -LIMIT_POS, LIMIT_POS),
        outerEnd:   _clamp(_int(raw.outerEnd,   d.outerEnd),   -LIMIT_POS, LIMIT_POS),
        outerStep:  _clamp(_int(raw.outerStep,  d.outerStep),  -LIMIT_POS, LIMIT_POS),
        reverse:    !!raw.reverse,
        continuous: !!raw.continuous,
        fires:      _clamp(_int(raw.fires,  d.fires),  0, LIMIT_FIRES),
        cycles:     _clamp(_int(raw.cycles, d.cycles), 1, LIMIT_CYCLES),
        settleMs:   _clamp(_int(raw.settleMs,   d.settleMs),   0, LIMIT_MS),
        postFireMs: _clamp(_int(raw.postFireMs, d.postFireMs), 0, LIMIT_MS)
    }

    if (c.innerStep === 0) c.innerStep = d.innerStep
    if (c.outerStep === 0) c.outerStep = d.outerStep
    if (c.continuous && c.fires > 1) c.fires = 1
    return c
}

function copyConfig(cfg) { return normalizeConfig(cfg) }

function innerAxis(cfg) { return (cfg && cfg.order === ORDER_EL_AZ) ? "el" : "az" }
function outerAxis(cfg) { return (cfg && cfg.order === ORDER_EL_AZ) ? "az" : "el" }

function counts(cfg) {
    var c = normalizeConfig(cfg)
    function span(a, b, s) {
        if (!s) return 1
        return Math.floor(Math.abs(b - a) / Math.abs(s)) + 1
    }
    var i = span(c.innerStart, c.innerEnd, c.innerStep)
    var o = span(c.outerStart, c.outerEnd, c.outerStep)
    return { inner: i, outer: o, total: i * o }
}

var CONFIG_KEYS = ["order", "innerStart", "innerEnd", "innerStep",
                   "outerStart", "outerEnd", "outerStep",
                   "reverse", "continuous", "fires", "cycles", "settleMs", "postFireMs"]

function sameConfig(a, b) {
    if (!a || !b) return false
    for (var i = 0; i < CONFIG_KEYS.length; ++i) {
        var k = CONFIG_KEYS[i]
        if (a[k] !== b[k]) return false
    }
    return true
}

function _differs(cfg, sent, keys) {
    if (!sent) return false
    for (var i = 0; i < keys.length; ++i)
        if (cfg[keys[i]] !== sent[keys[i]]) return true
    return false
}

var SHAPE_KEYS = ["order", "innerStart", "innerEnd", "innerStep",
                  "outerStart", "outerEnd", "outerStep"]

// The marks the badge row draws, as codes and numbers. The words are StandBadgeRow's, so the
// vocabulary exists once and the two cannot drift.
function marks(cfg, sent) {
    var c = normalizeConfig(cfg)
    var n = counts(c)
    var out = []

    out.push({ key: "order", tone: "plain", inner: innerAxis(c), outer: outerAxis(c),
               stale: _differs(c, sent, ["order"]) })
    out.push({ key: "grid", tone: "plain", inner: n.inner, outer: n.outer,
               stale: _differs(c, sent, SHAPE_KEYS) })
    out.push({ key: c.reverse ? "roundTrip" : "reset", tone: c.reverse ? "mode" : "plain",
               stale: _differs(c, sent, ["reverse"]) })
    if (c.continuous)
        out.push({ key: "sweep", tone: "sweep", stale: _differs(c, sent, ["continuous"]) })
    out.push({ key: c.fires === 0 ? "noFire" : "fires", tone: c.fires === 0 ? "warn" : "plain",
               n: c.fires, stale: _differs(c, sent, ["fires"]) })
    if (c.cycles > 1)
        out.push({ key: "cycles", tone: "plain", n: c.cycles, stale: _differs(c, sent, ["cycles"]) })
    out.push({ key: "timing", tone: "plain", settle: c.settleMs,
               postFire: c.continuous ? -1 : c.postFireMs,
               stale: _differs(c, sent, c.continuous ? ["settleMs"] : ["settleMs", "postFireMs"]) })
    return out
}

// The checks the firmware performs, run before a command leaves. Returns a code, not a
// sentence: the panel carries no prose, and the caller decides what to do with a refusal.
//
// ON THE CONFIGURATION AS GIVEN, not on a normalised copy. Normalising first repaired the very
// things this is meant to catch -- the clamp turned every rejectable scan into an acceptable one
// and every branch below became unreachable, so the guard passed everything.
function validate(cfg) {
    if (!cfg || typeof cfg !== "object") return "config"
    if (_int(cfg.innerStep, 0) === 0) return "innerStep"
    if (_int(cfg.outerStep, 0) === 0) return "outerStep"
    if (_int(cfg.cycles, 0) < 1) return "cycles"
    if (cfg.continuous && _int(cfg.fires, 0) > 1) return "firesSweep"
    return ""
}

// Node consumes this via module.exports; QML sees the top-level functions and ignores it.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        ORDER_AZ_EL: ORDER_AZ_EL, ORDER_EL_AZ: ORDER_EL_AZ,
        LIMIT_POS: LIMIT_POS, LIMIT_FIRES: LIMIT_FIRES,
        LIMIT_CYCLES: LIMIT_CYCLES, LIMIT_MS: LIMIT_MS,
        CONFIG_KEYS: CONFIG_KEYS, SHAPE_KEYS: SHAPE_KEYS,
        defaultConfig: defaultConfig, normalizeConfig: normalizeConfig, copyConfig: copyConfig,
        innerAxis: innerAxis, outerAxis: outerAxis, counts: counts,
        sameConfig: sameConfig, marks: marks, validate: validate
    };
}

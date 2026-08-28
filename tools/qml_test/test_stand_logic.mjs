// Behaviour tests for the stand panel: what a scan is, what it counts to, what has changed since
// it was sent, and what the firmware would refuse. No Qt, no window, no device.
//
//   node tools/qml_test/test_stand_logic.mjs
//
// Exit 0 = all pass. Four things are being defended.
//
// First, the FIRMWARE'S OWN REFUSALS, encoded here so they are met before a frame leaves rather
// than as a rejection the operator never sees. The stand reports nothing: a refused Start is
// silence, indistinguishable from a stand that is simply slow to move.
//
// Second, the STALE MARKS. Start is the only write the stand accepts, so between an edit and the
// next Start the form and the device disagree — and with no dirty banner and no readback, the
// struck-through marks are the only place the panel says so. A mark that fails to strike is a
// panel confidently reporting a scan that is not running.
//
// Third, ONE VOCABULARY. StandLogic returns codes; StandBadgeRow.qml owns every word. Two copies
// of a vocabulary become two vocabularies, so the suite asserts over the QML source that the row
// names exactly the keys the logic can return and nothing more.
//
// Fourth, things invisible in both a behaviour test and a screenshot, asserted over SOURCE TEXT:
// that editing a field sends nothing, that the panel kind is hidden without a stand and without a
// device, and that no explanatory prose has crept back into a panel that must not change height
// while it is being read.

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { readFileSync } from "node:fs";
import path from "node:path";

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));
const S = require(path.join(here, "..", "..", "qml", "app", "StandLogic.js"));

const src = (...p) => readFileSync(path.join(here, "..", "..", ...p), "utf8");
const BADGE_ROW = src("qml", "app", "StandBadgeRow.qml");
const PANEL     = src("qml", "app", "StandPanelPopup.qml");
const KIND_STEP = src("qml", "app", "WidgetKindStep.qml");
const STORE     = src("qml", "app", "WorkspaceStore.qml");
const MAIN      = src("qml", "app", "MainWindow.qml");

let pass = 0;
const fails = [];
function ok(name, cond, detail) {
    if (cond) { pass++; console.log("  ok   " + name); }
    else { fails.push(name); console.log(`  FAIL ${name}${detail ? "   -- " + detail : ""}`); }
}
const j = (v) => JSON.stringify(v);
function eq(name, got, want) { ok(name, j(got) === j(want), `got ${j(got)}, want ${j(want)}`); }

// The reference scan: 21 inner points across seven outer steps, round-trip, one fire each.
function ref(over) {
    return Object.assign({
        order: S.ORDER_AZ_EL,
        innerStart: -4000, innerEnd: 4000, innerStep: 400,
        outerStart: 0, outerEnd: 1800, outerStep: 300,
        reverse: true, continuous: false,
        fires: 1, cycles: 1, settleMs: 150, postFireMs: 50
    }, over || {});
}
const keysOf = (cfg, sent) => S.marks(cfg, sent).map(m => m.key);
const markFor = (cfg, sent, key) => S.marks(cfg, sent).find(m => m.key === key);

// ── normalisation: a def off disk is not to be trusted ───────────────────────
console.log("normalisation");
{
    eq("null normalises to the default", S.normalizeConfig(null), S.defaultConfig());
    eq("a string normalises to the default", S.normalizeConfig("nonsense"), S.defaultConfig());

    const d = S.defaultConfig();
    ok("the default is valid", S.validate(d) === "");
    ok("the default fires once", d.fires === 1);
    ok("the default runs one cycle", d.cycles === 1);

    // A zero step is the firmware's first rejection and an infinite loop in any point count, so
    // it never survives normalisation -- not even from a blob that predates this rule.
    ok("a zero inner step falls back", S.normalizeConfig({ innerStep: 0 }).innerStep !== 0);
    ok("a zero outer step falls back", S.normalizeConfig({ outerStep: 0 }).outerStep !== 0);
    ok("a NaN step falls back", S.normalizeConfig({ innerStep: NaN }).innerStep !== 0);
    ok("a missing step falls back", S.normalizeConfig({}).innerStep !== 0);

    ok("cycles cannot be zero", S.normalizeConfig({ cycles: 0 }).cycles === 1);
    ok("cycles cannot be negative", S.normalizeConfig({ cycles: -4 }).cycles === 1);
    ok("fires may be zero", S.normalizeConfig({ fires: 0 }).fires === 0);
    ok("fires cannot be negative", S.normalizeConfig({ fires: -1 }).fires === 0);
    ok("timing cannot be negative", S.normalizeConfig({ settleMs: -5 }).settleMs === 0);
    ok("timing is capped", S.normalizeConfig({ settleMs: 1e9 }).settleMs === S.LIMIT_MS);

    ok("a fractional step is rounded, not truncated to zero",
       S.normalizeConfig({ innerStep: 0.6 }).innerStep === 1);
    ok("an unknown order falls back to az->el",
       S.normalizeConfig({ order: "diagonal" }).order === S.ORDER_AZ_EL);
    ok("el->az survives", S.normalizeConfig({ order: S.ORDER_EL_AZ }).order === S.ORDER_EL_AZ);

    // Normalisation must be a fixed point, or a def rewrites itself on every load and a panel
    // reads dirty against a scan nobody edited.
    const once = S.normalizeConfig(ref());
    eq("normalisation is a fixed point", S.normalizeConfig(once), once);
    ok("copyConfig equals normalizeConfig", S.sameConfig(S.copyConfig(ref()), once));
    ok("a copy is not the same object", S.copyConfig(once) !== once);
}

// ── the firmware's refusals, met before a frame leaves ───────────────────────
console.log("what the firmware would refuse");
{
    ok("a valid scan validates clean", S.validate(ref()) === "");
    ok("a zero step is refused as given", S.validate(ref({ innerStep: 0 })) === "innerStep");
    ok("a zero outer step is refused as given", S.validate(ref({ outerStep: 0 })) === "outerStep");
    ok("zero cycles are refused as given", S.validate(ref({ cycles: 0 })) === "cycles");
    ok("a non-object is refused", S.validate(null) === "config");

    // Sweeping cannot fire twice at one point: the motor never stops there. The firmware answers
    // this with a rejected payload, which reaches the operator as nothing at all.
    const sweep5 = S.normalizeConfig(ref({ continuous: true, fires: 5 }));
    ok("sweeping clamps fires to one", sweep5.fires === 1);
    ok("...and therefore validates clean", S.validate(sweep5) === "");
    ok("sweeping with no fire is legal", S.validate(ref({ continuous: true, fires: 0 })) === "");

    // The clamp is normalisation's, so a config that bypassed it must still be caught.
    const raw = Object.assign(S.normalizeConfig(ref()), { continuous: true, fires: 4 });
    ok("an unnormalised sweep with several fires is refused", S.validate(raw) === "firesSweep");

    ok("step-and-shoot may fire many times", S.validate(ref({ fires: 12 })) === "");
    ok("validate returns a code, never a sentence",
       !/ /.test(S.validate(raw)) && S.validate(raw).length > 0);
}

// ── the point count ──────────────────────────────────────────────────────────
console.log("the point count");
{
    const n = S.counts(ref());
    eq("the reference scan", [n.inner, n.outer, n.total], [21, 7, 147]);

    ok("a single-point axis counts one",
       S.counts(ref({ outerStart: 0, outerEnd: 0 })).outer === 1);
    ok("direction does not change the count",
       S.counts(ref({ innerStart: 4000, innerEnd: -4000 })).inner === n.inner);
    ok("a negative step does not change the count",
       S.counts(ref({ innerStep: -400 })).inner === n.inner);

    // A step that does not divide its range is the mistake the drawing used to show and the
    // badges cannot: the count is what is left of that check.
    ok("an indivisible range counts the points actually visited",
       S.counts(ref({ innerStart: 0, innerEnd: 1000, innerStep: 300 })).inner === 4);
    ok("cycles are not folded into the point count",
       S.counts(ref({ cycles: 9 })).total === n.total);
}

// ── which marks appear, and when ─────────────────────────────────────────────
console.log("the marks");
{
    eq("a plain scan", keysOf(ref({ reverse: false })),
       ["order", "grid", "reset", "fires", "timing"]);
    eq("round-trip replaces reset", keysOf(ref()),
       ["order", "grid", "roundTrip", "fires", "timing"]);
    eq("sweeping adds a mark", keysOf(ref({ continuous: true })),
       ["order", "grid", "roundTrip", "sweep", "fires", "timing"]);
    eq("several cycles add a mark", keysOf(ref({ cycles: 3 })),
       ["order", "grid", "roundTrip", "fires", "cycles", "timing"]);

    ok("one cycle says nothing", keysOf(ref({ cycles: 1 })).indexOf("cycles") < 0);
    ok("step-and-shoot says nothing about sweeping",
       keysOf(ref()).indexOf("sweep") < 0);

    // Motion-only is deliberate and looks exactly like a fault, so it is the one mark that
    // carries a warning tone rather than being left to read as a zero.
    const zero = markFor(ref({ fires: 0 }), null, "noFire");
    ok("no fire is its own mark", !!zero);
    ok("...and it warns", zero.tone === "warn");
    ok("a firing scan does not warn", markFor(ref(), null, "fires").tone === "plain");

    // Post-fire wait does not apply mid-sweep, so the mark stops quoting a number the stand
    // will ignore.
    ok("post-fire is quoted when it applies", markFor(ref(), null, "timing").postFire === 50);
    ok("post-fire is dropped while sweeping",
       markFor(ref({ continuous: true }), null, "timing").postFire < 0);

    eq("the order mark names both axes",
       [markFor(ref(), null, "order").inner, markFor(ref(), null, "order").outer], ["az", "el"]);
    eq("scan order swaps them",
       [S.innerAxis(ref({ order: S.ORDER_EL_AZ })), S.outerAxis(ref({ order: S.ORDER_EL_AZ }))],
       ["el", "az"]);
}

// ── the strike-through, which is the only dirty signal there is ──────────────
console.log("marks that differ from what was sent");
{
    const sent = S.normalizeConfig(ref());

    ok("nothing sent yet strikes nothing",
       S.marks(ref(), null).every(m => !m.stale));
    ok("an unedited scan strikes nothing",
       S.marks(sent, sent).every(m => !m.stale));

    const flip = (over, key) => markFor(S.normalizeConfig(ref(over)), sent, key);

    ok("a changed traversal strikes its own mark", flip({ reverse: false }, "reset").stale);
    ok("a changed order strikes the order mark", flip({ order: S.ORDER_EL_AZ }, "order").stale);
    ok("a changed step strikes the grid mark", flip({ innerStep: 800 }, "grid").stale);
    ok("a changed endpoint strikes the grid mark", flip({ innerEnd: 2000 }, "grid").stale);
    ok("changed fires strike the firing mark", flip({ fires: 3 }, "fires").stale);
    ok("changed timing strikes the timing mark", flip({ settleMs: 400 }, "timing").stale);

    // The whole point of per-mark striking: an operator sees WHICH part of the scan the stand
    // is not running, not merely that something differs.
    const one = S.marks(S.normalizeConfig(ref({ settleMs: 400 })), sent);
    eq("one edit strikes exactly one mark",
       one.filter(m => m.stale).map(m => m.key), ["timing"]);

    // The order mark and the grid mark both read the order, because changing which axis is the
    // inner loop changes the shape of the grid as well as its name.
    const orderEdit = S.marks(S.normalizeConfig(ref({ order: S.ORDER_EL_AZ })), sent);
    eq("changing the order strikes both marks that depend on it",
       orderEdit.filter(m => m.stale).map(m => m.key), ["order", "grid"]);

    // Post-fire is not part of a swept scan, so editing it cannot make a swept scan read stale.
    const sweptSent = S.normalizeConfig(ref({ continuous: true }));
    const sweptEdit = S.normalizeConfig(ref({ continuous: true, postFireMs: 900 }));
    ok("post-fire does not strike a swept timing mark",
       !markFor(sweptEdit, sweptSent, "timing").stale);
    ok("...while settle still does",
       markFor(S.normalizeConfig(ref({ continuous: true, settleMs: 900 })), sweptSent, "timing").stale);

    ok("sameConfig sees an edit", !S.sameConfig(S.normalizeConfig(ref({ fires: 2 })), sent));
    ok("sameConfig ignores nothing in CONFIG_KEYS",
       S.CONFIG_KEYS.every(k => {
           const other = S.copyConfig(sent);
           other[k] = (typeof other[k] === "boolean") ? !other[k]
                    : (typeof other[k] === "number")  ? other[k] + 7 : "elAz";
           return !S.sameConfig(other, sent);
       }));
}

// ── one vocabulary, asserted over the QML source ─────────────────────────────
console.log("the words live in one file");
{
    // Every key the logic can emit, gathered by driving it rather than by listing it here --
    // a list would be a third copy of the vocabulary.
    const emitted = new Set();
    for (const rev of [true, false])
        for (const cont of [true, false])
            for (const fires of [0, 1])
                for (const cycles of [1, 3])
                    keysOf(ref({ reverse: rev, continuous: cont, fires, cycles }))
                        .forEach(k => emitted.add(k));

    eq("the logic emits nine marks in total", [...emitted].sort(),
       ["cycles", "fires", "grid", "noFire", "order", "reset", "roundTrip", "sweep", "timing"]);

    for (const key of emitted)
        ok(`StandBadgeRow names "${key}"`, BADGE_ROW.includes(`case "${key}"`));

    // ...and nothing else: a case for a key the logic cannot return is dead, and reads as a
    // feature that exists.
    const cased = [...BADGE_ROW.matchAll(/case "([a-zA-Z]+)":/g)].map(m => m[1]);
    eq("StandBadgeRow names nothing the logic cannot emit",
       cased.filter(k => !emitted.has(k)), []);

    ok("the logic calls no qsTr", !/qsTr\s*\(/.test(src("qml", "app", "StandLogic.js")));
    ok("the badge row contains no scan arithmetic",
       !BADGE_ROW.includes("Math.floor") && !BADGE_ROW.includes("innerStep"));
}

// ── source rules: things a screenshot cannot show ────────────────────────────
console.log("what the panel must not do");
{
    // EDIT -> COMMIT. Every other device control in this app writes on change; this one cannot,
    // because Start is the only write the stand takes. If _writeCfg ever sends, the form stops
    // being a draft and the strike-through becomes a lie.
    const writeCfg = PANEL.slice(PANEL.indexOf("function _writeCfg"),
                                 PANEL.indexOf("function _toggleExpanded"));
    ok("editing a field sends nothing",
       !writeCfg.includes("_call(") && !writeCfg.includes("standStart"));
    ok("editing a field persists the scan", writeCfg.includes("setWidgetStandConfig"));

    // The stand publishes no state, so Start cannot be gated on a run: the guard would be a
    // guess in the shape of a safeguard.
    const start = PANEL.slice(PANEL.indexOf("function start()"), PANEL.indexOf("function stop()"));
    ok("Start is gated only on validity and capability",
       start.includes("_canStart") && !start.includes("_sentCmd"));
    ok("Start records what it sent", start.includes("_sent = "));

    // Pause and Resume are two controls. One that changed face would be asserting a device state
    // that nothing reports.
    ok("Resume is reachable", PANEL.includes("root.resume()"));
    ok("Pause is reachable", PANEL.includes("root.pause()"));
    ok("neither is derived from the last command sent",
       !/glyph:\s*root\._sentCmd/.test(PANEL));

    // No progress readout, ever: it could only be composed from what this app sent.
    for (const word of ["progress", "Progress", "currentPoint", "pointIndex"])
        ok(`the panel has no ${word}`, !PANEL.includes(word));

    // NO PROSE. Every long string in the panel must be a tooltip, which costs no height. A line
    // that appears and disappears with state changes a floating panel's height while it is being
    // read, which is worse than saying less.
    const longStrings = [...PANEL.matchAll(/qsTr\("([^"]{45,})"/g)].map(m => m[0]);
    const looseProse = longStrings.filter(s => {
        const line = PANEL.slice(0, PANEL.indexOf(s)).split("\n").pop();
        return !line.includes("toolTipText");
    });
    eq("every long string is a tooltip", looseProse, []);
    ok("the panel declares no hint element", !/class="hint"|id:\s*hint/.test(PANEL));
}

// ── source rules: the kind is invisible without a stand ──────────────────────
console.log("the kind hides itself");
{
    ok("the store asks the device layer, not a snapshot",
       STORE.includes("deviceManagerWrapper.standAvailable"));
    ok("the store counts stand-capable devices", STORE.includes("standDeviceCount"));
    ok("the store knows whether a stand panel exists", STORE.includes("hasStandPanel"));

    const card = KIND_STEP.slice(KIND_STEP.indexOf('kind: "stand"') - 700,
                                 KIND_STEP.indexOf('kind: "stand"'));
    ok("the kind card requires a stand", card.includes("standAvailable"));
    ok("the kind card refuses a second panel", card.includes("!step.store.hasStandPanel"));

    ok("the panel binds to the stand's device, not the active one",
       MAIN.includes("dev: workspaceStore.standDevice"));
    ok("the on-scene panel hides with the stand",
       /popupVisible:[^\n]*standAvailable/.test(MAIN));

    // A hidden panel is still a panel: its scan has to survive the stand being unplugged, or
    // every reconnection costs the operator a re-entry.
    ok("hiding a panel does not delete it", STORE.includes("function widgetListed"));
    ok("a stand def carries its scan", STORE.includes("config: Stand.normalizeConfig(raw.config)"));
    ok("a stand def carries its regime", STORE.includes("expanded: raw.expanded === true"));
}

console.log(`\n${pass} passed, ${fails.length} failed`);
if (fails.length) {
    console.log("failed: " + fails.join(", "));
    process.exit(1);
}

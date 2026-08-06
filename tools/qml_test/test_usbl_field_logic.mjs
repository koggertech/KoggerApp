// Behaviour tests for the USBL widget field logic. No Qt, no window, no device.
//
//   node tools/qml_test/test_usbl_field_logic.mjs
//
// Exit 0 = all pass. The thing being defended: a widget labelled "beacon 2" must never
// show beacon 1's number. That failure looks completely correct on screen.

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { readFileSync } from "node:fs";
import path from "node:path";

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));
const U = require(path.join(here, "..", "..", "qml", "kqml_types", "UsblFieldLogic.js"));

let pass = 0;
const fails = [];
function ok(name, cond, detail) {
    if (cond) { pass++; console.log("  ok   " + name); }
    else { fails.push(name); console.log(`  FAIL ${name}${detail ? "   -- " + detail : ""}`); }
}
const j = (v) => JSON.stringify(v);
function eq(name, got, want) { ok(name, j(got) === j(want), `got ${j(got)}, want ${j(want)}`); }

const NOW = 1_000_000;
function sol(addr, over) {
    return Object.assign({
        address: addr, distance: 10 + addr, azimuth: 90 + addr, elevation: -5,
        snr: 20 + addr, beaconLat: 59.9386, beaconLon: 30.3141, beaconDepth: 12.5,
        epochMs: NOW - 1000, coordValid: true
    }, over || {});
}
// Beacon 2 answered most recently; 1 is older. This is the shape a two-node schedule
// produces, and the shape the old single-solution model could not represent.
const SOLS = { "1": sol(1, { epochMs: NOW - 4000 }), "2": sol(2, { epochMs: NOW - 500 }) };

// ── addressing: the whole point ──────────────────────────────────────────────
console.log("addressing");
{
    eq("an address selects that beacon, not the newest", U.pick(SOLS, 2).address, 2);
    eq("...and the other one is still reachable", U.pick(SOLS, 1).address, 1);
    eq("their ranges differ, so a mix-up would be visible",
       [U.rawValue("usblRange", U.pick(SOLS, 1), NOW),
        U.rawValue("usblRange", U.pick(SOLS, 2), NOW)], ["11.0", "12.0"]);

    eq("ANY resolves to the most recent fix", U.pick(SOLS, U.ANY).address, 2);
    eq("...which is what the old single-solution model gave",
       U.rawValue("usblRange", U.pick(SOLS, U.ANY), NOW), "12.0");

    ok("a beacon that never answered yields nothing", U.pick(SOLS, 7) === null);
    ok("...and every field on it is invalid",
       !U.isValid("usblRange", U.pick(SOLS, 7), NOW));
    ok("...rendering as no value, never as zero",
       U.rawValue("usblRange", U.pick(SOLS, 7), NOW) === null);

    ok("an empty map yields nothing for ANY too", U.pick({}, U.ANY) === null);
    ok("...and a missing map does not throw", U.pick(null, 2) === null);
}

// ── address normalisation ────────────────────────────────────────────────────
console.log("address normalisation");
{
    eq("in-range addresses pass through", [0, 1, 8].map(U.normAddr), [0, 1, 8]);
    eq("ANY passes through", U.normAddr(-1), U.ANY);
    // Out of range must NOT fall back to 0: address 0 is the promiscuous address and a
    // real selectable beacon, so coercing to it would silently show someone else's fix.
    eq("out of range becomes ANY, never 0", [9, 255, -2].map(U.normAddr),
       [U.ANY, U.ANY, U.ANY]);
    eq("garbage becomes ANY", ["", null, undefined, "x"].map(U.normAddr),
       [U.ANY, U.ANY, U.ANY, U.ANY]);
    eq("a numeric string is accepted (JSON round trips)", U.normAddr("3"), 3);
}

// ── which fields may be pinned ───────────────────────────────────────────────
console.log("addressable fields");
{
    ok("range is addressable", U.isAddressable("usblRange"));
    ok("beacon position is addressable", U.isAddressable("usblBeaconCoord"));
    // Pinning "who answered" to an address makes it a constant.
    ok("the responding address is NOT addressable", !U.isAddressable("usblAddress"));
    ok("presence is NOT addressable", !U.isAddressable("usblPresent"));
    ok("a non-USBL field is not one of ours", !U.isUsblField("depth"));
    ok("...and is not addressable either", !U.isAddressable("depth"));
}

// ── liveness ─────────────────────────────────────────────────────────────────
console.log("liveness");
{
    eq("a recent fix is tracking", U.stateCode(sol(2, { epochMs: NOW - 500 }), NOW), "tracking");
    eq("an old one is stale", U.stateCode(sol(2, { epochMs: NOW - 60000 }), NOW), "stale");
    eq("a missing one is none", U.stateCode(null, NOW), "none");
    eq("the boundary is not stale", U.stateCode(sol(2, { epochMs: NOW - U.STALE_MS }), NOW),
       "tracking");
    eq("one millisecond past it is", U.stateCode(sol(2, { epochMs: NOW - U.STALE_MS - 1 }), NOW),
       "stale");

    eq("age is reported in seconds", U.rawValue("usblAge", U.pick(SOLS, 1), NOW), 4);
    eq("...per beacon, not globally", U.rawValue("usblAge", U.pick(SOLS, 2), NOW), 0.5);
    ok("age never goes negative on a clock step",
       U.ageMs(sol(2, { epochMs: NOW + 5000 }), NOW) === 0);
}

// ── values carry no units and no prose ───────────────────────────────────────
// The catalog adds the unit and the translated word. If a unit leaked in here it could
// not be translated, and the same string would be wrong in feet.
console.log("raw values");
{
    const e = U.pick(SOLS, 2);
    eq("range is a bare number", U.rawValue("usblRange", e, NOW), "12.0");
    eq("azimuth is a bare number", U.rawValue("usblAzimuth", e, NOW), "92.0");
    eq("snr is rounded to whole dB", U.rawValue("usblSnr", e, NOW), "22");
    eq("beacon depth keeps centimetres", U.rawValue("usblBeaconDepth", e, NOW), "12.50");
    eq("state is a code, not a sentence", U.rawValue("usblState", e, NOW), "tracking");
    ok("no value contains a unit",
       ["usblRange", "usblAzimuth", "usblSnr", "usblBeaconDepth"]
           .every((k) => !/[a-zA-Z°]/.test(String(U.rawValue(k, e, NOW)))));

    eq("the beacon position is two lines",
       U.rawValue("usblBeaconCoord", e, NOW), "N 59.9386°\nE 30.3141°");
    ok("...and is withheld when the coordinate is not valid",
       U.rawValue("usblBeaconCoord", sol(2, { coordValid: false }), NOW) === null);

    // NaN reaches here whenever the device could not solve a component.
    const nan = sol(2, { distance: NaN, azimuth: NaN });
    ok("NaN is no value, not the string NaN",
       U.rawValue("usblRange", nan, NOW) === null && !U.isValid("usblRange", nan, NOW));
    ok("...while the other fields on the same fix still work",
       U.rawValue("usblSnr", nan, NOW) === "22");
}

// ── the wizard's address picker ──────────────────────────────────────────────
console.log("known addresses");
{
    eq("only beacons that answered are offered", U.knownAddresses(SOLS), [1, 2]);
    eq("...ascending, whatever order they replied in",
       U.knownAddresses({ "5": sol(5), "0": sol(0), "3": sol(3) }), [0, 3, 5]);
    eq("an empty map offers nothing", U.knownAddresses({}), []);
    eq("a missing map does not throw", U.knownAddresses(null), []);
    eq("out-of-range keys are ignored", U.knownAddresses({ "9": sol(9), "2": sol(2) }), [2]);
}

// ── the palette must not grow behind everyone's back ─────────────────────────
// DataFieldCatalog.fields is bound straight to a Repeater in WidgetPlaceStep, so an entry
// added there is a tile in front of EVERY user on EVERY device -- shared UI, not a data
// declaration. That is how ten USBL tiles once shipped to people with no beacon.
//
// This reads the QML as text on purpose: the property is the contract, and node cannot
// instantiate a QML singleton to check it any other way.
console.log("palette containment");
{
    const src = readFileSync(
        path.join(here, "..", "..", "qml", "kqml_types", "DataFieldCatalog.qml"), "utf8");

    const slice = (name) => {
        const at = src.indexOf(`readonly property var ${name}: [`);
        ok(`${name} exists`, at >= 0);
        return at < 0 ? "" : src.slice(at, src.indexOf("\n    ]", at));
    };

    const paletteKeys = [...slice("fields").matchAll(/key:\s*"([^"]+)"/g)].map((m) => m[1]);
    const usblKeys = [...slice("usblFields").matchAll(/key:\s*"([^"]+)"/g)].map((m) => m[1]);

    eq("every USBL field is declared outside the palette list",
       usblKeys.slice().sort(), U.FIELDS.map((f) => f.key).sort());
    eq("...and none of them leaked into it",
       paletteKeys.filter((k) => k.startsWith("usbl")), []);
    ok("the palette still carries the general fields", paletteKeys.length >= 14);
    ok("no key is declared twice",
       new Set([...paletteKeys, ...usblKeys]).size === paletteKeys.length + usblKeys.length);
}

// ── a colour per address ─────────────────────────────────────────────────────
// The colour IS the beacon's identity: it ties a row in the settings pane to a row on the
// scene panel to a marker on the map. Two beacons sharing one, or one beacon changing colour
// between releases, breaks the only thing it is for — and neither is visible in a screenshot
// of a single pane.
console.log("address colours");
{
    eq("every protocol address has a colour", U.ADDRESS_COLORS.length, U.MAX_ADDR + 1);

    let allHex = true;
    for (const c of U.ADDRESS_COLORS) if (!/^#[0-9A-Fa-f]{6}$/.test(c)) allHex = false;
    ok("...all of them literal 6-digit hex, so no theme can move them", allHex);

    ok("no two addresses share a colour",
       new Set(U.ADDRESS_COLORS).size === U.ADDRESS_COLORS.length);

    for (let a = 0; a <= U.MAX_ADDR; ++a)
        eq(`address ${a} takes its own entry`, U.addressColor(a), U.ADDRESS_COLORS[a]);

    // 0 is the promiscuous address, not a beacon, so it is deliberately the neutral one and
    // the out-of-range fallback lands on it too.
    eq("out of range falls back to the neutral", U.addressColor(99), U.ADDRESS_COLORS[0]);
    eq("...as does a negative", U.addressColor(-1), U.ADDRESS_COLORS[0]);
    eq("...and a non-number", U.addressColor("beacon"), U.ADDRESS_COLORS[0]);
    eq("a numeric string still resolves", U.addressColor("3"), U.ADDRESS_COLORS[3]);

    // THE TABLE IS A COMPATIBILITY SURFACE. Inserting rather than appending re-colours every
    // beacon above the insertion for every operator who had learned them, and nothing in the
    // app would report it. Pinned verbatim so that edit fails here instead.
    eq("the table is pinned — append only, never insert or reorder", U.ADDRESS_COLORS, [
        "#64748B", "#3E8FD6", "#16A34A", "#E0902B", "#8B5CF6",
        "#0E9BB5", "#D6539B", "#C2703A", "#6D8B21"
    ]);

    // The badge is a fixed fill under a changing theme, so its ink cannot come from the theme.
    const badge = readFileSync(path.join(here, "..", "..", "qml", "app", "UsblAddressBadge.qml"),
                               "utf8");
    ok("the badge takes its fill from this table",
       /usblAddressColor\(/.test(badge));
    ok("...and picks readable ink for whatever colour that is",
       /AppPalette\.luminance\(/.test(badge));
    ok("...and is a rounded square, not a disc",
       /radius:\s*Tokens\.radiusSm/.test(badge) && !/radius:\s*height\s*\/\s*2/.test(badge));
}

console.log("");
console.log(`${pass} passed, ${fails.length} failed`);
for (const f of fails) console.log("  FAILED: " + f);
process.exit(fails.length ? 1 : 0);

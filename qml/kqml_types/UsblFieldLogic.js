// Pure selection and formatting for the USBL widget fields. No QML types, no qsTr, no
// property reads -- a solution entry goes in, a string comes out.
//
// WHY THIS IS A SEPARATE FILE
//
// Everything a USBL widget shows is picked out of Dataset.usblSolutions, a plain
// address -> object map. Picking the right entry and rendering it are the two things
// that can silently be wrong -- a widget showing beacon 1's range under a "beacon 2"
// label looks perfectly fine on screen. Keeping it here means `node` asserts it with no
// Qt, no window and no device, the same way UsblPlanLogic.js is tested.
//
// Anything user-visible that is not a number stays in DataFieldCatalog.qml, where qsTr
// lives: this module returns numbers and CODES.

var ANY = -1;                 // "whichever beacon answered last"
var MAX_ADDR = 8;             // the protocol's range is 0..8; 0xFF means "no address"
var STALE_MS = 5000;          // a fix older than this is not to be trusted as live

// Every field this module can render, and whether naming a beacon means anything for it.
// `usblAddress` is deliberately NOT addressable: it answers "who replied", so pinning it
// to an address would make it a constant.
var FIELDS = [
    { key: "usblRange",        addressable: true,  digits: 1 },
    { key: "usblAzimuth",      addressable: true,  digits: 1 },
    { key: "usblElevation",    addressable: true,  digits: 1 },
    { key: "usblSnr",          addressable: true,  digits: 0 },
    { key: "usblBeaconDepth",  addressable: true,  digits: 2 },
    { key: "usblBeaconCoord",  addressable: true,  digits: 4 },
    { key: "usblAge",          addressable: true,  digits: 1 },
    { key: "usblState",        addressable: true,  digits: 0 },
    { key: "usblAddress",      addressable: false, digits: 0 },
    { key: "usblPresent",      addressable: false, digits: 0 }
];

// ── a colour per address ─────────────────────────────────────────────────────
//
// IDENTITY, NOT STATE. A node's colour never changes: it is how you match a row in the pane to
// a row in the on-scene panel to a marker on the map. State is carried by the chips, which are
// pale tinted fills with a word in them -- a different visual register from a saturated solid
// with a digit -- so a green address badge does not read as "replied".
//
// THIS TABLE IS A COMPATIBILITY SURFACE. Inserting a colour rather than appending re-colours
// every beacon above it for every operator who had learned them. Append only; the order is
// asserted verbatim in test_usbl_field_logic.mjs so a well-meant reshuffle fails there rather
// than on a boat.
//
// Address 0 is the PROMISCUOUS address, so it gets a neutral slate rather than an identity hue
// -- it is not one beacon, and colouring it like one would say it was.
//
// Deliberately NOT the plan's `groupColors`: a command group and a node are different things
// that appear in the same pane, and one palette for both would invite reading a node's colour
// as its group's.
var ADDRESS_COLORS = [
    "#64748B",  // 0 promiscuous — neutral
    "#3E8FD6",  // 1 blue
    "#16A34A",  // 2 green
    "#E0902B",  // 3 amber
    "#8B5CF6",  // 4 violet
    "#0E9BB5",  // 5 cyan
    "#D6539B",  // 6 magenta
    "#C2703A",  // 7 copper
    "#6D8B21"   // 8 olive
];

// Anything outside 0..MAX_ADDR takes the neutral. A hand-edited blob or a future wider address
// range then renders a legible badge instead of an empty one.
function addressColor(addr) {
    var n = parseInt(addr, 10);
    if (isNaN(n) || n < 0 || n >= ADDRESS_COLORS.length) return ADDRESS_COLORS[0];
    return ADDRESS_COLORS[n];
}

function meta(key) {
    for (var i = 0; i < FIELDS.length; ++i)
        if (FIELDS[i].key === key) return FIELDS[i];
    return null;
}
function isUsblField(key) { return meta(key) !== null; }
function isAddressable(key) {
    var m = meta(key);
    return !!m && m.addressable;
}

// -1 (any) or 0..8. Anything else -- a hand-edited blob, a stale binding -- becomes ANY
// rather than silently selecting beacon 0.
function normAddr(a) {
    var n = parseInt(a, 10);
    if (isNaN(n)) return ANY;
    if (n === ANY) return ANY;
    return (n >= 0 && n <= MAX_ADDR) ? n : ANY;
}

// The addresses that have ever answered, ascending. Drives the wizard's picker: offering
// a beacon that has never replied is offering a permanently blank widget.
function knownAddresses(solutions) {
    var out = [];
    for (var k in (solutions || {})) {
        var n = parseInt(k, 10);
        if (!isNaN(n) && n >= 0 && n <= MAX_ADDR) out.push(n);
    }
    return out.sort(function (a, b) { return a - b; });
}

// The entry a cell should read. ANY resolves to the most recent fix across all beacons,
// which is what the single-solution model used to give -- so a plan with one beacon
// behaves exactly as before.
function pick(solutions, addr) {
    var s = solutions || {}, a = normAddr(addr);
    if (a !== ANY) {
        var e = s[String(a)];
        return e ? e : null;
    }
    var best = null;
    for (var k in s) {
        var v = s[k];
        if (!v) continue;
        if (!best || (v.epochMs || 0) > (best.epochMs || 0)) best = v;
    }
    return best;
}

function ageMs(entry, nowMs) {
    if (!entry || !entry.epochMs) return -1;
    return Math.max(0, nowMs - entry.epochMs);
}

// CODES, not prose: the caller translates.
//   none    -- this beacon has never answered
//   stale   -- it answered, but not recently enough to act on
//   tracking-- live
function stateCode(entry, nowMs) {
    var ms = ageMs(entry, nowMs);
    if (ms < 0) return "none";
    return ms > STALE_MS ? "stale" : "tracking";
}

function _num(v, digits) {
    if (v === undefined || v === null || typeof v !== "number" || !isFinite(v)) return null;
    return v.toFixed(digits);
}

function _dms(deg, isLat) {
    if (deg === undefined || deg === null || typeof deg !== "number" || !isFinite(deg))
        return null;
    var hemi = isLat ? (deg >= 0 ? "N" : "S") : (deg >= 0 ? "E" : "W");
    return hemi + " " + Math.abs(deg).toFixed(4) + "°";
}

// A field is valid when the number behind it exists. A beacon that has never answered is
// invalid for every field, which is what puts an em dash on screen rather than a zero.
function isValid(key, entry, nowMs) {
    if (key === "usblPresent") return true;
    if (!entry) return false;
    switch (key) {
    case "usblRange":       return _num(entry.distance, 1) !== null;
    case "usblAzimuth":     return _num(entry.azimuth, 1) !== null;
    case "usblElevation":   return _num(entry.elevation, 1) !== null;
    case "usblSnr":         return _num(entry.snr, 0) !== null;
    case "usblBeaconDepth": return _num(entry.beaconDepth, 2) !== null;
    case "usblBeaconCoord": return !!entry.coordValid;
    case "usblAge":         return ageMs(entry, nowMs) >= 0;
    case "usblState":       return true;
    case "usblAddress":     return typeof entry.address === "number";
    }
    return false;
}

// Returns the bare number/string with NO unit and NO translated word -- the catalog adds
// those. `null` means "no value", which the catalog renders as an em dash.
//
// usblState and usblAge return codes/numbers for the same reason: "3.4 s ago" and
// "TRACKING" are sentences, and sentences belong where qsTr is.
function rawValue(key, entry, nowMs) {
    if (key === "usblPresent")
        return entry ? "yes" : "no";
    if (!entry) return null;
    switch (key) {
    case "usblRange":       return _num(entry.distance, 1);
    case "usblAzimuth":     return _num(entry.azimuth, 1);
    case "usblElevation":   return _num(entry.elevation, 1);
    case "usblSnr":         return _num(entry.snr, 0);
    case "usblBeaconDepth": return _num(entry.beaconDepth, 2);
    case "usblBeaconCoord":
        if (!entry.coordValid) return null;
        var la = _dms(entry.beaconLat, true), lo = _dms(entry.beaconLon, false);
        return (la && lo) ? (la + "\n" + lo) : null;
    case "usblAge":         { var ms = ageMs(entry, nowMs); return ms < 0 ? null : ms / 1000; }
    case "usblState":       return stateCode(entry, nowMs);
    case "usblAddress":     return typeof entry.address === "number" ? String(entry.address) : null;
    }
    return null;
}

// Node consumes this via module.exports; QML sees the top-level functions and ignores it.
if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        ANY: ANY, MAX_ADDR: MAX_ADDR, STALE_MS: STALE_MS, FIELDS: FIELDS,
        ADDRESS_COLORS: ADDRESS_COLORS, addressColor: addressColor,
        meta: meta, isUsblField: isUsblField, isAddressable: isAddressable,
        normAddr: normAddr, knownAddresses: knownAddresses, pick: pick,
        ageMs: ageMs, stateCode: stateCode, isValid: isValid, rawValue: rawValue
    };
}

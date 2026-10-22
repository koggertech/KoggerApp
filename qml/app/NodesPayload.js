.pragma library

var MAX_BITS = 183;
var POSITION_BITS = 66;

var FIELD_TYPES = ["uint", "int", "bool"];

/** Hex string without separators, upper case, even length. */
function normalizeHex(hex) {
    var s = String(hex || "").replace(/[^0-9a-fA-F]/g, "").toUpperCase();
    if (s.length % 2) s = s.substring(0, s.length - 1);
    return s;
}

/** Hex string grouped by byte for display. */
function prettyHex(hex) {
    var s = normalizeHex(hex), out = [];
    for (var i = 0; i < s.length; i += 2) out.push(s.substring(i, i + 2));
    return out.join(" ");
}

function _bytesFromHex(hex) {
    var s = normalizeHex(hex), out = [];
    for (var i = 0; i < s.length; i += 2) out.push(parseInt(s.substring(i, i + 2), 16));
    return out;
}

function _hexFromBytes(bytes) {
    var out = "";
    for (var i = 0; i < bytes.length; ++i) {
        var h = (bytes[i] & 0xFF).toString(16).toUpperCase();
        out += (h.length < 2 ? "0" : "") + h;
    }
    return out;
}

function _bitArray(bitsList) {
    var n = bitsList.length, bytes = [];
    for (var b = 0; b < Math.ceil(n / 8); ++b) bytes.push(0);
    for (var k = 0; k < n; ++k)
        if (bitsList[k]) bytes[Math.floor(k / 8)] |= (1 << (k % 8));
    return bytes;
}

function _bitsOfBytes(bytes, count) {
    var out = [];
    for (var k = 0; k < count; ++k) {
        var byte = bytes[Math.floor(k / 8)] || 0;
        out.push((byte >> (k % 8)) & 1);
    }
    return out;
}

function formatById(formats, id) {
    for (var i = 0; i < (formats ? formats.length : 0); ++i)
        if (formats[i].id === id) return formats[i];
    return null;
}

function formatBits(format) {
    var n = 0;
    var fs = format && format.fields ? format.fields : [];
    for (var i = 0; i < fs.length; ++i) n += Math.max(0, fs[i].bits | 0);
    return n;
}

function _fieldRaw(field, value) {
    var bits = Math.max(1, field.bits | 0);
    if (field.type === "bool") return value ? 1 : 0;
    var scale = (typeof field.scale === "number" && field.scale !== 0) ? field.scale : 1;
    var offset = (typeof field.offset === "number") ? field.offset : 0;
    var v = Math.round(((Number(value) || 0) - offset) / scale);
    if (field.type === "int") {
        var lo = -Math.pow(2, bits - 1), hi = Math.pow(2, bits - 1) - 1;
        v = Math.max(lo, Math.min(hi, v));
        if (v < 0) v = Math.pow(2, bits) + v;
        return v;
    }
    return Math.max(0, Math.min(Math.pow(2, bits) - 1, v));
}

function _fieldValue(field, raw) {
    var bits = Math.max(1, field.bits | 0);
    if (field.type === "bool") return raw !== 0;
    var v = raw;
    if (field.type === "int" && raw >= Math.pow(2, bits - 1)) v = raw - Math.pow(2, bits);
    var scale = (typeof field.scale === "number" && field.scale !== 0) ? field.scale : 1;
    var offset = (typeof field.offset === "number") ? field.offset : 0;
    return v * scale + offset;
}

/** Number of payload bits a data spec carries on the wire. */
function bitsOf(data, formats) {
    if (!data || !data.type || data.type === "none") return 0;
    switch (data.type) {
    case "bytes": {
        var nb = _bytesFromHex(data.hex).length * 8;
        var b = (typeof data.bits === "number" && data.bits > 0) ? Math.min(data.bits, nb) : nb;
        return b;
    }
    case "bits": return String(data.binary || "").replace(/[^01]/g, "").length;
    case "text": return String(data.ascii || "").length * 8;
    case "format": return formatBits(formatById(formats, data.format));
    case "position": return POSITION_BITS;
    }
    return 0;
}

/** Wire function code for a data spec: 0 default, 1 bit array, 2 position + azimuth. */
function wireFunction(data) {
    if (!data || !data.type || data.type === "none") return 0;
    return data.type === "position" ? 2 : 1;
}

/** Encodes a data spec into { hex, bits }, bits packed LSB-first. Position encodes nothing. */
function encode(data, formats) {
    if (!data || !data.type || data.type === "none" || data.type === "position")
        return { hex: "", bits: data && data.type === "position" ? POSITION_BITS : 0 };
    var list = [];
    switch (data.type) {
    case "bytes": {
        var bytes = _bytesFromHex(data.hex);
        var bits = bitsOf(data, formats);
        list = _bitsOfBytes(bytes, bits);
        break;
    }
    case "bits": {
        var s = String(data.binary || "").replace(/[^01]/g, "");
        for (var i = 0; i < s.length; ++i) list.push(s.charAt(i) === "1" ? 1 : 0);
        break;
    }
    case "text": {
        var t = String(data.ascii || "");
        for (var c = 0; c < t.length; ++c) {
            var code = t.charCodeAt(c) & 0xFF;
            for (var k = 0; k < 8; ++k) list.push((code >> k) & 1);
        }
        break;
    }
    case "format": {
        var f = formatById(formats, data.format);
        var fs = f && f.fields ? f.fields : [];
        var values = data.values || {};
        for (var j = 0; j < fs.length; ++j) {
            var w = Math.max(0, fs[j].bits | 0);
            var raw = _fieldRaw(fs[j], values[fs[j].name]);
            for (var q = 0; q < w; ++q) list.push(Math.floor(raw / Math.pow(2, q)) % 2);
        }
        break;
    }
    }
    return { hex: _hexFromBytes(_bitArray(list)), bits: list.length };
}

/** Decodes received bytes with the data spec that describes them. */
function decode(hex, bits, data, formats) {
    var bytes = _bytesFromHex(hex);
    var n = (typeof bits === "number" && bits >= 0) ? bits : bytes.length * 8;
    var list = _bitsOfBytes(bytes, n);
    var type = data && data.type ? data.type : "bytes";
    if (type === "text") {
        var s = "";
        for (var i = 0; i + 8 <= list.length; i += 8) {
            var code = 0;
            for (var k = 0; k < 8; ++k) code |= list[i + k] << k;
            s += (code >= 32 && code < 127) ? String.fromCharCode(code) : "·";
        }
        return { kind: "text", text: s };
    }
    if (type === "bits")
        return { kind: "bits", text: list.join("") };
    if (type === "format") {
        var f = formatById(formats, data.format);
        var fs = f && f.fields ? f.fields : [];
        var pos = 0, values = {};
        for (var j = 0; j < fs.length; ++j) {
            var w = Math.max(0, fs[j].bits | 0), raw = 0;
            for (var q = 0; q < w; ++q) raw += (list[pos + q] || 0) * Math.pow(2, q);
            pos += w;
            values[fs[j].name] = _fieldValue(fs[j], raw);
        }
        return { kind: "format", values: values, text: describeValues(f, values) };
    }
    return { kind: "bytes", text: prettyHex(hex) };
}

/** Number of differing bits among the first `bits` of two payloads; bits past `bits` are ignored. */
function differingBits(hexA, hexB, bits) {
    var n = Math.max(0, bits | 0);
    var a = _bitsOfBytes(_bytesFromHex(hexA), n), b = _bitsOfBytes(_bytesFromHex(hexB), n), d = 0;
    for (var i = 0; i < n; ++i) if (a[i] !== b[i]) ++d;
    return d;
}

function describeValues(format, values) {
    var fs = format && format.fields ? format.fields : [], parts = [];
    for (var i = 0; i < fs.length; ++i) {
        var v = values ? values[fs[i].name] : undefined;
        var t = (typeof v === "boolean") ? (v ? "1" : "0")
              : (typeof v === "number") ? String(Math.round(v * 1000) / 1000) : "—";
        parts.push(fs[i].name + "=" + t + (fs[i].unit ? " " + fs[i].unit : ""));
    }
    return parts.join(" · ");
}

/** Short label of a data spec for arrows and lists. */
function summary(data, formats) {
    if (!data || !data.type || data.type === "none") return "";
    var bits = bitsOf(data, formats);
    switch (data.type) {
    case "bytes": return prettyHex(data.hex) + " · " + bits + " bit";
    case "bits": return bits + " bit";
    case "text": return "\"" + String(data.ascii || "") + "\" · " + bits + " bit";
    case "format": {
        var f = formatById(formats, data.format);
        return (f ? f.name : "?") + " · " + bits + " bit";
    }
    case "position": return "position · " + POSITION_BITS + " bit";
    }
    return "";
}

/** Bit map of a format for the editor preview: [{name, from, to}]. */
function bitMap(format) {
    var fs = format && format.fields ? format.fields : [], out = [], pos = 0;
    for (var i = 0; i < fs.length; ++i) {
        var w = Math.max(0, fs[i].bits | 0);
        out.push({ name: fs[i].name, from: pos, to: pos + w - 1 });
        pos += w;
    }
    return out;
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        MAX_BITS: MAX_BITS, POSITION_BITS: POSITION_BITS, FIELD_TYPES: FIELD_TYPES,
        normalizeHex: normalizeHex, prettyHex: prettyHex, formatById: formatById,
        formatBits: formatBits, bitsOf: bitsOf, wireFunction: wireFunction,
        encode: encode, decode: decode, differingBits: differingBits, describeValues: describeValues,
        summary: summary, bitMap: bitMap
    };
}

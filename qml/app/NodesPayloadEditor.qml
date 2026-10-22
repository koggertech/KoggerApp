import QtQuick 2.15
import kqml_types 1.0
import "NodesPayload.js" as Payload
import "NodesTiming.js" as Timing

Column {
    id: root

    property var store: null
    property var spec: ({ type: "none" })
    property string title: ""
    property bool allowPosition: false
    property bool positionOnly: false
    property int maxBits: Payload.MAX_BITS
    signal edited(var spec)

    spacing: Tokens.spaceXs

    readonly property var _formats: store ? store.formats : []
    readonly property var _types: {
        var t = [ { id: "none", label: qsTr("Nothing") },
                  { id: "bytes", label: qsTr("Bytes (hex)") },
                  { id: "bits", label: qsTr("Bits") },
                  { id: "text", label: qsTr("Text") } ]
        if (_formats.length) t.push({ id: "format", label: qsTr("Fields") })
        if (allowPosition) t.push({ id: "position", label: qsTr("Position + azimuth") })
        return positionOnly ? t.filter(function (x) { return x.id === "none" || x.id === "position" }) : t
    }
    readonly property string _type: spec && spec.type ? spec.type : "none"
    readonly property int _bits: Payload.bitsOf(spec, _formats)
    readonly property bool _over: _bits > maxBits

    function _emit(d) { root.edited(d) }

    Row {
        width: parent.width
        spacing: Tokens.spaceSm
        Text {
            text: root.title
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
            width: Math.round(84 * AppPalette.scale)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        KCombo {
            model: root._types.map(function (t) { return t.label })
            currentIndex: {
                for (var i = 0; i < root._types.length; ++i) if (root._types[i].id === root._type) return i
                return 0
            }
            implicitWidth: Math.round(150 * AppPalette.scale)
            fontPixelSize: Tokens.fontSm
            anchors.verticalCenter: parent.verticalCenter
            onActivated: function (i) {
                var id = root._types[i].id
                if (id === root._type) return
                switch (id) {
                case "none": root._emit({ type: "none" }); break
                case "bytes": root._emit({ type: "bytes", hex: "00" }); break
                case "bits": root._emit({ type: "bits", binary: "1010" }); break
                case "text": root._emit({ type: "text", ascii: "OK" }); break
                case "format": root._emit({ type: "format", format: root._formats[0].id, values: {} }); break
                case "position": root._emit({ type: "position", source: "headNav" }); break
                }
            }
        }
    }

    Row {
        visible: root._type === "bytes"
        spacing: Tokens.spaceSm
        leftPadding: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        UsblHexField {
            value: root._type === "bytes" ? Payload.prettyHex(root.spec.hex) : ""
            placeholder: qsTr("hex bytes")
            implicitWidth: Math.round(150 * AppPalette.scale)
            onCommitted: function (v) {
                var d = { type: "bytes", hex: Payload.normalizeHex(v) }
                if (root.spec.bits) d.bits = root.spec.bits
                root._emit(d)
            }
        }
        Text {
            text: qsTr("send")
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblSpin {
            from: 0; to: Payload.MAX_BITS; stepSize: 1
            devValue: root._type === "bytes" && root.spec.bits ? root.spec.bits : 0
            implicitWidth: Math.round(84 * AppPalette.scale)
            anchors.verticalCenter: parent.verticalCenter
            writeBack: function (v) {
                var d = { type: "bytes", hex: root.spec.hex }
                if (v > 0) d.bits = v
                root._emit(d)
            }
        }
        Text {
            text: qsTr("bit (0 = all)")
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    NodesTextField {
        visible: root._type === "bits"
        x: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        width: Math.max(Math.round(120 * AppPalette.scale), root.width - x)
        monospace: true
        value: root._type === "bits" ? root.spec.binary : ""
        placeholder: qsTr("bits, first sent first: 1011…")
        validator: RegularExpressionValidator { regularExpression: /[01 ]*/ }
        onCommitted: function (v) { root._emit({ type: "bits", binary: v.replace(/[^01]/g, "") }) }
    }

    NodesTextField {
        visible: root._type === "text"
        x: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        width: Math.max(Math.round(120 * AppPalette.scale), root.width - x)
        value: root._type === "text" ? root.spec.ascii : ""
        placeholder: qsTr("ASCII text")
        onCommitted: function (v) { root._emit({ type: "text", ascii: v }) }
    }

    Column {
        visible: root._type === "format"
        x: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        width: root.width - x
        spacing: Tokens.spaceXs
        readonly property var _f: root._type === "format" ? Payload.formatById(root._formats, root.spec.format) : null
        KCombo {
            model: root._formats.map(function (f) { return f.name + " · " + Payload.formatBits(f) + " bit" })
            currentIndex: {
                for (var i = 0; i < root._formats.length; ++i) if (root._type === "format" && root._formats[i].id === root.spec.format) return i
                return 0
            }
            implicitWidth: Math.round(170 * AppPalette.scale)
            fontPixelSize: Tokens.fontSm
            onActivated: function (i) { root._emit({ type: "format", format: root._formats[i].id, values: root.spec.values || {} }) }
        }
        Repeater {
            model: parent._f ? parent._f.fields : []
            delegate: Row {
                required property var modelData
                spacing: Tokens.spaceSm
                Text {
                    text: modelData.name + (modelData.unit ? ", " + modelData.unit : "")
                    color: AppPalette.text; font.pixelSize: Tokens.fontSm
                    width: Math.round(90 * AppPalette.scale)
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                NodesTextField {
                    implicitWidth: Math.round(90 * AppPalette.scale)
                    value: {
                        var v = root.spec.values ? root.spec.values[modelData.name] : undefined
                        return (v === undefined || v === null) ? "" : String(v)
                    }
                    placeholder: "0"
                    onCommitted: function (v) {
                        var vals = {}
                        var src = root.spec.values || {}
                        for (var k in src) vals[k] = src[k]
                        vals[modelData.name] = modelData.type === "bool" ? (v === "1" || v === "true") : Number(v)
                        root._emit({ type: "format", format: root.spec.format, values: vals })
                    }
                }
                Text {
                    text: modelData.bits + " bit · " + modelData.type
                    color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Text {
        visible: root._type === "position"
        x: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        width: root.width - x
        wrapMode: Text.WordWrap
        text: qsTr("The head's own position and geographic azimuth, 66 bit, filled from its navigation at the moment it sends. Decoded values are not checked strongly.")
        color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
    }

    Text {
        visible: root._type !== "none"
        x: Math.round(84 * AppPalette.scale) + Tokens.spaceSm
        width: root.width - x
        wrapMode: Text.WordWrap
        text: qsTr("%1 bit · %2 ms on air · limit %3 bit")
              .arg(root._bits).arg(Math.round(Timing.airMs(root._bits))).arg(root.maxBits)
        color: root._over ? AppPalette.dangerText : AppPalette.textSecond
        font.pixelSize: Tokens.fontSm
        font.bold: root._over
    }
}

import QtQuick 2.15
import kqml_types 1.0

PipelineStatusGroup {
    id: panel

    readonly property var ctrlRef: (typeof MosaicViewControlMenuController !== "undefined")
                                   ? MosaicViewControlMenuController : null
    readonly property var st: ctrlRef ? ctrlRef.pipelineStatus : ({})
    readonly property bool hasData: st && st.state !== undefined

    ctrl: ctrlRef
    fmt: panel._format
    verdict: _verdict()
    sections: [
        { title: qsTr("Pipeline"),   rows: panel.pipelineRows },
        { title: qsTr("Epoch probe"),rows: panel.probeRows },
        { title: qsTr("Beams"),      rows: panel.beamRows },
        { title: qsTr("Channels"),   rows: panel.channelRows },
        { title: qsTr("Geometry"),   rows: panel.geometryRows },
        { title: qsTr("Renderer"),   rows: panel.renderRows }
    ]

    function _num(key) {
        var v = st ? st[key] : undefined
        return v === undefined ? NaN : Number(v)
    }

    function _flag(key) {
        var v = st ? st[key] : undefined
        return v === true
    }

    function _int(key) {
        var v = _num(key)
        return isNaN(v) ? "—" : String(Math.round(v))
    }

    function _real(key, digits) {
        var v = _num(key)
        if (isNaN(v) || Math.abs(v) > 1e30)
            return "—"
        return v.toFixed(digits)
    }

    function _onOff(key) {
        var v = st ? st[key] : undefined
        return v === undefined ? "—" : (v ? qsTr("on") : qsTr("off"))
    }

    function _text(key) {
        var v = st ? st[key] : undefined
        if (v === undefined)
            return "—"
        return String(v).length ? String(v) : qsTr("none")
    }

    function _stateText() {
        switch (_num("state")) {
        case 0:  return qsTr("idle")
        case 1:  return qsTr("bottom track")
        case 2:  return qsTr("isobaths")
        case 3:  return qsTr("mosaic")
        case 4:  return qsTr("surface")
        default: return "—"
        }
    }

    function _sourceText() {
        switch (_num("mosaicSource")) {
        case 0:  return qsTr("amplitude")
        case 1:  return qsTr("side-scan")
        case 2:  return qsTr("TGC")
        default: return "—"
        }
    }

    function _format(row) {
        switch (row.kind) {
        case "state":  return _stateText()
        case "source": return _sourceText()
        case "onoff":  return _onOff(row.key)
        case "text":   return _text(row.key)
        case "usable": return _int("probeUsableFirst") + " / " + _int("probeUsableSecond")
        case "pairs":  return _int("probePairsFirst") + " / " + _int("probePairsSecond")
        case "real0":  return _real(row.key, 0)
        case "real1":  return _real(row.key, 1)
        case "real2":  return _real(row.key, 2)
        default:       return _int(row.key)
        }
    }

    readonly property bool anyChannel: (st && st.mosaicFirstChannel !== undefined
                                        && String(st.mosaicFirstChannel).length > 0)
                                       || (st && st.mosaicSecondChannel !== undefined
                                           && String(st.mosaicSecondChannel).length > 0)
    readonly property real usablePairs: Math.max(_num("probePairsFirst") || 0, _num("probePairsSecond") || 0)
    readonly property real usableEpochs: Math.max(_num("probeUsableFirst") || 0, _num("probeUsableSecond") || 0)
    readonly property real anyBeam: Math.max(_num("probeFirstBeam") || 0, _num("probeSecondBeam") || 0)
    readonly property real anyBottom: Math.max(_num("probeFirstBottom") || 0, _num("probeSecondBottom") || 0)

    function _verdict() {
        if (!hasData)
            return { text: qsTr("Waiting for the first snapshot…"), ok: false, neutral: true }
        if (!_flag("updateMosaic"))
            return { text: qsTr("Mosaic computation is off — nothing will be built"), ok: false, neutral: false }
        if (!_flag("mosaicRequested"))
            return { text: qsTr("Mosaic is not enabled in the scene"), ok: false, neutral: true }
        if (!anyChannel)
            return { text: qsTr("No mosaic channel selected — pick a side-scan channel"), ok: false, neutral: false }
        if (_num("probeChecked") <= 0)
            return { text: qsTr("No epochs in the dataset window"), ok: false, neutral: false }
        if (_num("probeValid") <= 0)
            return { text: qsTr("No valid epochs in the probed window"), ok: false, neutral: false }
        if (_num("probeWithPos") <= 0)
            return { text: qsTr("No sonar position — mosaic cannot place a strip"), ok: false, neutral: false }
        if (_num("probeWithYaw") <= 0)
            return { text: qsTr("No valid yaw — the strip cannot be oriented"), ok: false, neutral: false }
        if (anyBeam <= 0)
            return { text: qsTr("No beam on the selected channels — check channel and subchannel"), ok: false, neutral: false }
        if (anyBottom <= 0)
            return { text: qsTr("No bottom point on the beams — mosaic cannot project them"), ok: false, neutral: false }
        if (usableEpochs <= 0)
            return { text: qsTr("Position, yaw, beam and bottom never come together in one epoch"), ok: false, neutral: false }
        if (usablePairs <= 0)
            return { text: qsTr("Usable epochs are not adjacent — a strip needs two neighbours in a row"), ok: false, neutral: false }
        if (!_flag("renderMosaicOn"))
            return { text: qsTr("Renderer has the mosaic layer disabled"), ok: false, neutral: false }
        if (_num("renderTiles") <= 0)
            return { text: qsTr("Tiles never reached the renderer"), ok: false, neutral: false }
        return { text: qsTr("Pipeline healthy — mosaic strips are being laid"), ok: true, neutral: false }
    }

    readonly property var pipelineRows: [
        { label: qsTr("State"),       key: "state",            kind: "state" },
        { label: qsTr("Mosaic calc"), key: "updateMosaic",     kind: "onoff" },
        { label: qsTr("In 3D"),       key: "mosaicRequested",  kind: "onoff" },
        { label: qsTr("BT calc"),     key: "updateBottomTrack",kind: "onoff" },
        { label: qsTr("Opening"),     key: "openingFile",      kind: "onoff" },
        { label: qsTr("Zeroing"),     key: "zeroing",          kind: "onoff" },
        { label: qsTr("Fake last N"), key: "fakeCoordsLastN",  kind: "int" },
        { label: qsTr("Tile DB"),     key: "dbReady",          kind: "onoff" }
    ]

    readonly property var probeRows: [
        { label: qsTr("Window"),   key: "probeWindow",  kind: "int" },
        { label: qsTr("Checked"),  key: "probeChecked", kind: "int" },
        { label: qsTr("Valid"),    key: "probeValid",   kind: "int" },
        { label: qsTr("Position"), key: "probeWithPos", kind: "int" },
        { label: qsTr("Yaw"),      key: "probeWithYaw", kind: "int" }
    ]

    readonly property var beamRows: [
        { label: qsTr("Left beam"),    key: "probeFirstBeam",    kind: "int" },
        { label: qsTr("Right beam"),   key: "probeSecondBeam",   kind: "int" },
        { label: qsTr("Left bottom"),  key: "probeFirstBottom",  kind: "int" },
        { label: qsTr("Right bottom"), key: "probeSecondBottom", kind: "int" },
        { label: qsTr("Usable L / R"), key: "probeUsableFirst",  kind: "usable" },
        { label: qsTr("Pairs L / R"),  key: "probePairsFirst",   kind: "pairs" }
    ]

    readonly property var channelRows: [
        { label: qsTr("Left ch"),  key: "mosaicFirstChannel",  kind: "text" },
        { label: qsTr("Left sub"), key: "mosaicFirstSub",      kind: "int" },
        { label: qsTr("Right ch"), key: "mosaicSecondChannel", kind: "text" },
        { label: qsTr("Right sub"),key: "mosaicSecondSub",     kind: "int" },
        { label: qsTr("Source"),   key: "mosaicSource",        kind: "source" },
        { label: qsTr("Level low"), key: "levelLow",           kind: "real0" },
        { label: qsTr("Level high"),key: "levelHigh",          kind: "real0" }
    ]

    readonly property var geometryRows: [
        { label: qsTr("L offset, °"), key: "mosaicLAngleOffset",  kind: "real1" },
        { label: qsTr("R offset, °"), key: "mosaicRAngleOffset",  kind: "real1" },
        { label: qsTr("Res, px/m"),   key: "mosaicTileResolution",kind: "real1" },
        { label: qsTr("Last calc"),   key: "mosaicLastCalcEpoch", kind: "int" },
        { label: qsTr("Last accept"), key: "mosaicLastAccepted",  kind: "int" },
        { label: qsTr("Last trace"),  key: "mosaicLastTraceLine", kind: "int" },
        { label: qsTr("Queue"),       key: "queuedMosaic",        kind: "int" },
        { label: qsTr("Last idx"),    key: "lastMosaicIndx",      kind: "int" }
    ]

    readonly property var renderRows: [
        { label: qsTr("Tiles"),      key: "renderTiles",      kind: "int" },
        { label: qsTr("Vis. keys"),  key: "visibleTileKeys",  kind: "int" },
        { label: qsTr("Mesh tiles"), key: "meshTilesInited",  kind: "int" },
        { label: qsTr("Mesh zoom"),  key: "meshZoom",         kind: "int" },
        { label: qsTr("Mesh init"),  key: "meshInited",       kind: "onoff" },
        { label: qsTr("Has data"),   key: "meshHasData",      kind: "onoff" },
        { label: qsTr("Mosaic layer"), key: "renderMosaicOn", kind: "onoff" },
        { label: qsTr("Iso layer"),  key: "renderIsobathsOn", kind: "onoff" },
        { label: qsTr("Meas. lines"), key: "measLineVisible", kind: "onoff" },
        { label: qsTr("Grid"),       key: "gridVisible",      kind: "onoff" }
    ]
}

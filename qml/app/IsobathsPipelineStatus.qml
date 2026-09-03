import QtQuick 2.15
import kqml_types 1.0

PipelineStatusGroup {
    id: panel

    readonly property var ctrlRef: (typeof IsobathsViewControlMenuController !== "undefined")
                                   ? IsobathsViewControlMenuController : null
    readonly property var st: ctrlRef ? ctrlRef.pipelineStatus : ({})
    readonly property bool hasData: st && st.state !== undefined

    ctrl: ctrlRef
    fmt: panel._format
    verdict: _verdict()
    sections: [
        { title: qsTr("Pipeline"),      rows: panel.pipelineRows },
        { title: qsTr("Surface"),       rows: panel.surfaceRows },
        { title: qsTr("Renderer"),      rows: panel.renderRows },
        { title: qsTr("Input"),         rows: panel.inputRows },
        { title: qsTr("Triangulation"), rows: panel.triRows },
        { title: qsTr("Iso processor"), rows: panel.isoRows }
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

    function _format(row) {
        switch (row.kind) {
        case "state": return _stateText()
        case "onoff": return _onOff(row.key)
        case "grid":  return _int("meshTilesWide") + "×" + _int("meshTilesHigh")
        case "real0": return _real(row.key, 0)
        case "real1": return _real(row.key, 1)
        case "real2": return _real(row.key, 2)
        default:      return _int(row.key)
        }
    }

    function _verdict() {
        if (!hasData)
            return { text: qsTr("Waiting for the first snapshot…"), ok: false, neutral: true }
        if (!_flag("processEnabled") || !_flag("updateSurface"))
            return { text: qsTr("Surface computation is off — nothing will be built"), ok: false, neutral: false }
        if (!_flag("updateIsobaths"))
            return { text: qsTr("Isobaths computation is off in the processor"), ok: false, neutral: false }
        if (!_flag("isobathsRequested"))
            return { text: qsTr("Isobaths are not enabled in the scene"), ok: false, neutral: true }
        if (_num("meshPoints") <= 0 && _num("triPoints") <= 0)
            return { text: qsTr("No depth points reached triangulation"), ok: false, neutral: false }
        if (!_flag("meshInited") || !_flag("meshHasData"))
            return { text: qsTr("Surface mesh is empty — no points accepted"), ok: false, neutral: false }
        if (_num("triTriangles") <= 0)
            return { text: qsTr("Triangulation produced no triangles"), ok: false, neutral: false }
        if (_num("meshTilesInited") <= 0)
            return { text: qsTr("No mesh tiles initialised"), ok: false, neutral: false }
        if (_num("colorIntervals") <= 0)
            return { text: qsTr("Colour intervals are empty — isobath levels cannot be derived"), ok: false, neutral: false }
        if (_num("renderTiles") <= 0)
            return { text: qsTr("Tiles never reached the renderer"), ok: false, neutral: false }
        if (!_flag("renderIsobathsOn"))
            return { text: qsTr("Renderer has isobaths disabled"), ok: false, neutral: false }
        if (_num("renderIsoLabels") <= 0)
            return { text: qsTr("Surface is drawn, but no isobath labels were built"), ok: false, neutral: false }
        return { text: qsTr("Pipeline healthy — surface and isobaths are live"), ok: true, neutral: false }
    }

    readonly property var pipelineRows: [
        { label: qsTr("State"),        key: "state",            kind: "state" },
        { label: qsTr("Surface calc"), key: "updateSurface",    kind: "onoff" },
        { label: qsTr("Iso calc"),     key: "updateIsobaths",   kind: "onoff" },
        { label: qsTr("In 3D"),        key: "isobathsRequested",kind: "onoff" },
        { label: qsTr("Opening"),      key: "openingFile",      kind: "onoff" },
        { label: qsTr("BT busy"),      key: "bottomTrackBusy",  kind: "onoff" },
        { label: qsTr("Tile DB"),      key: "dbReady",          kind: "onoff" }
    ]

    readonly property var inputRows: [
        { label: qsTr("Chart idx"),   key: "lastChartIndx",   kind: "int" },
        { label: qsTr("Epoch idx"),   key: "lastEpochIndx",   kind: "int" },
        { label: qsTr("Surf. queue"), key: "queuedSurface",   kind: "int" },
        { label: qsTr("BT queue"),    key: "queuedBtVertices",kind: "int" }
    ]

    readonly property var triRows: [
        { label: qsTr("Points"),    key: "triPoints",    kind: "int" },
        { label: qsTr("Triangles"), key: "triTriangles", kind: "int" },
        { label: qsTr("In mesh"),   key: "meshPoints",   kind: "int" }
    ]

    readonly property var surfaceRows: [
        { label: qsTr("Mesh init"), key: "meshInited",        kind: "onoff" },
        { label: qsTr("Has data"),  key: "meshHasData",       kind: "onoff" },
        { label: qsTr("Tiles"),     key: "meshTilesInited",   kind: "int" },
        { label: qsTr("Zoom"),      key: "meshZoom",          kind: "int" },
        { label: qsTr("Grid"),      key: "meshTilesWide",     kind: "grid" },
        { label: qsTr("Step, m"),   key: "surfaceStep",       kind: "real2" },
        { label: qsTr("Min Z, m"),  key: "surfaceMinZ",       kind: "real1" },
        { label: qsTr("Max Z, m"),  key: "surfaceMaxZ",       kind: "real1" },
        { label: qsTr("Colours"),   key: "colorIntervals",    kind: "int" },
        { label: qsTr("Edge, m"),   key: "surfaceEdgeLimit",  kind: "real0" },
        { label: qsTr("Extra, m"),  key: "surfaceExtraWidth", kind: "int" }
    ]

    readonly property var isoRows: [
        { label: qsTr("Mesh linked"), key: "isoHasMesh",      kind: "onoff" },
        { label: qsTr("Segments"),    key: "isoLineSegments", kind: "int" },
        { label: qsTr("Labels"),      key: "isoLabels",       kind: "int" },
        { label: qsTr("Step, m"),     key: "isoLineStep",     kind: "real2" }
    ]

    readonly property var renderRows: [
        { label: qsTr("Tiles"),     key: "renderTiles",       kind: "int" },
        { label: qsTr("Labels"),    key: "renderIsoLabels",   kind: "int" },
        { label: qsTr("Vis. keys"), key: "visibleTileKeys",   kind: "int" },
        { label: qsTr("Step, m"),   key: "renderSurfaceStep", kind: "real2" },
        { label: qsTr("Iso layer"), key: "renderIsobathsOn",  kind: "onoff" },
        { label: qsTr("Mosaic"),    key: "renderMosaicOn",    kind: "onoff" }
    ]
}

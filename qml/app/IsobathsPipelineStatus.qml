import QtQuick 2.15
import kqml_types 1.0

SettingsGroup {
    id: panel

    property bool active: false

    readonly property var ctrl: (typeof IsobathsViewControlMenuController !== "undefined")
                                ? IsobathsViewControlMenuController : null
    readonly property var st: ctrl ? ctrl.pipelineStatus : ({})
    readonly property bool hasData: st && st.state !== undefined
    readonly property var verdict: _verdict()
    visible: active
    width: parent ? parent.width : preferredWidth
    preferredWidth: width

    title: qsTr("Pipeline status")
    contentSpacing: 0
    collapsedByDefault: true
    scrollIntoViewOnExpand: false
    headerTint: verdict.neutral ? "transparent"
                : verdict.ok ? AppPalette.linkOkBg
                : AppPalette.dangerBg

    onActiveChanged: _applyMonitor()
    onExpandedChanged: _applyMonitor()
    Component.onCompleted: _applyMonitor()
    Component.onDestruction: if (ctrl) ctrl.statusMonitorEnabled = false

    function _applyMonitor() {
        if (!ctrl)
            return

        ctrl.statusDetailedPolling = panel.expanded
        ctrl.statusMonitorEnabled = panel.active
    }

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

    component StatusSection: Column {
        id: section

        required property string title
        required property var rows
        required property var fmt
        property color flashColor: AppPalette.linkOkBorder

        spacing: 0

        Item {
            width: section.width
            height: Math.round(Tokens.fontSm * 1.5) + Tokens.spaceXs

            Text {
                text: section.title
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                font.bold: true
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.4
                anchors.left: parent.left
                anchors.top: parent.top
            }

            Rectangle {
                height: Math.max(1, Math.round(AppPalette.scale))
                color: AppPalette.separator
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Math.round(Tokens.spaceXs / 2)
            }
        }

        Repeater {
            model: section.rows

            Item {
                id: statusRow

                width: section.width
                height: Math.round(Tokens.fontBase * 1.6)

                Text {
                    text: modelData.label
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm
                    elide: Text.ElideRight
                    anchors.left: parent.left
                    anchors.right: rowValue.left
                    anchors.rightMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    id: rowValue

                    property real highlight: 0.0

                    readonly property color baseColor: text === "—" ? AppPalette.textMuted : AppPalette.text

                    text: section.fmt(modelData)
                    color: highlight > 0.0
                           ? Qt.tint(baseColor, Qt.rgba(section.flashColor.r,
                                                        section.flashColor.g,
                                                        section.flashColor.b,
                                                        highlight))
                           : baseColor
                    font.pixelSize: Tokens.fontSm
                    font.bold: true
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    onTextChanged: highlightAnim.restart()

                    SequentialAnimation {
                        id: highlightAnim

                        NumberAnimation {
                            target: rowValue
                            property: "highlight"
                            to: 1.0
                            duration: 150
                            easing.type: Easing.OutQuad
                        }
                        NumberAnimation {
                            target: rowValue
                            property: "highlight"
                            to: 0.0
                            duration: 1200
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }
    }

    Item {
        id: verdictSlot

        readonly property int sideInset: Tokens.spaceMd
        readonly property int innerPad: Tokens.spaceMd + Tokens.spaceXs
        readonly property int topGap: Tokens.spaceMd
        readonly property int bottomGap: Tokens.spaceMd + Tokens.spaceSm

        width: parent.width
        height: verdictBox.height + topGap + bottomGap

        Rectangle {
            id: verdictBox

            readonly property color accent: panel.verdict.neutral ? AppPalette.border
                                            : panel.verdict.ok ? AppPalette.linkOkBorder
                                            : AppPalette.dangerBorder
            readonly property int dotSize: Math.round(6 * AppPalette.scale)

            anchors.top: parent.top
            anchors.topMargin: verdictSlot.topGap
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: verdictSlot.sideInset
            anchors.rightMargin: verdictSlot.sideInset

            radius: Tokens.radiusMd
            height: verdictText.implicitHeight + verdictSlot.innerPad * 2
            color: panel.verdict.neutral ? AppPalette.bg
                   : panel.verdict.ok ? AppPalette.linkOkBg
                   : AppPalette.dangerBg
            border.width: Math.max(1, Math.round(AppPalette.scale))
            border.color: verdictBox.accent

            Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            Behavior on border.color { ColorAnimation { duration: Anim.disclosureMs } }

            Rectangle {
                width: verdictBox.dotSize
                height: width
                radius: width / 2
                color: verdictBox.accent
                anchors.left: parent.left
                anchors.leftMargin: verdictSlot.innerPad
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            }

            Text {
                id: verdictText

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: verdictSlot.innerPad * 2 + verdictBox.dotSize
                anchors.rightMargin: verdictSlot.innerPad
                text: panel.verdict.text
                wrapMode: Text.WordWrap
                font.pixelSize: Tokens.fontBase
                color: panel.verdict.neutral ? AppPalette.textSecond
                       : panel.verdict.ok ? AppPalette.linkOkText
                       : AppPalette.dangerText

                Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            }
        }
    }

    Grid {
        id: grid

        readonly property int minColW: Math.round(150 * AppPalette.scale)

        width: parent.width
        columnSpacing: Tokens.spaceLg
        rowSpacing: Tokens.spaceLg
        columns: width <= 0 ? 3
                 : (width - columnSpacing * 2) / 3 >= minColW ? 3
                 : (width - columnSpacing) / 2 >= minColW ? 2
                 : 1

        readonly property real colW: Math.floor((width - columnSpacing * (columns - 1)) / columns)

        StatusSection {
            width: grid.colW
            title: qsTr("Pipeline")
            rows: panel.pipelineRows
            fmt: panel._format
        }
        StatusSection {
            width: grid.colW
            title: qsTr("Surface")
            rows: panel.surfaceRows
            fmt: panel._format
        }
        StatusSection {
            width: grid.colW
            title: qsTr("Renderer")
            rows: panel.renderRows
            fmt: panel._format
        }
        StatusSection {
            width: grid.colW
            title: qsTr("Input")
            rows: panel.inputRows
            fmt: panel._format
        }
        StatusSection {
            width: grid.colW
            title: qsTr("Triangulation")
            rows: panel.triRows
            fmt: panel._format
        }
        StatusSection {
            width: grid.colW
            title: qsTr("Iso processor")
            rows: panel.isoRows
            fmt: panel._format
        }
    }
}

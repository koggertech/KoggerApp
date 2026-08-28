import QtQuick 2.15
import kqml_types 1.0

Item {
    id: overlay

    required property var store

    anchors.fill: parent
    // GRID PANELS ONLY. This overlay exists to be a drop target for cells, and the other kind
    // has none -- an acoustic-nodes panel is laid out by the USBL plan, not here. Without the
    // kind gate it dims the scene and offers a 1x1 grid belonging to no panel.
    visible: !!(store && store.widgetEditorActive
                && store.widgetDraftKind !== "usblNodes"
                && store.widgetDraftKind !== "stand")

    readonly property real _s: AppPalette.appScale
    readonly property bool _touch: (typeof inputDeviceTracker !== "undefined" && inputDeviceTracker)
                                   ? inputDeviceTracker.touchMode : false
    readonly property var _sysbat: (typeof systemBattery !== "undefined") ? systemBattery : null
    readonly property real _k: _s * (store ? store.widgetDraftScale : 1.0)
    readonly property real _cell: Math.round(84 * _k)
    readonly property real _gap: Math.round(4 * _k)
    readonly property real _pad: Math.round(8 * _k)

    readonly property int _cols: store ? (store.widgetHoverCols > 0 ? store.widgetHoverCols : store.widgetDraftCols) : 1
    readonly property int _rows: store ? (store.widgetHoverRows > 0 ? store.widgetHoverRows : store.widgetDraftRows) : 1
    readonly property var _cells: store ? store.widgetDraftCells : []
    readonly property real _gridW: _cols * _cell + (_cols - 1) * _gap
    readonly property real _gridH: _rows * _cell + (_rows - 1) * _gap
    readonly property real _baseW: _cols * Math.round(84 * _s) + (_cols - 1) * Math.round(4 * _s) + Math.round(8 * _s) * 2
    readonly property real _baseH: _rows * Math.round(84 * _s) + (_rows - 1) * Math.round(4 * _s) + Math.round(8 * _s) * 2
    readonly property real _bgAlpha: Math.max(0, Math.min(1, 1 - (store ? store.widgetDraftTransparency : 0) / 100))

    readonly property real _visLeft: (store && store.settingsPanelOpen && store.settingsSide === "left") ? store.settingsPanelSizePx : 0
    readonly property real _visRight: (store && store.settingsPanelOpen && store.settingsSide === "right") ? store.settingsPanelSizePx : 0

    readonly property real _availW: Math.max(0, (width - _visLeft - _visRight)) * 0.94
    readonly property real _availH: Math.max(0, height) * 0.94
    readonly property real _minScale: (store && store.widgetScaleSteps.length > 0) ? store.widgetScaleSteps[0] : 0.75
    readonly property real _maxScale: Math.max(_minScale,
        Math.min(_baseW > 0 ? _availW / _baseW : 99, _baseH > 0 ? _availH / _baseH : 99))

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.82
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
            onWheel: function(wheel) { wheel.accepted = true }
        }
    }

    Rectangle {
        id: widgetCard
        width: overlay._gridW + overlay._pad * 2
        height: overlay._gridH + overlay._pad * 2
        x: overlay._visLeft + Math.round((overlay.width - overlay._visLeft - overlay._visRight - width) / 2)
        y: Math.round((overlay.height - height) / 2)
        radius: Tokens.radiusLg
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, overlay._bgAlpha)

        Item {
            width: overlay._gridW
            height: overlay._gridH
            anchors.centerIn: parent

            Repeater {
                model: overlay._cols * overlay._rows
                delegate: DropArea {
                    id: dropCell
                    required property int index
                    readonly property int cellRow: Math.floor(index / overlay._cols)
                    readonly property int cellCol: index % overlay._cols
                    readonly property var occ: overlay.store ? overlay.store.widgetDraftOccupantAt(cellRow, cellCol) : null
                    readonly property bool isAnchor: occ && occ.kind === "anchor"
                    readonly property bool isTail: occ && occ.kind === "tail"

                    x: cellCol * (overlay._cell + overlay._gap)
                    y: cellRow * (overlay._cell + overlay._gap)
                    width: overlay._cell + overlay._gap
                    height: overlay._cell + overlay._gap

                    onEntered: function(drag) {
                        if (!overlay.store) return
                        overlay.store.widgetOverPalette = false
                        overlay.store.widgetDropRow = cellRow
                        overlay.store.widgetDropCol = cellCol
                        var rep = drag.source ? drag.source.representationType : "value"
                        var big = drag.source ? (drag.source.big === true) : false
                        var field = drag.source ? drag.source.fieldKey : ""
                        var isMove = !!(drag.source && drag.source.isMove === true)
                        var s = overlay.store._cellSpan(rep, big)
                        overlay.store.widgetDropSpan = s.sc
                        overlay.store.widgetDropSpanRows = s.sr
                        overlay.store.widgetDropValid = isMove
                            ? overlay.store._widgetDraftFits(cellRow, cellCol, rep, big, field)
                            : overlay.store.widgetDraftCanDrop(cellRow, cellCol, rep, big, field)
                    }

                    onExited: {
                        if (!overlay.store) return
                        overlay.store.widgetDropRow = -1
                        overlay.store.widgetDropCol = -1
                    }

                    Rectangle {
                        width: overlay._cell
                        height: overlay._cell
                        visible: !dropCell.isTail && !dropCell.isAnchor && !dropCell.containsDrag
                        readonly property bool _isTarget: !!overlay.store && overlay.store.widgetDragActive
                            && overlay.store._widgetDraftFits(dropCell.cellRow, dropCell.cellCol,
                                   overlay.store.widgetDragRep, overlay.store.widgetDragBig, overlay.store.widgetDragField)
                        radius: Tokens.radiusSm
                        color: "transparent"
                        border.width: _isTarget ? Math.max(2, Math.round(2 * overlay._s)) : 1
                        border.color: _isTarget ? AppPalette.accentBorder
                                                : Qt.rgba(AppPalette.border.r, AppPalette.border.g, AppPalette.border.b, 0.5)
                        Behavior on border.width { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        Behavior on border.color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
                    }

                    Rectangle {
                        readonly property bool _isDrag: dropCell.containsDrag
                        readonly property bool _isPreview: !_isDrag && !!overlay.store
                            && overlay.store.widgetPreviewRow === dropCell.cellRow
                            && overlay.store.widgetPreviewCol === dropCell.cellCol
                        readonly property bool _show: _isDrag || _isPreview
                        readonly property bool _drag: !!overlay.store && overlay.store.widgetDragActive
                        z: 3
                        readonly property int _spanC0: overlay.store ? (_drag ? overlay.store.widgetDropSpan : overlay.store.widgetPreviewSpan) : 1
                        readonly property int _spanR0: overlay.store ? (_drag ? overlay.store.widgetDropSpanRows : overlay.store.widgetPreviewSpanRows) : 1
                        readonly property bool _valid: !!overlay.store && (_drag ? overlay.store.widgetDropValid : overlay.store.widgetPreviewValid)
                        readonly property int _span: Math.min(_spanC0, Math.max(1, overlay._cols - dropCell.cellCol))
                        readonly property int _spanRows: Math.min(_spanR0, Math.max(1, overlay._rows - dropCell.cellRow))
                        x: 0
                        y: 0
                        width: _span * overlay._cell + (_span - 1) * overlay._gap
                        height: _spanRows * overlay._cell + (_spanRows - 1) * overlay._gap
                        radius: Tokens.radiusSm
                        color: _valid ? "#3316A34A" : "#33EF4444"
                        border.width: 1
                        border.color: _valid ? "#16A34A" : "#EF4444"
                        opacity: _show ? 1 : 0
                        visible: opacity > 0.01
                        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                    }

                    Item {
                        id: occCard
                        visible: dropCell.isAnchor
                        z: 6
                        readonly property bool isMove: true
                        readonly property bool big: dropCell.isAnchor ? (dropCell.occ.cell.big === true) : false
                        readonly property var _sp: dropCell.isAnchor
                            ? overlay.store._cellSpan(dropCell.occ.cell.rep, big)
                            : { sc: 1, sr: 1 }
                        width: _sp.sc * overlay._cell + (_sp.sc - 1) * overlay._gap
                        height: _sp.sr * overlay._cell + (_sp.sr - 1) * overlay._gap

                        property string fieldKey: dropCell.isAnchor ? dropCell.occ.cell.field : ""
                        property string representationType: dropCell.isAnchor ? dropCell.occ.cell.rep : "value"
                        readonly property string _label: fieldKey ? DataFieldCatalog.label(fieldKey) : ""
                        readonly property string _value: fieldKey
                            ? DataFieldCatalog.previewValue(fieldKey, overlay.store, overlay._sysbat)
                            : ""

                        Drag.active: dragHandle.dragActive
                        Drag.source: occCard
                        Drag.hotSpot.x: dragHandle.x + dragHandle.width / 2
                        Drag.hotSpot.y: dragHandle.y + dragHandle.height / 2

                        readonly property bool _dragging: dragHandle.dragActive
                        on_DraggingChanged: {
                            if (!overlay.store) return
                            if (_dragging) overlay.store.widgetDragBegin(representationType, big, fieldKey)
                            else overlay.store.widgetDragEnd()
                        }

                        states: State {
                            when: dragHandle.dragActive
                            ParentChange { target: occCard; parent: overlay.store ? overlay.store.widgetDragLayer : null }
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: Tokens.radiusSm
                            readonly property bool _hot: dragHandle.dragActive || occHover.hovered
                            color: dragHandle.dragActive ? Qt.rgba(1, 1, 1, 0.06) : "transparent"
                            border.width: _hot ? Math.max(2, Math.round(2 * overlay._s)) : 0
                            border.color: AppPalette.accentBorder
                            Behavior on border.width { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                            Behavior on color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        }

                        WidgetCellContent {
                            anchors.fill: parent
                            rep: occCard.representationType
                            label: occCard._label
                            value: occCard._value
                            k: overlay._k
                            gap: overlay._gap
                        }

                        HoverHandler { id: occHover; enabled: !overlay._touch }

                        KCircleIconButton {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: Math.round(3 * overlay._s)
                            width: Math.round(28 * overlay._s)
                            height: Math.round(28 * overlay._s)
                            rounded: true
                            glyph: "×"
                            glyphPixelSize: Math.round(20 * overlay._s)
                            glyphColor: AppPalette.textSecond
                            fillColor: AppPalette.controlRaised
                            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
                            borderColor: AppPalette.border
                            borderHoverColor: AppPalette.borderHover
                            visible: true
                            toolTipText: qsTr("Remove widget")
                            z: 8
                            onClicked: if (overlay.store) overlay.store.widgetDraftRemove(occCard.fieldKey)
                        }

                        MouseArea {
                            id: occTap
                            anchors.fill: parent
                            z: 5
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (overlay.store) overlay.store.widgetDraftCycle(occCard.fieldKey)
                        }

                        WidgetDragHandle {
                            id: dragHandle
                            z: 7
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.margins: Math.round(3 * overlay._s)
                            diameter: Math.round(28 * overlay._s)
                            dragTarget: occCard
                            onReleased: if (overlay.store) overlay.store.widgetDraftCommitMoveOrRemove(occCard.fieldKey, occCard.representationType, occCard.big)
                        }
                    }
                }
            }
        }

        Item {
            id: resizeGrip
            visible: true
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: -width / 2
            anchors.bottomMargin: -height / 2
            width: Math.round(26 * overlay._s)
            height: Math.round(26 * overlay._s)
            z: 30

            Repeater {
                model: [{ gx: 13, gy: 3 }, { gx: 8, gy: 8 }, { gx: 13, gy: 8 },
                        { gx: 3, gy: 13 }, { gx: 8, gy: 13 }, { gx: 13, gy: 13 }]
                delegate: Rectangle {
                    required property var modelData
                    x: modelData.gx * overlay._s
                    y: modelData.gy * overlay._s
                    width: Math.max(1, Math.round(2 * overlay._s))
                    height: width
                    radius: Math.max(1, Math.round(1 * overlay._s))
                    color: gripMouse.pressed ? "#93C5FD" : (gripMouse.containsMouse ? "#CBD5E1" : "#94A3B8")
                }
            }

            MouseArea {
                id: gripMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeFDiagCursor
                property real _px: 0
                property real _startScale: 1.0
                onPressed: function(mouse) {
                    if (!overlay.store) return
                    var p = mapToItem(overlay, mouse.x, mouse.y)
                    _px = p.x
                    _startScale = overlay.store.widgetDraftScale
                }
                onPositionChanged: function(mouse) {
                    if (!pressed || !overlay.store || overlay._baseW <= 0) return
                    var p = mapToItem(overlay, mouse.x, mouse.y)
                    var rawScale = (overlay._baseW * _startScale + (p.x - _px)) / overlay._baseW
                    overlay.store.widgetDraftSetScale(rawScale, overlay._maxScale)
                }
            }
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Item {
    id: overlay

    required property var store

    anchors.fill: parent
    visible: !!(store && store.widgetEditorActive)

    readonly property var _ds: (typeof dataset !== "undefined") ? dataset : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null

    readonly property real _s: AppPalette.appScale
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
    readonly property real _bgAlpha: Math.max(0, Math.min(1, 1 - (store ? store.widgetDraftTransparency : 0) / 100))

    readonly property real _visLeft: (store && store.settingsPanelOpen && store.settingsSide === "left") ? store.settingsPanelSizePx : 0
    readonly property real _visRight: (store && store.settingsPanelOpen && store.settingsSide === "right") ? store.settingsPanelSizePx : 0

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.6
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
                    width: overlay._cell
                    height: overlay._cell

                    onEntered: function(drag) {
                        if (!overlay.store) return
                        overlay.store.widgetOverPalette = false
                        overlay.store.widgetDropRow = cellRow
                        overlay.store.widgetDropCol = cellCol
                        var rep = drag.source ? drag.source.representationType : "value"
                        overlay.store.widgetDropSpan = (rep === "labelValueRow") ? 2 : 1
                        overlay.store.widgetDropValid = overlay.store.widgetDraftCanDrop(cellRow, cellCol,
                                                        rep, drag.source ? drag.source.fieldKey : "")
                    }

                    onExited: {
                        if (!overlay.store) return
                        overlay.store.widgetDropRow = -1
                        overlay.store.widgetDropCol = -1
                        overlay.store.widgetDropValid = false
                    }

                    Rectangle {
                        anchors.fill: parent
                        visible: !dropCell.isTail && !dropCell.isAnchor && !dropCell.containsDrag
                        radius: Tokens.radiusSm
                        color: "transparent"
                        border.width: 1
                        border.color: Qt.rgba(AppPalette.border.r, AppPalette.border.g, AppPalette.border.b, 0.5)
                    }

                    Rectangle {
                        visible: dropCell.containsDrag
                        z: 3
                        readonly property int _span: {
                            var s = overlay.store ? overlay.store.widgetDropSpan : 1
                            return Math.min(s, Math.max(1, overlay._cols - dropCell.cellCol))
                        }
                        x: 0
                        y: 0
                        width: _span * overlay._cell + (_span - 1) * overlay._gap
                        height: overlay._cell
                        radius: Tokens.radiusSm
                        color: (overlay.store && overlay.store.widgetDropValid) ? "#3316A34A" : "#33EF4444"
                        border.width: 1
                        border.color: (overlay.store && overlay.store.widgetDropValid) ? "#16A34A" : "#EF4444"
                    }

                    Item {
                        id: occCard
                        visible: dropCell.isAnchor
                        z: 6
                        width: (dropCell.isAnchor && dropCell.occ.cell.rep === "labelValueRow")
                               ? overlay._cell * 2 + overlay._gap : overlay._cell
                        height: overlay._cell

                        property string fieldKey: dropCell.isAnchor ? dropCell.occ.cell.field : ""
                        property string representationType: dropCell.isAnchor ? dropCell.occ.cell.rep : "value"
                        readonly property string _label: fieldKey ? DataFieldCatalog.label(fieldKey) : ""
                        readonly property string _value: fieldKey
                            ? DataFieldCatalog.formatValue(fieldKey, overlay._ds, overlay._dmw, overlay.store)
                            : ""

                        Drag.active: occDrag.drag.active
                        Drag.source: occCard
                        Drag.hotSpot.x: overlay._cell / 2
                        Drag.hotSpot.y: overlay._cell / 2

                        states: State {
                            when: occDrag.drag.active
                            ParentChange { target: occCard; parent: overlay.store ? overlay.store.widgetDragLayer : null }
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: Tokens.radiusSm
                            color: occDrag.drag.active ? Qt.rgba(1, 1, 1, 0.06) : "transparent"
                            border.width: occHover.hovered ? 1 : 0
                            border.color: AppPalette.accentBorder
                        }

                        Text {
                            visible: occCard.representationType === "value"
                            anchors.fill: parent
                            anchors.margins: Math.round(6 * overlay._k)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            minimumPixelSize: Math.max(6, Math.round(7 * overlay._k))
                            font.pixelSize: Math.round(overlay._cell * 0.6)
                            font.bold: true
                            elide: Text.ElideRight
                            text: occCard._value
                            color: AppPalette.textStrong
                        }

                        Item {
                            visible: occCard.representationType === "labelValueStacked"
                            anchors.fill: parent
                            anchors.margins: Math.round(6 * overlay._k)

                            Text {
                                id: stLabel
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: parent.height * 0.45
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                fontSizeMode: Text.Fit
                                minimumPixelSize: Math.max(6, Math.round(7 * overlay._k))
                                font.pixelSize: Math.round(overlay._cell * 0.42)
                                font.bold: true
                                elide: Text.ElideRight
                                text: occCard._label
                                color: AppPalette.textSecond
                            }
                            Text {
                                anchors.top: stLabel.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                minimumPixelSize: Math.max(6, Math.round(7 * overlay._k))
                                font.pixelSize: Math.round(overlay._cell * 0.55)
                                font.bold: true
                                elide: Text.ElideRight
                                text: occCard._value
                                color: AppPalette.textStrong
                            }
                        }

                        Item {
                            visible: occCard.representationType === "labelValueRow"
                            anchors.fill: parent
                            anchors.margins: Math.round(6 * overlay._k)

                            Text {
                                id: rowLabel
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: (parent.width - overlay._gap) / 2
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                fontSizeMode: Text.Fit
                                minimumPixelSize: Math.max(6, Math.round(7 * overlay._k))
                                font.pixelSize: Math.round(overlay._cell * 0.42)
                                font.bold: true
                                elide: Text.ElideRight
                                text: occCard._label
                                color: AppPalette.textSecond
                            }
                            Text {
                                anchors.left: rowLabel.right
                                anchors.leftMargin: overlay._gap
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                minimumPixelSize: Math.max(6, Math.round(7 * overlay._k))
                                font.pixelSize: Math.round(overlay._cell * 0.5)
                                font.bold: true
                                elide: Text.ElideRight
                                text: occCard._value
                                color: AppPalette.textStrong
                            }
                        }

                        HoverHandler { id: occHover }

                        KCircleIconButton {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: Math.round(3 * overlay._k)
                            width: Math.round(18 * overlay._k)
                            height: Math.round(18 * overlay._k)
                            rounded: true
                            glyph: "×"
                            glyphPixelSize: Math.round(12 * overlay._k)
                            glyphColor: AppPalette.textSecond
                            fillColor: AppPalette.controlRaised
                            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
                            borderColor: AppPalette.border
                            borderHoverColor: AppPalette.borderHover
                            visible: occHover.hovered
                            toolTipText: qsTr("Remove field")
                            z: 8
                            onClicked: if (overlay.store) overlay.store.widgetDraftRemove(occCard.fieldKey)
                        }

                        MouseArea {
                            id: occDrag
                            anchors.fill: parent
                            cursorShape: Qt.OpenHandCursor
                            drag.target: occCard
                            onReleased: if (overlay.store) overlay.store.widgetDraftCommitMoveOrRemove(occCard.fieldKey, occCard.representationType)
                            onClicked: if (overlay.store) overlay.store.widgetDraftCycle(occCard.fieldKey)
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
                    overlay.store.widgetDraftSetScale(rawScale)
                }
            }
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var def: null

    readonly property bool _isNodes: !!(def && def.kind === "usblNodes")
    readonly property bool _isStand: !!(def && def.kind === "stand")
    readonly property int _cols: (def && typeof def.cols === "number") ? def.cols : 1
    readonly property int _rows: (def && typeof def.rows === "number") ? def.rows : 1
    readonly property real _gap: Math.round(3 * AppPalette.scale)

    Rectangle {
        anchors.fill: parent
        radius: Tokens.radiusMd
        color: AppPalette.bgDeep
        border.width: 1
        border.color: AppPalette.border

        // A nodes panel has no grid to preview and no fixed row count to draw honestly, so the
        // thumbnail is a glyph for "a list": three full-width bars. Drawing three cells of a
        // grid instead would promise a shape the panel does not have.
        Column {
            visible: root._isNodes
            anchors.fill: parent
            anchors.margins: Tokens.spaceXs
            spacing: root._gap
            Repeater {
                model: 3
                delegate: Rectangle {
                    required property int index
                    width: parent.width
                    height: Math.max(2, (parent.height - 2 * root._gap) / 3)
                    radius: 2
                    color: AppPalette.accentBg
                    border.width: 1
                    border.color: AppPalette.accentBorder
                    // Fading down the list says "and however many more", which is the one thing
                    // a fixed-count thumbnail has to get across.
                    opacity: 1.0 - index * 0.28
                }
            }
        }

        // A stand panel has no grid and no row count either. What it always has is a command
        // row, so the thumbnail is that: one wide button and three round ones.
        Row {
            visible: root._isStand
            anchors.centerIn: parent
            spacing: root._gap
            readonly property real dot: Math.max(3, Math.min(parent.height * 0.34, parent.width * 0.12))

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.dot * 2.4; height: parent.dot
                radius: height / 2
                color: AppPalette.accentBg
                border.width: 1
                border.color: AppPalette.accentBorder
            }
            Repeater {
                model: 3
                delegate: Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.dot; height: parent.dot
                    radius: height / 2
                    color: AppPalette.rowRaised
                    border.width: 1
                    border.color: AppPalette.border
                }
            }
        }

        Item {
            id: area
            visible: !root._isNodes && !root._isStand
            anchors.fill: parent
            anchors.margins: Tokens.spaceXs

            readonly property real cell: Math.max(0, Math.min(
                root._cols > 0 ? (width - (root._cols - 1) * root._gap) / root._cols : width,
                root._rows > 0 ? (height - (root._rows - 1) * root._gap) / root._rows : height))
            readonly property real gridW: root._cols * cell + (root._cols - 1) * root._gap
            readonly property real gridH: root._rows * cell + (root._rows - 1) * root._gap
            readonly property real offX: (width - gridW) / 2
            readonly property real offY: (height - gridH) / 2

            Repeater {
                model: root._cols * root._rows
                delegate: Rectangle {
                    required property int index
                    readonly property int _r: Math.floor(index / root._cols)
                    readonly property int _c: index % root._cols
                    x: area.offX + _c * (area.cell + root._gap)
                    y: area.offY + _r * (area.cell + root._gap)
                    width: area.cell
                    height: area.cell
                    radius: 2
                    color: "transparent"
                    border.width: 1
                    border.color: AppPalette.border
                }
            }

            Repeater {
                model: root.def ? root.def.cells : []
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool _cellBig: modelData.big === true
                    readonly property int _sc: (modelData.rep === "labelValueRow" ? 2 : 1) * (_cellBig ? 2 : 1)
                    readonly property int _sr: _cellBig ? 2 : 1
                    x: area.offX + modelData.col * (area.cell + root._gap)
                    y: area.offY + modelData.row * (area.cell + root._gap)
                    width: _sc * area.cell + (_sc - 1) * root._gap
                    height: _sr * area.cell + (_sr - 1) * root._gap
                    radius: 2
                    color: AppPalette.accentBg
                    border.width: 1
                    border.color: AppPalette.accentBorder
                }
            }
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var def: null

    readonly property int _cols: def ? def.cols : 1
    readonly property int _rows: def ? def.rows : 1
    readonly property real _gap: Math.round(3 * AppPalette.scale)

    Rectangle {
        anchors.fill: parent
        radius: Tokens.radiusMd
        color: AppPalette.bgDeep
        border.width: 1
        border.color: AppPalette.border

        Item {
            id: area
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

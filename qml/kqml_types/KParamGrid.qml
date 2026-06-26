import QtQuick 2.15
import QtQuick.Layouts 1.15

// Responsive parameter grid: at most 2 equal-width columns. Each child should
// set Layout.fillWidth: true so it fills its column (half the row when two fit).
// Drops to a single column when two cells won't fit the width; a lone trailing
// child then occupies one column (half) in 2-column mode. Shared rule for every
// label + spinbox/combo parameter row across the UI.
GridLayout {
    property real minCellWidth: Math.round(150 * AppPalette.scale)

    width: parent ? parent.width : implicitWidth
    columnSpacing: Tokens.spaceMd
    rowSpacing: Tokens.spaceSm
    columns: (width >= minCellWidth * 2 + columnSpacing) ? 2 : 1
}

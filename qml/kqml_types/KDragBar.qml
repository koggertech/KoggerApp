import QtQuick 2.15

// Unified drag handle — capsule + grip dots. One finger-sized control used for
// pane-split resize bars, floating-popup headers, and the bottom-track palette.
// Visual only: the consumer owns the actual drag mechanism (MouseArea /
// DragHandler). Size comes from AppPalette so "one finger = one size" lives in
// a single place; narrow containers may shrink `barLength` (thickness stays).
Item {
    id: root

    // "horizontal" → bar runs left↔right (dots in a Row);
    // "vertical"   → bar runs top↕bottom (dots in a Column).
    property string orientation: "horizontal"
    readonly property bool _vertical: orientation === "vertical"

    property int barLength: Math.round(AppPalette.dragBarLengthPx * AppPalette.scale)
    property int barThickness: Math.round(AppPalette.dragBarThicknessPx * AppPalette.scale)
    property int dotCount: 3
    property bool showCapsule: true

    property color barColor: AppPalette.card
    property color borderColor: AppPalette.border
    property color dotColor: AppPalette.text

    readonly property int _dotSpacing: Math.max(2, Math.round(3 * AppPalette.scale))
    readonly property int _dotLong:  Math.max(2, Math.round(5 * AppPalette.scale))
    readonly property int _dotShort: Math.max(1, Math.round(2 * AppPalette.scale))

    implicitWidth:  _vertical ? barThickness : barLength
    implicitHeight: _vertical ? barLength : barThickness
    width: implicitWidth
    height: implicitHeight

    Rectangle {
        anchors.fill: parent
        visible: root.showCapsule
        radius: Math.min(width, height) / 2
        color: root.barColor
        border.width: 1
        border.color: root.borderColor
    }

    Grid {
        anchors.centerIn: parent
        rows:    root._vertical ? root.dotCount : 1
        columns: root._vertical ? 1 : root.dotCount
        rowSpacing: root._dotSpacing
        columnSpacing: root._dotSpacing

        Repeater {
            model: root.dotCount
            delegate: Rectangle {
                // Dots elongate across the bar's short axis (like the edge bars).
                width:  root._vertical ? root._dotLong : root._dotShort
                height: root._vertical ? root._dotShort : root._dotLong
                radius: Math.min(width, height) / 2
                color: root.dotColor
            }
        }
    }
}

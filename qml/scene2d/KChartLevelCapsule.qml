import QtQuick 2.15
import QtQuick.Layouts 1.15
import controls
import kqml_types 1.0

// Styled dual-range level slider shared by the echogram (Plot2D) and the Mosaic
// settings: a rounded capsule (theme card + border) with the stop value on top,
// ChartLevel in the middle, start value at the bottom. One source of truth so
// every level slider in the app looks identical and scales/themes the same way.
Rectangle {
    id: root

    // Range API — two-way to the inner ChartLevel.
    property alias startValue:  chartLevel.startValue
    property alias stopValue:   chartLevel.stopValue
    property alias from:        chartLevel.from
    property alias to:          chartLevel.to
    property alias heightCoeff: chartLevel.heightCoeff
    readonly property alias slider: chartLevel

    property int capsuleWidth: Math.round(40 * AppPalette.scale)

    implicitWidth: capsuleWidth
    implicitHeight: col.implicitHeight + Math.round(20 * AppPalette.scale)
    radius: width / 2
    color: AppPalette.card
    border.width: 1
    border.color: AppPalette.border

    ColumnLayout {
        id: col
        // Fill the capsule (vertical padding only) so the ChartLevel can stretch
        // to the capsule height — lets a call site grow it via Layout.fillHeight
        // (Mosaic = full height) while the echogram keeps its implicit height.
        anchors.fill: parent
        anchors.topMargin: Math.round(10 * AppPalette.scale)
        anchors.bottomMargin: Math.round(10 * AppPalette.scale)
        spacing: 2

        CText {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: chartLevel.stopValue
            small: true
        }

        ChartLevel {
            id: chartLevel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: root.capsuleWidth
            Layout.alignment: Qt.AlignHCenter
            widthSlider: root.capsuleWidth - Math.round(14 * AppPalette.scale)
        }

        CText {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: chartLevel.startValue
            small: true
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Item {
    id: row

    property string label: ""
    property string unit: ""
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    property real value: 0
    property real defaultValue: NaN
    property color textColor: AppPalette.accentText

    signal editBegan()
    signal liveChanged(real value)
    signal editEnded()

    function resetToDefault() {
        if (isNaN(defaultValue)) return
        editBegan()
        liveChanged(defaultValue)
        editEnded()
    }

    readonly property real _s: AppPalette.scale
    readonly property int labelW: Math.round(90 * _s)

    implicitHeight: Math.max(slider.implicitHeight, Tokens.controlHMd)
    height: implicitHeight

    Text {
        id: labelText
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: row.labelW
        text: row.label
        color: row.textColor
        font.pixelSize: Tokens.fontSm
        elide: Text.ElideRight

        KTapArea {
            cursorPointing: !isNaN(row.defaultValue)
            onDoubleTapped: row.resetToDefault()
        }
    }

    KSlider {
        id: slider
        anchors.left: labelText.right
        anchors.leftMargin: Tokens.spaceMd
        anchors.right: valueText.left
        anchors.rightMargin: Tokens.spaceMd
        anchors.verticalCenter: parent.verticalCenter
        from: row.from
        to: row.to
        stepSize: row.stepSize
        value: row.value
        showValueTip: false
        trackFillColor: Qt.rgba(1, 1, 1, 0.85)
        trackOffColor: Qt.rgba(0, 0, 0, 0.28)
        trackOffBorderColor: Qt.rgba(0, 0, 0, 0.35)
        knobBorderColor: Qt.rgba(0, 0, 0, 0.35)
        onValueModified: function(v) {
            if (slider.pressed) { row.liveChanged(v); return }
            row.editBegan()
            row.liveChanged(v)
            row.editEnded()
        }
        onPressedChanged: {
            if (pressed) row.editBegan()
            else row.editEnded()
        }
    }

    Text {
        id: valueText
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Math.round(58 * row._s)
        horizontalAlignment: Text.AlignRight
        text: Number(slider.pressed ? slider.value : row.value).toFixed(row.decimals) + (row.unit.length ? " " + row.unit : "")
        color: row.textColor
        font.pixelSize: Tokens.fontBase
        font.bold: true

        KTapArea {
            cursorPointing: !isNaN(row.defaultValue)
            onDoubleTapped: row.resetToDefault()
        }
    }
}

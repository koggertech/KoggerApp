import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    id: control

    property color trackOffColor: AppPalette.trackOff
    property color trackOffBorderColor: AppPalette.trackOffBorder
    property color trackFillColor: AppPalette.accentBar
    property color knobColor: AppPalette.knob
    property color knobBorderColor: AppPalette.borderHover
    property color knobBorderActiveColor: AppPalette.accentBorder

    property int trackHeight: 6
    property int knobSize: 18

    property string toolTipText: ""
    property bool showValueTip: true
    property int valueDecimals: 0
    property real valueDivisor: 1.0
    property string valueSuffix: ""

    signal valueModified(real val)

    implicitWidth: 200
    implicitHeight: Math.max(knobSize, 30)
    horizontalPadding: knobSize / 2
    verticalPadding: 0
    snapMode: Slider.SnapAlways
    opacity: enabled ? 1.0 : 0.55

    onMoved: control.valueModified(value)

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.availableWidth
        height: control.trackHeight
        radius: height / 2
        color: control.trackOffColor
        border.width: 1
        border.color: control.trackOffBorderColor

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: control.trackFillColor

            Behavior on width {
                enabled: !control.pressed
                NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth) - width / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.knobSize
        height: control.knobSize
        radius: width / 2
        color: control.knobColor
        border.width: control.pressed || control.hovered ? 2 : 1
        border.color: control.pressed || control.hovered
                      ? control.knobBorderActiveColor
                      : control.knobBorderColor

        Behavior on border.color {
            ColorAnimation { duration: 100 }
        }

        scale: control.pressed ? 1.1 : 1.0
        Behavior on scale {
            NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
        }
    }

    KToolTip {
        targetItem: control
        shown: control.showValueTip && (control.pressed || control.hovered) && control.enabled
        text: control.toolTipText.length > 0
              ? control.toolTipText
              : (control.value / control.valueDivisor).toFixed(control.valueDecimals) + control.valueSuffix
    }
}

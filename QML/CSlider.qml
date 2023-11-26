import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Slider {
    id: slider
    from: 0
    value: 0
    to: 0
    horizontalPadding: 0
    snapMode: Slider.SnapAlways

    property real backHandleX: slider.horizontal ? slider.leftPadding + slider.visualPosition * (slider.width - slider.leftPadding - slider.rightPadding - handleControl.width): slider.leftPadding
    property real backHandleY: slider.horizontal ? slider.topPadding + slider.availableHeight / 2 - handleControl.height / 2 : -slider.topPadding / 2 + slider.visualPosition * (slider.height) - handleControl.height / 2

    handle: Rectangle {
        id: handleControl
        x: backHandleX
        y: backHandleY
        width: 10
        height: slider.height
        color: slider.pressed ? theme.textSolidColor : theme.textColor
    }

    background: Rectangle {
        radius: 1
        color: slider.pressed ? theme.controlSolidBackColor : theme.controlBackColor
        border.color: slider.pressed ? theme.controlSolidBorderColor : theme.controlBorderColor
        border.width: 0
    }
}

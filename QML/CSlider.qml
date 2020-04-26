import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Slider {
    id: slider
    from: 0
    value: 0
    to: 0
    horizontalPadding: 0
    snapMode: Slider.SnapAlways

    handle: Item {
        Rectangle {
            id: backHandle
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth) - width/2
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 28
            implicitHeight: 12
            color: "#70C0F0"
        }

        Rectangle {
            id: leftPipe
            x: backHandle.x - 4
            y: backHandle.y
            width: 2
            height: backHandle.implicitHeight
            color: "#7090b0"
        }

        Rectangle {
            id: rightPipe
            x: backHandle.x + backHandle.width + 2
            y: backHandle.y
            width: 2
            height: backHandle.implicitHeight
            color: "#7090b0"
        }
    }

    background: Item {
        Rectangle {
            x: slider.leftPadding - backHandle.width
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: leftPipe.x - x
            height: 2
            color: "#7090b0"
        }

        Rectangle {
            x: rightPipe.x
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.width - slider.leftPadding - x + backHandle.width
            height: 2
            color: "#7090b0"
        }
    }
}

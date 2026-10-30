import QtQuick 2.15
import kqml_types 1.0

Item {
    id: bar

    property real value: 0

    width: Math.round(84 * AppPalette.appScale)
    height: Math.round(6 * AppPalette.appScale)

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: AppPalette.trackOff
    }
    Rectangle {
        x: bar.width / 2 + Math.min(0, bar.value) * bar.width / 2
        width: Math.abs(bar.value) * bar.width / 2
        height: bar.height
        radius: height / 2
        color: AppPalette.accentBar
    }
    Rectangle {
        x: bar.width / 2 - width / 2
        width: Math.max(1, Math.round(AppPalette.appScale))
        height: bar.height
        color: AppPalette.textMuted
    }
}

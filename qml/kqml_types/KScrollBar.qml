import QtQuick 2.15
import QtQuick.Controls 2.15

ScrollBar {
    id: control

    required property Flickable flickable
    property int autoHideMs: 1500
    property int thumbWidth: Math.round(12 * AppPalette.scale)

    orientation: Qt.Vertical
    policy: flickable && flickable.contentHeight > flickable.height + 1 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    width: thumbWidth
    implicitWidth: thumbWidth
    z: 4

    size: flickable && flickable.contentHeight > 0 ? Math.min(1, flickable.height / flickable.contentHeight) : 0
    position: flickable && flickable.contentHeight > 0 ? flickable.contentY / flickable.contentHeight : 0
    stepSize: 0.04
    active: (flickable && flickable.movingVertically) || pressed || hovered

    property bool _shown: false

    Timer {
        id: hideTimer
        interval: control.autoHideMs
        onTriggered: control._shown = false
    }

    onActiveChanged: {
        if (active) {
            hideTimer.stop()
            _shown = true
        } else {
            hideTimer.restart()
        }
    }
    onVisibleChanged: if (visible) { _shown = true; hideTimer.restart() }

    Connections {
        target: control.flickable
        function onContentYChanged() {
            control._shown = true
            if (!control.active)
                hideTimer.restart()
        }
    }

    onPositionChanged: {
        if (pressed && flickable)
            flickable.contentY = position * flickable.contentHeight
    }

    contentItem: Rectangle {
        implicitWidth: control.width
        radius: width / 2
        color: control.pressed ? AppPalette.text : (control.hovered ? AppPalette.textSecond : AppPalette.textMuted)
        opacity: !control._shown ? 0.0 : (control.pressed ? 0.85 : (control.hovered ? 0.65 : 0.45))
        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    }

    background: Item {}
}

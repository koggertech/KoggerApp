import QtQuick 2.15
import QtQuick.Controls 2.15

ScrollBar {
    id: control

    required property Flickable flickable
    property int autoHideMs: 1500
    property int thumbWidth: Math.round(12 * AppPalette.scale)

    readonly property real _content: !flickable ? 0 : (horizontal ? flickable.contentWidth : flickable.contentHeight)
    readonly property real _view: !flickable ? 0 : (horizontal ? flickable.width : flickable.height)
    readonly property real _offset: !flickable ? 0 : (horizontal ? flickable.contentX : flickable.contentY)

    orientation: Qt.Vertical
    policy: _content > _view + 1 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    width: horizontal ? implicitWidth : thumbWidth
    height: horizontal ? thumbWidth : implicitHeight
    implicitWidth: horizontal ? 0 : thumbWidth
    implicitHeight: horizontal ? thumbWidth : 0
    z: 4

    size: _content > 0 ? Math.min(1, _view / _content) : 0
    position: _content > 0 ? _offset / _content : 0
    stepSize: 0.04
    active: (flickable && (horizontal ? flickable.movingHorizontally : flickable.movingVertically)) || pressed || hovered

    property bool _shown: false

    function _poke() {
        _shown = true
        if (!active)
            hideTimer.restart()
    }

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
        function onContentYChanged() { if (!control.horizontal) control._poke() }
        function onContentXChanged() { if (control.horizontal) control._poke() }
    }

    onPositionChanged: {
        if (!pressed || !flickable)
            return
        if (horizontal)
            flickable.contentX = position * _content
        else
            flickable.contentY = position * _content
    }

    contentItem: Rectangle {
        implicitWidth: control.horizontal ? 0 : control.width
        implicitHeight: control.horizontal ? control.height : 0
        radius: (control.horizontal ? height : width) / 2
        color: control.pressed ? AppPalette.text : (control.hovered ? AppPalette.textSecond : AppPalette.textMuted)
        opacity: !control._shown ? 0.0 : (control.pressed ? 0.85 : (control.hovered ? 0.65 : 0.45))
        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    }

    background: Item {}
}

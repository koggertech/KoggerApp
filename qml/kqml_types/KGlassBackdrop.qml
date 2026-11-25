import QtQuick
import QtQuick.Effects

Item {
    id: glass

    property Item source: null
    property Item originSpace: parent
    property point origin: Qt.point(x, y)
    property Item maskItem: null
    property real radius: 0
    property int blurPx: 40

    readonly property int pad: blurPx
    readonly property point _spaceOffset: {
        var src = glass.source
        var space = glass.originSpace
        if (!src || !src.parent || !space || space === src.parent)
            return Qt.point(0, 0)
        return space.mapToItem(src.parent, 0, 0)
    }
    readonly property rect inSource: source
                                     ? Qt.rect(_spaceOffset.x + origin.x - source.x,
                                               _spaceOffset.y + origin.y - source.y,
                                               width, height)
                                     : Qt.rect(0, 0, 0, 0)
    readonly property bool overSource: !!source && source.visible
                                       && inSource.x < source.width && inSource.x + inSource.width > 0
                                       && inSource.y < source.height && inSource.y + inSource.height > 0
    readonly property bool active: visible && overSource

    readonly property Item _rectShape: Item {
        width: capture.width
        height: capture.height

        Rectangle {
            x: glass.pad + 1
            y: glass.pad + 1
            width: Math.max(0, glass.width - 2)
            height: Math.max(0, glass.height - 2)
            radius: Math.max(0, glass.radius - 1)
        }
    }

    function tint(c: color, a: real): color {
        return active ? Qt.rgba(c.r, c.g, c.b, c.a * a) : c
    }

    ShaderEffectSource {
        id: capture
        x: -glass.pad
        y: -glass.pad
        width: glass.width + glass.pad * 2
        height: glass.height + glass.pad * 2
        sourceItem: glass.active ? glass.source : null
        sourceRect: Qt.rect(glass.inSource.x - glass.pad, glass.inSource.y - glass.pad, width, height)
        live: glass.active
        visible: false
    }

    ShaderEffectSource {
        id: itemMask
        width: capture.width
        height: capture.height
        sourceItem: glass.active ? glass.maskItem : null
        sourceRect: Qt.rect(-glass.pad, -glass.pad, width, height)
        live: glass.active && glass.maskItem !== null
        visible: false
    }

    ShaderEffectSource {
        id: rectMask
        width: capture.width
        height: capture.height
        hideSource: true
        live: glass.active && glass.maskItem === null
        visible: false
        sourceItem: glass.active && glass.maskItem === null ? glass._rectShape : null
    }

    MultiEffect {
        x: capture.x
        y: capture.y
        width: capture.width
        height: capture.height
        visible: glass.overSource
        source: capture
        autoPaddingEnabled: false
        blurEnabled: true
        blur: 1.0
        blurMax: glass.pad
        maskEnabled: true
        maskSource: glass.maskItem ? itemMask : rectMask
        maskThresholdMin: 0.3333
        maskSpreadAtMin: 0.5
    }
}

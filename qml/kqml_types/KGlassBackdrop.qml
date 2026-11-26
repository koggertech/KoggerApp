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
    property real captureScale: 1.0

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
    readonly property rect captureRect: {
        if (!source)
            return Qt.rect(0, 0, 0, 0)
        var r = inSource
        var x0 = r.x - pad
        var y0 = r.y - pad
        var x1 = r.x + r.width + pad
        var y1 = r.y + r.height + pad
        if (r.x >= 0)
            x0 = Math.max(x0, 0)
        if (r.y >= 0)
            y0 = Math.max(y0, 0)
        if (r.x + r.width <= source.width)
            x1 = Math.min(x1, source.width)
        if (r.y + r.height <= source.height)
            y1 = Math.min(y1, source.height)
        return Qt.rect(x0, y0, Math.max(1, x1 - x0), Math.max(1, y1 - y0))
    }

    readonly property Item _rectShape: Item {
        width: capture.width
        height: capture.height

        Rectangle {
            x: 1 - capture.x
            y: 1 - capture.y
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
        x: glass.captureRect.x - glass.inSource.x
        y: glass.captureRect.y - glass.inSource.y
        width: glass.captureRect.width
        height: glass.captureRect.height
        sourceItem: glass.active ? glass.source : null
        sourceRect: glass.captureRect
        textureSize: glass.captureScale < 1
                     ? Qt.size(Math.max(1, Math.round(width * glass.captureScale)),
                               Math.max(1, Math.round(height * glass.captureScale)))
                     : Qt.size(0, 0)
        live: glass.active
        visible: false
    }

    ShaderEffectSource {
        id: itemMask
        width: capture.width
        height: capture.height
        sourceItem: glass.active ? glass.maskItem : null
        sourceRect: Qt.rect(capture.x, capture.y, width, height)
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

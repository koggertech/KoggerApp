import QtQuick
import kqml_types 1.0

Item {
    id: pad

    property var view: null
    readonly property real designW: 560
    readonly property real designH: 380
    readonly property real fit: Math.max(0, Math.min(width / designW, height / designH))
    readonly property real triggerZone: view ? view.triggerZone : 0
    readonly property real stickZone: view ? view.stickZone : 0
    default property alias content: canvas.data

    function isDown(bit) {
        return view ? view.isDown(bit) : false
    }

    function isAvailable(bit) {
        return view ? view.isAvailable(bit) : true
    }

    function axis(index) {
        return view ? view.axisValue(index) : 0
    }

    function trigger(input) {
        return view ? view.halfRaw(input) : 0
    }

    function face(index) {
        return view && view.faceLabels.length > index ? view.faceLabels[index] : ""
    }

    Item {
        id: canvas
        x: (pad.width - pad.designW * pad.fit) / 2
        y: (pad.height - pad.designH * pad.fit) / 2
        width: pad.designW
        height: pad.designH
        transform: Scale { xScale: pad.fit; yScale: pad.fit }
    }
}

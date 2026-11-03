import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

Item {
    id: key

    property Item pad: null
    property int bit: 0
    readonly property bool down: pad ? pad.isDown(bit) : false
    readonly property bool available: pad ? pad.isAvailable(bit) : true

    opacity: available ? 1.0 : 0.2

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: key.down ? AppPalette.accent : AppPalette.controlRaised
            strokeColor: key.down ? Qt.lighter(AppPalette.accent, 1.3) : AppPalette.borderHover
            strokeWidth: 2
            joinStyle: ShapePath.RoundJoin
            startX: 3; startY: 3
            PathLine { x: key.width - 3; y: 3 }
            PathLine { x: key.width - 3; y: key.height - key.width / 2 }
            PathLine { x: key.width / 2; y: key.height - 3 }
            PathLine { x: 3; y: key.height - key.width / 2 }
            PathLine { x: 3; y: 3 }
        }
    }
}

import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

Item {
    id: root

    property url source: ""
    property color color: AppPalette.textSecond
    property string toolTipText: ""

    implicitWidth: Tokens.iconSm
    implicitHeight: Tokens.iconSm

    Image {
        id: image
        anchors.fill: parent
        source: root.source
        sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
        sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
        layer.enabled: true
        layer.smooth: true
        layer.textureSize: Qt.size(image.sourceSize.width, image.sourceSize.height)
    }

    ColorOverlay {
        anchors.fill: image
        source: image
        color: root.color
        smooth: true
        cached: true
    }

    HoverHandler {
        id: hover
        enabled: root.toolTipText.length > 0
    }

    KLazyToolTip {
        text: root.toolTipText
        targetItem: root
        shown: hover.hovered
    }
}

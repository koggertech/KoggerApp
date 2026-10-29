import QtQuick
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

Item {
    id: root

    property string family: ""
    property color color: AppPalette.text
    readonly property string source: family === "xbox" ? "qrc:/icons/ui/gamepad-xbox.svg"
                                   : family === "ps4" ? "qrc:/icons/ui/gamepad-ps4.svg"
                                   : family === "ps5" ? "qrc:/icons/ui/gamepad-ps5.svg"
                                   : family === "steam" ? "qrc:/icons/ui/gamepad-steam.svg"
                                   : "qrc:/icons/ui/gamepad-generic.svg"

    implicitWidth: Math.round(24 * AppPalette.scale)
    implicitHeight: implicitWidth

    Image {
        id: image
        anchors.fill: parent
        source: root.source
        sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
        sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: image
        source: image
        color: root.color
        cached: true
    }
}

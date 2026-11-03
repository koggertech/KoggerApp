import QtQuick
import QtQuick.Controls
import kqml_types 1.0

Popup {
    id: gallery

    readonly property var families: [
        { family: "xbox",    name: "Xbox",        faces: ["A", "B", "X", "Y"] },
        { family: "ps4",     name: "DualShock 4", faces: ["cross", "circle", "square", "triangle"] },
        { family: "ps5",     name: "DualSense",   faces: ["cross", "circle", "square", "triangle"] },
        { family: "steam",   name: "Steam",       faces: ["A", "B", "X", "Y"] },
        { family: "generic", name: qsTr("Other gamepad"), faces: [] }
    ]
    readonly property int columns: 3

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: Math.round(14 * AppPalette.scale)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(Math.round(1100 * AppPalette.scale), parent ? parent.width * 0.95 : 1100)
    height: Math.min(Math.round(640 * AppPalette.scale), parent ? parent.height * 0.95 : 640)

    background: Rectangle {
        color: AppPalette.bgDeep
        border.color: AppPalette.border
        border.width: 1
        radius: 10
    }

    Item {
        anchors.fill: parent

        Text {
            id: title
            text: qsTr("Gamepad drawings")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontXl
            font.bold: true
        }

        KCircleIconButton {
            anchors.right: parent.right
            anchors.verticalCenter: title.verticalCenter
            width: Tokens.controlHMd
            height: width
            glyph: "✕"
            glyphPixelSize: Math.round(12 * AppPalette.scale)
            fillColor: AppPalette.card
            onClicked: gallery.close()
        }

        Grid {
            id: grid
            readonly property real cellW: (width - (gallery.columns - 1) * columnSpacing) / gallery.columns
            readonly property int rowCount: Math.ceil(gallery.families.length / gallery.columns)
            readonly property real cellH: (height - (rowCount - 1) * rowSpacing) / rowCount

            anchors.top: title.bottom
            anchors.topMargin: Tokens.spaceLg
            anchors.bottom: parent.bottom
            width: parent.width
            columns: gallery.columns
            columnSpacing: Tokens.spaceLg
            rowSpacing: Tokens.spaceLg

            Repeater {
                model: gallery.families
                delegate: Item {
                    id: tile
                    required property var modelData
                    readonly property string family: modelData.family
                    readonly property var faceLabels: modelData.faces
                    readonly property real triggerZone: 0
                    readonly property real stickZone: 0

                    function isDown(bit) { return false }
                    function isAvailable(bit) { return true }
                    function axisValue(axis) { return 0 }
                    function halfRaw(input) { return 0 }

                    width: grid.cellW
                    height: grid.cellH

                    Text {
                        id: tileName
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: tile.modelData.name + "  ·  " + tile.family
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm
                    }

                    Loader {
                        anchors.top: parent.top
                        anchors.bottom: tileName.top
                        anchors.bottomMargin: Tokens.spaceXs
                        width: parent.width
                        source: tile.family === "ps4" || tile.family === "ps5" ? "GamepadDualShock.qml"
                              : tile.family === "steam" ? "GamepadSteam.qml"
                              : "GamepadXbox.qml"
                        onLoaded: item.view = tile
                    }
                }
            }
        }
    }
}

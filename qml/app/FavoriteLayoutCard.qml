import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    property var snapshot: null
    property var popupLinks: []
    property int favoriteIndex: 0
    property bool selected: false
    property bool showText: true
    property int previewWidth: 84
    property int previewHeight: 64
    property int contentMargin: 6
    property int contentSpacing: 8
    property string titlePrefix: "Favorite "
    property int previewRedrawDebounceMs: 48

    readonly property bool hovered: hitArea.containsMouse

    signal clicked()

    implicitWidth: showText ? 230 : previewWidth + contentMargin * 2
    implicitHeight: showText ? 88 : 76
    radius: 8
    color: hovered ? AppPalette.bg : AppPalette.card
    border.width: selected ? 2 : 1
    border.color: selected ? "#FACC15" : (hovered ? AppPalette.borderHover : AppPalette.border)

    function leafCount(node) {
        if (!node)
            return 0
        if (node.type === "leaf")
            return 1
        return leafCount(node.first) + leafCount(node.second)
    }

    LayoutSnapshotPreview {
        id: previewItem
        width: root.previewWidth
        height: root.previewHeight
        x: root.showText ? root.contentMargin : Math.round((root.width - width) / 2)
        anchors.verticalCenter: parent.verticalCenter
        layoutSnapshot: root.snapshot
        popupLinks: root.popupLinks
        redrawDebounceMs: root.previewRedrawDebounceMs
    }

    Column {
        visible: root.showText
        x: previewItem.x + previewItem.width + root.contentSpacing
        width: Math.max(0, root.width - x - root.contentMargin)
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Text {
            text: root.titlePrefix + (root.favoriteIndex + 1)
            color: root.selected ? "#FDE68A" : AppPalette.text
            font.pixelSize: 14
            font.bold: true
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            text: root.selected
                  ? (root.leafCount(root.snapshot) + " panes • active")
                  : (root.leafCount(root.snapshot) + " panes")
            color: AppPalette.textMuted
            font.pixelSize: 12
            elide: Text.ElideRight
            width: parent.width
        }
    }

    MouseArea {
        id: hitArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Rectangle {
        visible: root.selected
        width: 24
        height: 24
        radius: 12
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.rightMargin: root.showText ? 36 : 6
        color: AppPalette.bg
        border.width: 1
        border.color: "#FACC15"

        Canvas {
            anchors.centerIn: parent
            width: 14
            height: 14
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#FDE68A"
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(width * 0.16, height * 0.55)
                ctx.lineTo(width * 0.42, height * 0.8)
                ctx.lineTo(width * 0.86, height * 0.24)
                ctx.stroke()
            }
        }
    }
}

import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    property var snapshot: null
    property var popupLinks: []
    property int favoriteIndex: 0
    property bool selected: false
    property bool showText: true
    property int previewWidth:    Math.round(84 * AppPalette.scale)
    property int previewHeight:   Math.round(64 * AppPalette.scale)
    property int contentMargin:   Tokens.spaceSm
    property int contentSpacing:  Tokens.spaceMd
    property int previewRedrawDebounceMs: 48

    // External "look here" pulse — bumped via flashToken when highlighted.
    property bool highlighted: false
    property int flashToken: 0
    property color highlightBorderColor: AppPalette.accentBorder

    readonly property bool hovered: hitArea.containsMouse

    signal clicked()

    implicitWidth: showText ? Math.round(230 * AppPalette.scale)
                            : previewWidth + contentMargin * 2
    implicitHeight: showText ? Math.round(88 * AppPalette.scale)
                             : Math.round(76 * AppPalette.scale)
    radius: Tokens.radiusLg
    color: hovered ? AppPalette.bg : AppPalette.card
    border.width: selected ? Math.max(2, Math.round(2 * AppPalette.scale)) : 1
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
        spacing: Tokens.spaceXs

        Text {
            text: qsTr("Favorite %1").arg(root.favoriteIndex + 1)
            color: root.selected ? "#FDE68A" : AppPalette.text
            font.pixelSize: Tokens.fontBase
            font.bold: true
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            text: root.selected
                  ? qsTr("%1 panes (active)").arg(root.leafCount(root.snapshot))
                  : qsTr("%1 panes").arg(root.leafCount(root.snapshot))
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
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

    // Pulse overlay — fires when flashToken changes while highlighted.
    Rectangle {
        id: highlightOverlay
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.width: Math.max(2, Math.round(2 * AppPalette.scale))
        border.color: root.highlightBorderColor
        opacity: 0
        visible: root.highlighted
        z: 10
    }

    SequentialAnimation {
        id: highlightPulse
        running: false
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.95; duration: 90;  easing.type: Easing.OutCubic }
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.30; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.0;  duration: 280; easing.type: Easing.OutCubic }
    }

    onFlashTokenChanged: {
        if (highlighted)
            highlightPulse.restart()
    }

    onHighlightedChanged: {
        if (!highlighted)
            highlightOverlay.opacity = 0.0
    }

    // Newly-created delegates miss the flashToken change that preceded their
    // instantiation (initial binding read silently). Kick the pulse on init.
    Component.onCompleted: {
        if (highlighted)
            highlightPulse.restart()
    }

    // Selected-state ✓ indicator. Settings-panel mode (showText) reserves
    // room on the right for the external "remove" X button and sits on the
    // same vertical line as that X button. Inline mode (HotActions) shows
    // a smaller corner badge that fits inside the pill button.
    Rectangle {
        readonly property int _size: root.showText
                                     ? Math.round(24 * AppPalette.scale)
                                     : Math.round(10 * AppPalette.scale)

        visible: root.selected
        width: _size
        height: _size
        radius: _size / 2
        anchors.right: parent.right
        anchors.rightMargin: root.showText
                             ? Tokens.iconLg + 2 * Tokens.spaceSm
                             : Tokens.spaceXxs
        // Settings: vertically centered (paired with X-button); inline: top-right corner.
        anchors.verticalCenter: root.showText ? parent.verticalCenter : undefined
        anchors.top: root.showText ? undefined : parent.top
        anchors.topMargin: root.showText ? 0 : Tokens.spaceXxs
        color: AppPalette.bg
        border.width: 1
        border.color: "#FACC15"

        Canvas {
            anchors.centerIn: parent
            width:  Math.round(parent._size * 0.58)
            height: width
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#FDE68A"
                ctx.lineWidth = Math.max(1, Math.round(width * 0.14))
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

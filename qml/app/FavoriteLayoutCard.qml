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

    property bool extraHovered: false
    readonly property bool hovered: hitArea.containsMouse || extraHovered

    readonly property string layoutCaption: qsTr("Layout %1").arg(root.favoriteIndex + 1)

    signal clicked()

    implicitWidth: showText ? Math.round(230 * AppPalette.scale)
                            : previewWidth + contentMargin * 2
    implicitHeight: showText ? Math.round(88 * AppPalette.scale)
                             : Math.round(76 * AppPalette.scale)
    radius: Tokens.radiusLg
    color: hovered ? AppPalette.cardHover : AppPalette.card
    border.width: selected ? Math.max(2, Math.round(2 * AppPalette.scale)) : Tokens.cardBorderWidth
    border.color: selected ? "#FACC15" : (hovered ? AppPalette.borderHover : AppPalette.border)

    activeFocusOnTab: true
    Keys.onReturnPressed: root.clicked()
    Keys.onEnterPressed:  root.clicked()
    Keys.onSpacePressed:  root.clicked()

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
            text: root.layoutCaption
            color: root.selected ? "#FDE68A" : AppPalette.text
            font.pixelSize: Tokens.fontLg
            font.bold: true
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            visible: root.selected
            text: qsTr("Active")
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
        onPressed: focusRing.suppress()
        onClicked: { root.forceActiveFocus(); root.clicked() }
    }

    KFocusRing { id: focusRing; z: 12 }

    KToolTip {
        text: root.showText ? "" : root.layoutCaption
        shown: root.hovered
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

}

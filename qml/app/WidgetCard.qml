import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    property var def: null
    property string title: ""
    property bool showText: true
    property bool selectionMode: false
    property bool selected: false
    property bool extraHovered: false
    property int previewWidth: Math.round(84 * AppPalette.scale)
    property int previewHeight: Math.round(64 * AppPalette.scale)
    property int contentMargin: Tokens.spaceSm
    property int contentSpacing: Tokens.spaceMd

    readonly property bool hovered: hitArea.containsMouse || extraHovered

    readonly property string kindCaption: !def ? ""
        : def.kind === "usblNodes" ? qsTr("USBL")
        : def.kind === "servo"     ? qsTr("Servo control")
        : def.kind === "stand"     ? qsTr("Stand control")
                                   : (def.cols + "×" + def.rows)

    signal clicked()
    signal toggled(bool value)

    function _activate() {
        if (selectionMode)
            root.toggled(!root.selected)
        else
            root.clicked()
    }

    implicitWidth: showText ? Math.round(230 * AppPalette.scale) : previewWidth + contentMargin * 2
    implicitHeight: showText ? Math.round(88 * AppPalette.scale) : previewHeight + contentMargin * 2
    radius: Tokens.radiusLg
    color: hovered ? AppPalette.cardHover : AppPalette.card
    border.width: selected ? Math.max(2, Math.round(2 * AppPalette.scale)) : Tokens.cardBorderWidth
    border.color: selected ? "#FACC15" : (hovered ? AppPalette.borderHover : AppPalette.border)

    activeFocusOnTab: true
    Keys.onReturnPressed: root._activate()
    Keys.onEnterPressed:  root._activate()
    Keys.onSpacePressed:  root._activate()

    WidgetGridPreview {
        id: preview
        width: root.previewWidth
        height: root.previewHeight
        x: root.showText ? root.contentMargin : Math.round((root.width - width) / 2)
        anchors.verticalCenter: parent.verticalCenter
        def: root.def
    }

    Column {
        visible: root.showText
        x: preview.x + preview.width + root.contentSpacing
        width: Math.max(0, root.width - x - root.contentMargin)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Tokens.spaceXs

        Text {
            text: root.title
            color: root.selected ? "#FDE68A" : AppPalette.text
            font.pixelSize: Tokens.fontLg
            font.bold: true
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            // The subtitle is the panel's SHAPE, and a nodes panel does not have one to state —
            // its row count is the plan's and changes while you are looking at it. So it names
            // the kind instead of printing a number that would be wrong by the next fix.
            text: root.kindCaption
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
        onClicked: { root.forceActiveFocus(); root._activate() }
    }

    KToolTip {
        text: root.showText ? "" : (root.title.length > 0 ? root.title : root.kindCaption)
        shown: root.hovered
    }
}

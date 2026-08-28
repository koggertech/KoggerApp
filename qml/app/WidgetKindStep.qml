import QtQuick 2.15
import kqml_types 1.0

// Step 0 of the panel wizard: which KIND of panel. Three, and they are genuinely different
// objects rather than presets of one -- a field grid has a size and cells to lay out, an
// acoustic-nodes panel has neither because its rows come from the USBL plan at runtime, and a
// stand panel has no cells either but carries a configuration and takes input.
//
// It is the first step and not a toggle inside the others because the two wizards share no
// page after this one. Editing an existing panel skips it: the kind is not something you
// change, you make the other one.
Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Choose what this panel shows.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    component KindCard: Rectangle {
        id: card
        property string title: ""
        property string subtitle: ""
        property string kind: "grid"
        readonly property bool hovered: cardMouse.containsMouse

        width: parent ? parent.width : 0
        implicitHeight: cardCol.implicitHeight + 2 * Tokens.spaceLg
        radius: Tokens.radiusLg
        color: hovered ? AppPalette.cardHover : AppPalette.card
        border.width: hovered ? Math.max(1, Math.round(1 * AppPalette.scale)) : Tokens.cardBorderWidth
        border.color: hovered ? AppPalette.borderHover : AppPalette.border

        Column {
            id: cardCol
            x: Tokens.spaceLg; y: Tokens.spaceLg
            width: parent.width - 2 * Tokens.spaceLg
            spacing: Tokens.spaceXs

            Text {
                width: parent.width
                text: card.title
                color: AppPalette.text
                font.pixelSize: Tokens.fontLg
                font.bold: true
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: card.subtitle
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }
        }

        MouseArea {
            id: cardMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: if (step.store) step.store.widgetDraftSetKind(card.kind)
        }
    }

    KindCard {
        kind: "grid"
        title: qsTr("Field grid")
        subtitle: qsTr("A grid of cells you fill with values — depth, speed, coordinates.")
    }

    KindCard {
        kind: "usblNodes"
        title: qsTr("Acoustic nodes")
        subtitle: qsTr("One row per node in the USBL plan, with its range, SNR and state. "
                     + "The panel grows and shrinks with the plan.")
    }

    KindCard {
        // Hidden outright, not disabled: an operator with no stand should not learn that the
        // kind exists, and a greyed card is an invitation to ask why. Hidden again once one
        // exists -- the app drives one stand, and two panels would each hold a scan for it.
        visible: !!(step.store && step.store.standAvailable && !step.store.hasStandPanel)
        kind: "stand"
        title: qsTr("Stand")
        subtitle: qsTr("Configure and run a calibration scan, and see what the stand was last "
                     + "told. The only place a stand is driven from.")
    }
}

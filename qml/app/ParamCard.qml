import QtQuick 2.15
import kqml_types 1.0

// Parameter row card — matches KSwitch's full-width pattern.
//
//   [ label                  (interactive area)              [TOGGLE] ]
//                                  [ optional spinbox slot ]
//
// Click anywhere on the card (except the spinbox area) flips the toggle.
// Hover highlights the whole card. The default property is a content slot
// sized by `slotWidth` — drop a KSpinBox (or any control) inside it.
//
// Promoted from an inline component in AppSettingsPage so EchogramSettingsPanel
// can reuse it. References only AppPalette/Tokens singletons.
Rectangle {
    id: pcard

    property string label: ""
    property bool checked: false
    property int slotWidth: 0
    signal toggled(bool val)

    default property alias contentData: pcardSlot.data

    activeFocusOnTab: true
    Keys.onReturnPressed: pcard._flip()
    Keys.onEnterPressed:  pcard._flip()
    Keys.onSpacePressed:  pcard._flip()

    readonly property int _knobMargin: Math.max(2, Math.round(2 * AppPalette.scale))
    readonly property bool _hovered: cardMouseLeft.containsMouse || cardMouseRight.containsMouse

    width: parent ? parent.width : implicitWidth
    height: Math.round(38 * AppPalette.scale)
    radius: Tokens.radiusLg
    color: pcard._hovered ? AppPalette.bgHover : AppPalette.bg
    border.width: 1
    border.color: pcard._hovered ? AppPalette.borderHover : AppPalette.border

    Behavior on color       { ColorAnimation { duration: 110 } }
    Behavior on border.color { ColorAnimation { duration: 110 } }

    function _flip() {
        pcard.checked = !pcard.checked
        pcard.toggled(pcard.checked)
    }

    // ── Click-catchers: two MouseAreas flanking pcardSlot ──────────────
    // Splitting around the slot prevents stray clicks in the slot's
    // un-widget-covered gaps (e.g. between a KSpinBox's text and its +/−
    // buttons) from bubbling up and accidentally toggling the card.
    MouseArea {
        id: cardMouseLeft
        anchors.left: parent.left
        anchors.right: pcardSlot.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: -1
        onPressed: focusRing.suppress()
        onClicked: { pcard.forceActiveFocus(); pcard._flip() }
    }
    MouseArea {
        id: cardMouseRight
        anchors.left: pcardSlot.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: -1
        onPressed: focusRing.suppress()
        onClicked: { pcard.forceActiveFocus(); pcard._flip() }
    }

    // ── Label on the left ──────────────────────────────────────────────
    Text {
        anchors.left: parent.left
        anchors.leftMargin: Tokens.spaceMd
        anchors.right: pcardSlot.left
        anchors.rightMargin: Tokens.spaceMd
        anchors.verticalCenter: parent.verticalCenter
        text: pcard.label
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontMd
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    // ── Optional content slot (spinbox etc.) between label and toggle ──
    Item {
        id: pcardSlot
        width: pcard.slotWidth
        height: parent.height
        anchors.right: pcardToggle.left
        anchors.rightMargin: pcard.slotWidth > 0 ? Tokens.spaceMd : 0
    }

    // ── Toggle pill on the right (same look as KSwitch indicator) ─────
    Item {
        id: pcardToggle
        width: Math.round(44 * AppPalette.scale)
        height: Math.round(24 * AppPalette.scale)
        anchors.right: parent.right
        anchors.rightMargin: Tokens.spaceMd
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: pcard.checked ? AppPalette.accentBg : AppPalette.trackOff
            border.width: 1
            border.color: pcard.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder
            Behavior on color { ColorAnimation { duration: 120 } }

            Rectangle {
                width: parent.height - 2 * pcard._knobMargin
                height: width
                radius: width / 2
                y: pcard._knobMargin
                x: pcard.checked ? parent.width - width - pcard._knobMargin : pcard._knobMargin
                color: AppPalette.knob
                border.width: 1
                border.color: "#00000022"
                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }
        }
    }

    KFocusRing { id: focusRing }
}

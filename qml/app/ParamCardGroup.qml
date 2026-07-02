import QtQuick 2.15
import kqml_types 1.0

// Collapsible parameter card: header row (label + toggle) and an animated body
// holding child controls. The toggle both flips `checked` and (when expanded)
// reveals the body. Used for grouped sub-settings.
//
// Promoted from an inline component in AppSettingsPage so EchogramSettingsPanel
// can reuse it. References only AppPalette/Tokens/Anim singletons.
Rectangle {
    id: pgroup

    property string label: ""
    property bool checked: false
    property color fillColor: AppPalette.rowRaised   // bg when nested inside another card group
    signal toggled(bool val)

    default property alias bodyData: pgroupBody.data

    readonly property int _knobMargin: Math.max(2, Math.round(2 * AppPalette.scale))
    readonly property int _headerH: Math.round(38 * AppPalette.scale)
    readonly property bool _hovered: pgroupHeaderMouse.containsMouse

    width: parent ? parent.width : implicitWidth
    height: _headerH + bodyContainer.height
    radius: Tokens.radiusLg
    color: pgroup._hovered ? AppPalette.cardHover : pgroup.fillColor
    border.width: Tokens.cardBorderWidth
    border.color: pgroup._hovered ? AppPalette.borderHover : AppPalette.border
    clip: true

    Behavior on color       { ColorAnimation { duration: 110 } }
    Behavior on border.color { ColorAnimation { duration: 110 } }

    activeFocusOnTab: true
    Keys.onReturnPressed: pgroup._flip()
    Keys.onEnterPressed:  pgroup._flip()
    Keys.onSpacePressed:  pgroup._flip()

    function _flip() {
        pgroup.checked = !pgroup.checked
        pgroup.toggled(pgroup.checked)
    }

    // ── Header (clickable, toggle on the right) ────────────────────────
    Item {
        id: pgroupHeader
        width: parent.width
        height: pgroup._headerH

        KFocusRing { id: focusRing; focusItem: pgroup; radius: pgroup.radius }

        MouseArea {
            id: pgroupHeaderMouse
            anchors.left: parent.left
            anchors.right: pgroupToggle.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: focusRing.suppress()
            onClicked: { pgroup.forceActiveFocus(); pgroup._flip() }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Tokens.spaceMd
            anchors.right: pgroupToggle.left
            anchors.rightMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            text: pgroup.label
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontMd
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        // Toggle pill (same look as ParamCard / SmallCheck)
        Item {
            id: pgroupToggle
            width: Math.round(44 * AppPalette.scale)
            height: Math.round(24 * AppPalette.scale)
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: pgroup.checked ? AppPalette.accentBgStrong : AppPalette.trackOff
                border.width: 1
                border.color: pgroup.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder
                Behavior on color       { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }
            }
            Rectangle {
                width: parent.height - 2 * pgroup._knobMargin
                height: width
                radius: width / 2
                y: pgroup._knobMargin
                x: pgroup.checked ? parent.width - width - pgroup._knobMargin : pgroup._knobMargin
                color: AppPalette.knob
                border.width: 1
                border.color: "#00000022"
                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onPressed: focusRing.suppress()
                onClicked: { pgroup.forceActiveFocus(); pgroup._flip() }
            }
        }
    }

    // ── Body (animated; clipped during shrink) ─────────────────────────
    Item {
        id: bodyContainer
        anchors.top: pgroupHeader.bottom
        width: parent.width
        clip: true
        height: pgroup.checked ? pgroupBody.implicitHeight + 2 * Tokens.spaceSm : 0
        visible: bodyContainer.height > 0.5

        Behavior on height {
            NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing }
        }

        Column {
            id: pgroupBody
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Tokens.spaceMd
            anchors.rightMargin: Tokens.spaceMd
            anchors.top: parent.top
            anchors.topMargin: Tokens.spaceSm
            spacing: Tokens.spaceXs
        }
    }
}

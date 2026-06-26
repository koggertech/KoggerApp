import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

// Themed point-of-interest (contact) editor — pops up under the right-click on
// an echogram. Replaces the legacy CContact (MenuFrame + CheckButton). Same API
// (props + signals) so Plot2D wiring is unchanged; styled with AppPalette/Tokens
// and KCircleIconButton so it scales and follows the active theme.
Rectangle {
    id: root

    signal inputAccepted()
    signal setActiveButtonClicked()
    signal setButtonClicked()
    signal deleteButtonClicked()
    signal copyButtonClicked()

    property alias inputFieldText: inputField.text
    property bool accepted: false
    property string info: ""
    property int indx: -1
    property double lat: 0.0
    property double lon: 0.0
    property double depth: 0.0
    property bool isActive: false

    readonly property int btnSize: Tokens.controlHMd

    visible: false
    z: 60
    width: layout.implicitWidth + 2 * Tokens.spaceMd
    height: layout.implicitHeight + 2 * Tokens.spaceMd
    radius: Tokens.radiusLg
    color: AppPalette.card
    border.width: 1
    border.color: AppPalette.border

    function formatNumber(value, decimals) { return value.toFixed(decimals) }

    onVisibleChanged: if (visible) inputField.forceActiveFocus()
    onXChanged: if (visible) inputField.forceActiveFocus()
    onYChanged: if (visible) inputField.forceActiveFocus()

    // Swallow pointer events so they don't fall through to the echogram. Must
    // also grab hover — otherwise the echogram's hoverEnabled MouseArea below
    // gets the move, calls onCursorMoved(), and C++ resets contactVisible →
    // the popup would close as soon as the cursor moved over it.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onPressed: function(m) { m.accepted = true }
        onWheel: function(w) { w.accepted = true }
    }

    component IconBtn: KCircleIconButton {
        Layout.preferredWidth: root.btnSize
        Layout.preferredHeight: root.btnSize
        iconTintColor: AppPalette.text
        fillColor: AppPalette.card
        fillHoverColor: AppPalette.cardHover
        borderColor: AppPalette.border
    }

    component ReadoutField: Rectangle {
        property alias text: readout.text
        Layout.fillWidth: true
        Layout.preferredHeight: root.btnSize
        radius: Tokens.radiusMd
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border
        Text {
            id: readout
            anchors.fill: parent
            anchors.leftMargin: Tokens.spaceSm
            anchors.rightMargin: Tokens.spaceSm
            verticalAlignment: Text.AlignVCenter
            color: AppPalette.text
            font.pixelSize: Tokens.fontSm
            elide: Text.ElideRight
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Tokens.spaceMd
        spacing: Tokens.spaceSm

        RowLayout {
            spacing: Tokens.spaceXs

            IconBtn {
                iconSource: "qrc:/icons/ui/tag.svg"
                toolTipText: qsTr("Mark as active")
                enabled: root.indx >= 0
                opacity: enabled ? 1.0 : 0.4
                fillColor: root.isActive ? AppPalette.accentBgStrong : AppPalette.card
                fillHoverColor: root.isActive ? AppPalette.accentBgStrong : AppPalette.cardHover
                borderColor: root.isActive ? AppPalette.accentBorder : AppPalette.border
                onClicked: { root.visible = false; root.setActiveButtonClicked() }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: Math.round(120 * AppPalette.scale)
                Layout.preferredHeight: root.btnSize
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: 1
                border.color: inputField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextField {
                    id: inputField
                    anchors.fill: parent
                    leftPadding: Tokens.spaceSm
                    rightPadding: Tokens.spaceSm
                    verticalAlignment: TextInput.AlignVCenter
                    background: Item {}
                    color: AppPalette.text
                    placeholderText: qsTr("Enter text")
                    placeholderTextColor: AppPalette.textMuted
                    font.pixelSize: Tokens.fontMd
                    selectByMouse: true
                    onAccepted: {
                        root.accepted = true
                        root.visible = false
                        root.inputAccepted()
                    }
                }
            }

            IconBtn {
                iconSource: "qrc:/icons/ui/plus.svg"
                toolTipText: qsTr("Save")
                onClicked: {
                    root.accepted = true
                    root.visible = false
                    root.setButtonClicked()
                }
            }

            IconBtn {
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.dangerText
                toolTipText: qsTr("Delete")
                onClicked: { root.visible = false; root.deleteButtonClicked() }
            }
        }

        RowLayout {
            visible: root.info.length !== 0
            spacing: Tokens.spaceXs

            IconBtn {
                iconSource: "qrc:/icons/ui/gps.svg"
                iconTintColor: AppPalette.textMuted
                fillColor: "transparent"
                borderColor: "transparent"
                enabled: false
            }

            ReadoutField {
                id: latLonField
                text: root.formatNumber(root.lat, 4) + " " + root.formatNumber(root.lon, 4)
            }

            TextEdit { id: clipHelper; visible: false }

            IconBtn {
                iconSource: "qrc:/icons/ui/click.svg"
                toolTipText: qsTr("Copy")
                onClicked: {
                    clipHelper.text = latLonField.text
                    clipHelper.selectAll()
                    clipHelper.copy()
                    root.visible = false
                    root.copyButtonClicked()
                }
            }
        }

        RowLayout {
            visible: root.info.length !== 0
            spacing: Tokens.spaceXs

            IconBtn {
                iconSource: "qrc:/icons/ui/arrow_bar_down.svg"
                iconTintColor: AppPalette.textMuted
                fillColor: "transparent"
                borderColor: "transparent"
                enabled: false
            }

            ReadoutField {
                text: root.formatNumber(root.depth, 4)
            }
        }
    }
}

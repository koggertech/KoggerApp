import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

// Dedicated factory panel (visible only in factory mode).
// UX: enter the access key once -> the device list fills -> pick a device + Start ->
// the stage line shows the main steps / high-level errors (core.flasherTextInfo). This is an
// operator tool: no low-level detail — the flashing logic lives in the private factory module.
// Backed by core (FLASHER build): flasherProducts / refreshFlasherProducts / setFlasherData /
// connectOpenedLinkAsFlasher / flasherTextInfo / flasherIdInfo.
SettingsGroup {
    id: factoryGroup

    property var store: null

    // Live device list (factory mode only).
    readonly property var _products: (typeof core !== "undefined" && core && core.isFactoryMode && core.flasherProducts)
                                      ? core.flasherProducts : []
    readonly property int _idx: deviceCombo.currentIndex
    readonly property string _selPn: (_idx >= 0 && _idx < _products.length) ? _products[_idx].pn : ""
    readonly property bool _selReady: (_idx >= 0 && _idx < _products.length) ? !!_products[_idx].ready : false
    readonly property bool _isError: (typeof core !== "undefined" && core) ? (core.flasherIdInfo >= 500) : false
    readonly property bool _hasToken: (typeof core !== "undefined" && core && core.isFactoryMode) ? core.flasherHasToken : false

    preferredWidth: width
    title: qsTr("Factory")
    description: qsTr("Factory setup for new devices.")
    stateStore: store
    stateKey: "app.factory"
    collapsedByDefault: false
    contentSpacing: Tokens.spaceMd

    Component.onCompleted: if (typeof core !== "undefined" && core && core.isFactoryMode) core.refreshFlasherProducts()

    // ── Access key ────────────────────────────────────────────────────────
    Row {
        spacing: Tokens.spaceSm
        Text { text: qsTr("Access key:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }
        Text {
            visible: factoryGroup._hasToken
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("✓ saved"); color: "#10B981"; font.pixelSize: Tokens.fontXs
        }
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

        Rectangle {
            width: parent.width - setTokenBtn.width - parent.spacing
            height: Tokens.controlHMd; radius: Tokens.radiusMd; color: AppPalette.bg
            border.width: tokenField.activeFocus ? 1 : Tokens.cardBorderWidth
            border.color: tokenField.activeFocus ? AppPalette.accentBorder : AppPalette.border

            TextInput {
                id: tokenField
                activeFocusOnTab: true
                anchors.fill: parent; anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                verticalAlignment: TextInput.AlignVCenter
                color: AppPalette.text; font.pixelSize: Tokens.fontBase
                echoMode: TextInput.Password; clip: true
                TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: tokenField.selectAll() }

                Text {
                    visible: !tokenField.text.length
                    text: factoryGroup._hasToken ? qsTr("Key saved — paste to replace") : qsTr("Paste key…")
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontBase; anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        KButton {
            id: setTokenBtn
            width: Math.round(64 * AppPalette.scale); height: Tokens.controlHMd
            text: qsTr("Set")
            enabled: tokenField.text.length > 0
            onClicked: {
                core.setFlasherData(tokenField.text)
                tokenField.text = ""
                core.refreshFlasherProducts()
            }
        }
    }

    // ── Device selection ──────────────────────────────────────────────────
    Text { text: qsTr("Device:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

    RowLayout {
        width: parent.width; spacing: Tokens.spaceSm

        KCombo {
            id: deviceCombo
            Layout.fillWidth: true
            Layout.preferredHeight: Tokens.controlHMd
            model: factoryGroup._products.map(function(p) { return p.pn })
        }

        KCircleIconButton {
            width: Tokens.controlHMd; height: Tokens.controlHMd
            cornerRadius: Tokens.radiusMd; borderWidth: 0; scaleOnHover: false
            iconSource: "qrc:/icons/ui/refresh.svg"
            iconPixelSize: Math.round(Tokens.controlHMd * 0.5)
            iconTintColor: AppPalette.text
            toolTipText: qsTr("Refresh device list")
            fillColor: AppPalette.chipRaised; fillHoverColor: AppPalette.chipRaisedHover
            onClicked: if (typeof core !== "undefined" && core) core.refreshFlasherProducts()
        }
    }

    Text {
        visible: factoryGroup._selPn.length > 0 && !factoryGroup._selReady
        width: parent.width; wrapMode: Text.WordWrap
        text: qsTr("Selected device is not ready.")
        color: "#F59E0B"; font.pixelSize: Tokens.fontXs
    }

    // ── Start ─────────────────────────────────────────────────────────────
    KButton {
        width: parent.width; height: Tokens.controlHMd
        text: qsTr("Start flashing")
        enabled: factoryGroup._selPn.length > 0 && factoryGroup._selReady
        onClicked: if (typeof core !== "undefined" && core) core.connectOpenedLinkAsFlasher(factoryGroup._selPn)
    }

    // ── Stage / error line (main steps only) ──────────────────────────────
    Rectangle {
        width: parent.width
        radius: Tokens.radiusMd
        color: AppPalette.bg
        border.width: Tokens.cardBorderWidth
        border.color: factoryGroup._isError ? "#EF4444" : AppPalette.border
        implicitHeight: stageText.implicitHeight + 2 * Tokens.spaceSm
        visible: stageText.text.length > 0

        Text {
            id: stageText
            anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
            wrapMode: Text.WordWrap
            text: (typeof core !== "undefined" && core && core.isFactoryMode) ? core.flasherTextInfo : ""
            color: factoryGroup._isError ? "#EF4444" : AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }
    }
}

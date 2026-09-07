import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property bool _on: page.store ? page.store.aimPanelVisible : true

    component FieldRow: KIslandRow {
        id: fieldRow
        property alias checked: fieldSwitch.checked
        signal switched(bool value)

        interactive: true
        onClicked: fieldSwitch.click()

        KSwitch {
            id: fieldSwitch
            flat: true
            onToggled: fieldRow.switched(checked)
        }
    }

    KSwitch {
        width: parent.width
        text: qsTr("Show information panel")
        toolTipText: qsTr("Panel at the cursor on the echogram. The distance is always in it, the rest is optional.")
        checked: page._on
        onToggled: if (page.store) page.store.aimPanelVisible = checked
    }

    Text {
        text: qsTr("Fields:")
        color: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
        font.pixelSize: Tokens.fontLg
        font.bold: true
        leftPadding: Tokens.spaceXxs
    }

    KIsland {
        enabled: page._on

        FieldRow {
            label: qsTr("Channel")
            toolTipText: qsTr("The channel the cursor reads its numbers from.")
            checked: page.store ? page.store.aimChannel : true
            onSwitched: function(value) { if (page.store) page.store.aimChannel = value }
        }
        FieldRow {
            label: qsTr("Epoch")
            toolTipText: qsTr("Number of the ping under the cursor, counted from the start of the recording.")
            checked: page.store ? page.store.aimEpoch : true
            onSwitched: function(value) { if (page.store) page.store.aimEpoch = value }
        }
        FieldRow {
            label: qsTr("Resolution")
            toolTipText: qsTr("How many millimetres of depth one echogram sample covers.")
            checked: page.store ? page.store.aimResolution : true
            onSwitched: function(value) { if (page.store) page.store.aimResolution = value }
        }
        FieldRow {
            label: qsTr("Frequency")
            toolTipText: qsTr("The frequency this ping was made at, kHz.")
            checked: page.store ? page.store.aimFrequency : true
            onSwitched: function(value) { if (page.store) page.store.aimFrequency = value }
        }
        FieldRow {
            label: qsTr("Pulse count")
            toolTipText: qsTr("How many pulses were emitted for one ping.")
            checked: page.store ? page.store.aimPulseCount : true
            onSwitched: function(value) { if (page.store) page.store.aimPulseCount = value }
        }
        FieldRow {
            label: qsTr("Booster")
            toolTipText: qsTr("Whether the transmit booster was on for this ping.")
            checked: page.store ? page.store.aimBooster : true
            onSwitched: function(value) { if (page.store) page.store.aimBooster = value }
        }
        FieldRow {
            label: qsTr("Speed of sound")
            toolTipText: qsTr("The speed of sound used to turn this ping into depth, m/s.")
            checked: page.store ? page.store.aimSoundSpeed : true
            onSwitched: function(value) { if (page.store) page.store.aimSoundSpeed = value }
        }
    }
}

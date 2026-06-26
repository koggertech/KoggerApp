import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property bool _on: page.store ? page.store.aimPanelVisible : true

    KSwitch {
        width: parent.width
        text: qsTr("Show information panel")
        checked: page._on
        onToggled: if (page.store) page.store.aimPanelVisible = checked
    }

    Text {
        text: qsTr("Fields:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    KSwitch {
        width: parent.width; text: qsTr("Channel"); enabled: page._on
        checked: page.store ? page.store.aimChannel : true
        onToggled: if (page.store) page.store.aimChannel = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Epoch"); enabled: page._on
        checked: page.store ? page.store.aimEpoch : true
        onToggled: if (page.store) page.store.aimEpoch = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Resolution"); enabled: page._on
        checked: page.store ? page.store.aimResolution : true
        onToggled: if (page.store) page.store.aimResolution = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Frequency"); enabled: page._on
        checked: page.store ? page.store.aimFrequency : true
        onToggled: if (page.store) page.store.aimFrequency = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Pulse count"); enabled: page._on
        checked: page.store ? page.store.aimPulseCount : true
        onToggled: if (page.store) page.store.aimPulseCount = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Booster"); enabled: page._on
        checked: page.store ? page.store.aimBooster : true
        onToggled: if (page.store) page.store.aimBooster = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Speed of sound"); enabled: page._on
        checked: page.store ? page.store.aimSoundSpeed : true
        onToggled: if (page.store) page.store.aimSoundSpeed = checked
    }
}

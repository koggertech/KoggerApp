import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        text: qsTr("Devices:")
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontBase
        leftPadding: Tokens.spaceXxs
    }

    DeviceTopologyView {
        width: parent.width
        groups: (typeof deviceTopology !== "undefined" && deviceTopology) ? deviceTopology.groups : []
        activeDevice: page.store ? page.store.activeDevice : null
        onDeviceClicked: function(device) { if (page.store) page.store.selectDevice(device) }
    }

    Text {
        visible: !!(page.store && page.store.activeDevice)
        text: qsTr("Settings:")
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontBase
        leftPadding: Tokens.spaceXxs
    }

    DeviceSettingsPage {
        width: parent.width
        visible: !!(page.store && page.store.activeDevice)
        dev: page.store ? page.store.activeDevice : null
        store: page.store
    }
}

import QtQuick 2.15

Item {
    id: root

    required property var store

    readonly property real sidebarProgress: sidebar.progress

    SettingsSidebarBase {
        id: sidebar

        anchors.fill: parent
        open: root.store.modeSettingsPanelOpen
        dimEnabled: !root.store.settingsPushContent
        panelShadowEnabled: !root.store.editableMode
        title: {
            var paneNumber = root.store.paneNumberByLeafId(root.store.modeSettingsLeafId)
            return paneNumber > 0 ? "Settings \"Pane " + paneNumber + "\"" : "Settings"
        }
        side: root.store.settingsSide
        gearMode: root.store.modeSettingsMode
        headerColor: root.store.paneColorByLeafId(root.store.modeSettingsLeafId)
        panelSizePx: root.store.settingsPanelSizePx
        onCloseRequested: root.store.closeModeSettingsPanel()

        Loader {
            width: parent ? parent.width : implicitWidth
            sourceComponent: root.store.modeSettingsMode === "3D" ? pane3DSettings : pane2DSettings
        }

        Component {
            id: pane2DSettings

            Pane2DSettingsPage {
                store: root.store
                leafId: root.store.modeSettingsLeafId
            }
        }

        Component {
            id: pane3DSettings

            Pane3DSettingsPage {
                store: root.store
                leafId: root.store.modeSettingsLeafId
            }
        }
    }
}

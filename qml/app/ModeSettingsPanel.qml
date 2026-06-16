import QtQuick 2.15

Item {
    id: root

    required property var store

    readonly property real sidebarProgress: sidebar.progress
    readonly property bool pointerInside: sidebar.pointerInside

    SettingsSidebarBase {
        id: sidebar

        anchors.fill: parent
        open: root.store.modeSettingsPanelOpen
        dimEnabled: !root.store.effectivePushContent
        panelShadowEnabled: !root.store.editableMode
        title: {
            var paneNumber = root.store.paneNumberByLeafId(root.store.modeSettingsLeafId)
            return paneNumber > 0 ? qsTr("Settings") + " \"Pane " + paneNumber + "\"" : qsTr("Settings")
        }
        side: root.store.settingsSide
        gearMode: root.store.modeSettingsMode
        headerColor: root.store.paneColorByLeafId(root.store.modeSettingsLeafId)
        panelSizePx: root.store.settingsPanelSizePx
        store: root.store
        onCloseRequested: root.store.closeModeSettingsPanel()

        Loader {
            id: paneSettingsLoader
            width: parent ? parent.width : implicitWidth
            active: false
            sourceComponent: root.store.modeSettingsMode === "3D" ? pane3DSettings : pane2DSettings
            readonly property int targetLeaf: root.store.modeSettingsLeafId
            onTargetLeafChanged: { active = false; active = true }
            Component.onCompleted: active = true
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

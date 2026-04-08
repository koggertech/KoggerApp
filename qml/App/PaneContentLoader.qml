import QtQuick 2.15

Item {
    id: root

    property var paneData
    property int leafId: -1
    property bool rotateEnabled: false
    property bool active: true
    property var workspaceRoot: null

    Loader {
        anchors.fill: parent
        active: root.active
        sourceComponent: root.paneData && root.paneData.mode === "3D" ? pane3DComponent : pane2DComponent
    }

    Component {
        id: pane2DComponent

        Pane2DWindow {
            rotateEnabled: root.rotateEnabled
            workspaceRoot: root.workspaceRoot
            leafId: root.leafId
            paneData: root.paneData
        }
    }

    Component {
        id: pane3DComponent

        Pane3DWindow {
            rotateEnabled: root.rotateEnabled
            workspaceRoot: root.workspaceRoot
            leafId: root.leafId
            paneData: root.paneData
        }
    }
}

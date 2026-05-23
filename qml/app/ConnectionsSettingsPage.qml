import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg
    readonly property real groupWidth: Math.max(0, width)

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Connections")
        description: qsTr("Links, files, logging, imports and factory tools.")
        stateStore: root.store
        stateKey: "app.connections"
        collapsedByDefault: false
        contentSpacing: Tokens.spaceMd

        Loader {
            id: connectionsLoader
            width: parent.width
            active: true
            asynchronous: true
            source: "qrc:/qml/devices/ConnectionViewer.qml"

            onLoaded: {
                if (item) {
                    item.width = width
                    item.store = root.store
                }
                if (item && item.filePath && item.filePath.length > 0)
                    root.store.selectedConnectionFilePath = item.filePath
            }

            onWidthChanged: {
                if (item)
                    item.width = width
            }
        }

        Connections {
            target: connectionsLoader.item
            ignoreUnknownSignals: true

            function onFilePathChanged() {
                if (connectionsLoader.item
                        && connectionsLoader.item.filePath
                        && connectionsLoader.item.filePath.length > 0) {
                    root.store.selectedConnectionFilePath = connectionsLoader.item.filePath
                }
            }
        }
    }
}

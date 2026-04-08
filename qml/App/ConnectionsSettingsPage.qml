import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import KQMLTypes 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: 10
    readonly property real groupWidth: Math.max(0, width)

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: "Connections"
        stateStore: root.store
        stateKey: "app.connections"
        collapsedByDefault: false
        contentSpacing: 10

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Links, files, logging, imports and factory tools."
            color: "#94A3B8"
            font.pixelSize: 12
        }

        Item {
            width: parent.width
            implicitHeight: connectionsCard.implicitHeight
            height: implicitHeight

            Rectangle {
                id: connectionsCard
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                implicitHeight: connectionsLoader.item ? connectionsLoader.item.implicitHeight + 16 : 120
                height: implicitHeight
                radius: 10
                color: "#0B1220"
                border.width: 1
                border.color: "#334155"
                clip: true

                Loader {
                    id: connectionsLoader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    active: true
                    source: "qrc:/ConnectionViewer.qml"

                    onLoaded: {
                        if (item)
                            item.width = width
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
    }
}

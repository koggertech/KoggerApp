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
        id: filesGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Files")
        description: qsTr("Open log files and reopen recent ones.")
        stateStore: root.store
        stateKey: "app.files"
        collapsedByDefault: false
        contentSpacing: Tokens.spaceMd

        headerActions: KCircleIconButton {
            readonly property string _last: root.store ? root.store.selectedConnectionFilePath : ""
            width: filesGroup.headerActionSize
            height: filesGroup.headerActionSize
            cornerRadius: Tokens.radiusLg   // uniform rounded chip, full header height
            borderWidth: 0
            scaleOnHover: false
            enabled: _last.length > 0
            iconSource: "qrc:/icons/ui/folder-open.svg"
            iconPixelSize: Math.round(width * 0.64)
            iconTintColor: AppPalette.textSecond
            fillColor:      AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            toolTipText: qsTr("Open last file")
            onClicked: if (typeof core !== "undefined" && core && _last.length > 0)
                           core.openLogFile(_last, false, false)
        }

        Loader {
            id: filesLoader
            width: parent.width
            active: filesGroup.expanded
                    && (root.store ? root.store.settingsPanelOpen === true : false)
            asynchronous: true
            source: "qrc:/qml/devices/FilesViewer.qml"

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
            target: filesLoader.item
            ignoreUnknownSignals: true

            function onFilePathChanged() {
                if (filesLoader.item
                        && filesLoader.item.filePath
                        && filesLoader.item.filePath.length > 0) {
                    root.store.selectedConnectionFilePath = filesLoader.item.filePath
                }
            }
        }
    }
}

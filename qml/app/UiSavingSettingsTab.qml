import QtQuick 2.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

    function localPath(value) {
        if (!value) return ""
        if (typeof value === "string") {
            if (value.startsWith("file:///"))
                return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
            if (value.startsWith("file://"))
                return value.slice(7)
            return value
        }
        var lp = value.toLocalFile ? value.toLocalFile() : ""
        return lp.length ? lp : value.toString()
    }

    function safeFolder(folderUrl) {
        var lp = localPath(folderUrl)
        if (lp.length && uiStateSerializer && uiStateSerializer.pathExists(lp))
            return folderUrl
        return StandardPaths.writableLocation(StandardPaths.HomeLocation)
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Export the whole interface (layout, panels, all echogram/3D settings) to a JSON file, or import it from one.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Row {
        width: parent.width
        spacing: Tokens.spaceMd

        KButton {
            width: (parent.width - Tokens.spaceMd) / 2
            height: Tokens.controlHMd
            text: qsTr("Export…")
            onClicked: {
                uiExportDialog.currentFolder = page.safeFolder(page.exportFolder)
                uiExportDialog.open()
            }
        }

        KButton {
            width: (parent.width - Tokens.spaceMd) / 2
            height: Tokens.controlHMd
            text: qsTr("Import…")
            onClicked: {
                uiImportDialog.currentFolder = page.safeFolder(page.importFolder)
                uiImportDialog.open()
            }
        }
    }

    FileDialog {
        id: uiExportDialog
        title: qsTr("Export UI state")
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON (*.json)", "All Files (*)"]
        onAccepted: {
            page.exportFolder = uiExportDialog.currentFolder
            var path = page.localPath(uiExportDialog.selectedFile)
            if (!path.length) return
            if (!path.toLowerCase().endsWith(".json")) path += ".json"
            if (page.store) page.store.saveLayoutState()
            uiStateSerializer.exportToJsonFile(path)
        }
    }

    FileDialog {
        id: uiImportDialog
        title: qsTr("Import UI state")
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON (*.json)", "All Files (*)"]
        onAccepted: {
            page.importFolder = uiImportDialog.currentFolder
            var path = page.localPath(uiImportDialog.selectedFile)
            if (!path.length) return
            if (uiStateSerializer.importFromJsonFile(path) && page.store)
                page.store.reapplyImportedUiState()
        }
    }

    Settings { property alias uiStateExportFolder: page.exportFolder }
    Settings { property alias uiStateImportFolder: page.importFolder }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        visible: text.length > 0
        text: uiStateSerializer ? (uiStateSerializer.lastError.length ? uiStateSerializer.lastError : uiStateSerializer.lastStatus) : ""
        color: uiStateSerializer && uiStateSerializer.lastError.length ? "#FF6B6B" : AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }
}

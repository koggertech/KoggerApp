import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import "../controls"
import "../menus"


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: !!(dev && dev.isUpgradeSupport)
    property var upgradeFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property string selectedUpgradePathSource: ""

    function sourceUrl(value) {
        if (!value) {
            return ""
        }

        if (typeof value === "string") {
            if (value.startsWith("file:///")) {
                return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
            }
            if (value.startsWith("file://")) {
                return value.slice(7)
            }
            return value
        }

        var localPath = value.toLocalFile ? value.toLocalFile() : ""
        return localPath && localPath.length ? localPath : value.toString()
    }

    function displayUrl(value) {
        var source = sourceUrl(value)
        if (!source.length) {
            return ""
        }

        try {
            return decodeURIComponent(source)
        } catch (error) {
            return source
        }
    }

    function effectiveSource(displayText, storedSource) {
        if (!displayText || !displayText.length) {
            return ""
        }

        if (storedSource && displayText === displayUrl(storedSource)) {
            return storedSource
        }

        return displayText
    }

    function setUpgradePath(path) {
        selectedUpgradePathSource = sourceUrl(path)
        pathText.text = displayUrl(selectedUpgradePathSource)
    }

    function currentUpgradePath() {
        return effectiveSource(pathText.text, selectedUpgradePathSource)
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Please choose a file")
        currentFolder: control.upgradeFolder
        nameFilters: ["Upgrade files (*.ufw)"]

        onCurrentFolderChanged: {
            control.upgradeFolder = currentFolder
        }

        onAccepted: {
            control.upgradeFolder = fileDialog.currentFolder
            control.setUpgradePath(fileDialog.selectedFile)
        }
        onRejected: {
        }
    }

    Settings {
        property alias upgradeFolder: control.upgradeFolder
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: qsTr("Upgrade")

            CProgress {
                Layout.leftMargin: 20
                Layout.preferredWidth: 300
                Layout.fillWidth: true
                Layout.fillHeight: true
                from: -1
                to: 101
                value: dev && dev.upgradeFWStatus !== undefined ? dev.upgradeFWStatus : 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            width: control.width

            CTextField {
                id: pathText
                hoverEnabled: true
                Layout.fillWidth: true

                text: ""
                placeholderText: qsTr("Enter path")
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                onClicked: {
                    fileDialog.currentFolder = control.upgradeFolder
                    fileDialog.open()
                }
            }

            CButton {
                text: qsTr("UPGRADE")
                Layout.fillWidth: false
                Layout.leftMargin: 10
                visible: pathText.text !== ""

                onClicked: {
                    core.upgradeFW(control.currentUpgradePath(), dev)
                }
            }
        }
    }
}

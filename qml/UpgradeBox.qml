import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev ? dev.isUpgradeSupport : false

    FileDialog {
        id: fileDialog
        title: qsTr("Please choose a file")
        currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: ["Upgrade files (*.ufw)"]
        onAccepted: {
            pathText.text = fileDialog.selectedFile.toString()
        }
        onRejected: {
        }
    }

    Settings {
        property alias upgradeFolder: fileDialog.currentFolder
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
                value: dev ? dev.upgradeFWStatus : 0
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
                    fileDialog.open()
                }
            }

            CButton {
                text: qsTr("UPGRADE")
                Layout.fillWidth: false
                Layout.leftMargin: 10
                visible: pathText.text != ""

                onClicked: {
                    core.upgradeFW(pathText.text, dev)
                }
            }
        }
    }
}

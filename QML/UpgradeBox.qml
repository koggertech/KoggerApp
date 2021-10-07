import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isUpgradeSupport

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        nameFilters: ["Upgrade files (*.ufw)"]
        onAccepted: {
            pathText.text = fileDialog.fileUrl.toString()
        }
        onRejected: {
        }
    }

    Settings {
        property alias upgradeFolder: fileDialog.folder
    }

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Upgrade"

            CProgress {
                Layout.leftMargin: 20
                Layout.preferredWidth: 300
                Layout.fillWidth: true
                Layout.fillHeight: true
                from: -1
                to: 101
                value: dev.upgradeFWStatus
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
                text: "UPGRADE"
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

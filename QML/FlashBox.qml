import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


DevSettingsBox {
    id: control
    Layout.preferredHeight: columnItem.height
    isActive: dev.isUpgradeSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            spacing: 10

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: "Write"

                onClicked: {
                    dev.flashSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: "Erase"

                onClicked: {
                    dev.resetSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: "Reboot"

                onClicked: {
                    dev.reboot()
                }
            }
        }
    }
}

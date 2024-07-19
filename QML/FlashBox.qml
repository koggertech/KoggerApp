import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
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
                text: qsTr("Write")

                onClicked: {
                    dev.flashSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Erase")

                onClicked: {
                    dev.resetSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Reboot")

                onClicked: {
                    dev.reboot()
                }
            }
        }
    }
}

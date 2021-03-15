import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    id: control
    Layout.preferredHeight: columnItem.height

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
                from: 0
                to: 100
                value: sonarDriver.upgradeFWStatus
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            width: control.width

            TextField {
                id: pathText
                hoverEnabled: true
                Layout.fillWidth: true
                height: control.height
                padding: 4
                rightPadding: 40
                font.family: "Bahnschrift"; font.pointSize: 14;
                color: "#E07000"

                text: ""
                placeholderText: qsTr("Enter path")

                background: Rectangle {
                    color: "#505050"
                }
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                implicitHeight: 30
                onClicked: {
                    fileDialog.open()
                }
            }

            CButton {
                text: "UPGRADE"
                Layout.fillWidth: false
                Layout.leftMargin: 10
                implicitHeight: 30
                visible: pathText.text != ""
                onClicked: {
                    core.upgradeFW(pathText.text)
                }
            }
        }
    }
}

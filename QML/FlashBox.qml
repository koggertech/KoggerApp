import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


Item {
    id: control
    Layout.preferredHeight: columnItem.height

//    FileDialog {
//        id: fileDialog
//        title: "Please choose a file"
//        folder: shortcuts.home
//        onAccepted: {
//            console.log("You chose: " + fileDialog.fileUrls)
//            pathText.text = fileDialog.fileUrl.toString()
//        }
//        onRejected: {
//            console.log("Canceled")
//        }
//    }

//    Settings {
//        property alias upgradeFolder: fileDialog.folder
//    }

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        TitleMenuBox {
            titleText: "Settings"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            spacing: 10

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: "Write"

                onClicked: {
                    sonarDriver.flashSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: "Erase"

                onClicked: {
                    sonarDriver.resetSettings()
                }
            }

//            CButton {
//                Layout.fillWidth: true
//                Layout.preferredWidth: 100
//                text: "TO FILE"

//                onClicked: {
//                }
//            }

//            CButton {
//                Layout.fillWidth: true
//                Layout.preferredWidth: 100
//                text: "FROM FILE"

//                onClicked: {

//                }
//            }
        }
    }

}

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


Item {
    id: control
    Layout.preferredHeight: columnItem.height

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

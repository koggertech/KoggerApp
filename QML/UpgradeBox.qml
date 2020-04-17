import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    id: control
    Layout.fillWidth: true
    height: 30


    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + fileDialog.fileUrls)
            pathText.text = fileDialog.fileUrl.toString()
        }
        onRejected: {
            console.log("Canceled")
        }
    }

    RowLayout {
        Layout.fillWidth: true
        width: control.width

        TextField {
            id: pathText
            Layout.fillWidth: true
            height: control.height
            padding: 4
            rightPadding: 40
            font.family: "Bahnschrift"; font.pointSize: 14;
            color: "#F07000"

            text: ""
            placeholderText: qsTr("Enter path")

            background: Rectangle {
                color: "#104060"
            }

            CButton {
                x: pathText.width - width
                Layout.fillWidth: false
                implicitWidth: 30
                implicitHeight: 30
                width: 30
                height: 30

                font.pointSize: 24;
                text: ">"

                onClicked: {
                    fileDialog.open()
                }
            }
        }

        CButton {
            Layout.fillWidth: false
            implicitWidth: 100
            implicitHeight: 30
            font.pointSize: 16;
            text: "UPGRADE"
            onClicked: {
                core.upgradeFW(pathText.text)
            }
        }
    }


}

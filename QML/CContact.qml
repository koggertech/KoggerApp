import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Item {
    id: inputDialog
    property alias inputFieldText: inputField.text
    property bool accepted: false
    property string info: ""
    property int indx: -1

    width: 185
    visible: false


    Connections {
        target: plot
        onContactHovered: {
            console.log("Hovered in QML!")
            // ...
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            CheckButton {
                icon.source: "./icons/plus.svg"
                backColor: theme.controlBackColor
                implicitWidth: theme.controlHeight
                checkable: false

                onClicked: {
                    inputDialog.accepted = true;
                    inputDialog.visible = false;
                }
            }

            CheckButton {
                icon.source: "./icons/x.svg"
                backColor: theme.controlBackColor
                implicitWidth: theme.controlHeight
                checkable: false

                onClicked: {
                    inputDialog.accepted = false;
                    inputDialog.visible = false;
                }
            }

            CTextField {
                id: inputField
                placeholderText: qsTr("Enter text")
                Layout.fillWidth: true
                text: info
            }
        }



    }
}

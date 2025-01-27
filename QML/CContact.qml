import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Item {
    id: inputDialog
    property alias inputFieldText: inputField.text
    property bool accepted: false
    property string info: ""
    property int indx: -1
    property double lat: 0.0
    property double lon: 0.0
    property double depth: 0.0

    width: 185
    visible: false


    Connections {
        target: plot
        onContactHovered: {
            console.log("Hovered in QML!")
            // ...
        }
    }

    function formatNumber(value, decimals) {
        return value.toFixed(decimals);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            spacing: 0

            CheckButton {
                id: setButton
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
                id: deleteButton
                icon.source: "./icons/x.svg"
                backColor: theme.controlBackColor
                implicitWidth: theme.controlHeight
                checkable: false

                onClicked: {
                    contactDialog.deleteButtonClicked();
                    inputDialog.visible = false;
                }
            }

            CTextField {
                id: inputField
                placeholderText: qsTr("Enter text")
                Layout.fillWidth: true
                text: info

                onAccepted: {
                    inputDialog.accepted = true;
                    inputDialog.visible = false;
                }
            }
        }

        CTextField {
            id: latText
            Layout.fillWidth: true
            text:  inputDialog.formatNumber(lat, 4) + " " + inputDialog.formatNumber(lon, 4)
            visible: info.length != 0
        }
        CTextField {
            id: depthText
            Layout.fillWidth: true
            text: inputDialog.formatNumber(depth, 4)
            visible: info.length != 0
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


MenuFrame {
    id: inputDialog
    property alias inputFieldText: inputField.text
    property bool accepted: false
    property string info: ""
    property int indx: -1
    property double lat: 0.0
    property double lon: 0.0
    property double depth: 0.0

    visible: false

    width: 185

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
        Layout.fillWidth: true

        RowLayout {
            CheckButton {
                icon.source: "./icons/tag.svg"
                backColor: theme.menuBackColor
                borderColor: theme.menuBackColor
                implicitWidth: theme.controlHeight
                checkable: false
                enabled: false
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
        }

        RowLayout {
            visible: info.length != 0
            CheckButton {
                icon.source: "./icons/gps.svg"
                backColor: theme.menuBackColor
                borderColor: theme.menuBackColor
                implicitWidth: theme.controlHeight
                checkable: false
                enabled: false
            }
            CTextField {
                id: latLonText
                Layout.fillWidth: true
                text:  inputDialog.formatNumber(lat, 4) + " " + inputDialog.formatNumber(lon, 4)
                readOnly: true
            }
            TextEdit {
                id: textEdit
                visible: false
            }
            CheckButton {
                id: copyButton
                icon.source: "./icons/click.svg"
                backColor: theme.controlBackColor

                implicitWidth: theme.controlHeight
                checkable: false

                onClicked: {
                    textEdit.text = latLonText.text
                    textEdit.selectAll()
                    textEdit.copy()
                    inputDialog.visible = false
                }
            }
        }

        RowLayout {
            visible: info.length != 0
            CheckButton {
                icon.source: "./icons/arrow-bar-down.svg"
                backColor: theme.menuBackColor
                borderColor: theme.menuBackColor
                implicitWidth: theme.controlHeight
                checkable: false
                enabled: false
            }
            CTextField {
                id: depthText
                Layout.fillWidth: true
                text: inputDialog.formatNumber(depth, 4)
                readOnly: true
            }
        }
    }
}

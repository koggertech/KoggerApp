import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import Qt.labs.settings 1.1

Rectangle {
    id: consoleOut
    width: parent.width
    height: parent.height
    color: theme.menuBackColor

    ColumnLayout {
        width: parent.width
        height: parent.height



        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: theme.menuBackColor

            RowLayout {
                Layout.alignment: Qt.AlignRight

                CCheck {
                    id: consScrollEnable
                    checked: true
                    text: qsTr("Auto scroll")
                    Layout.alignment: Qt.AlignRight

                    Settings {
                        property alias consScrollEnable: consScrollEnable.checked
                    }
                }

                CCheck {
                    id: protoBinConsoled
                    checked: false
                    text: qsTr("Binnary")
                    Layout.alignment: Qt.AlignRight

                    onCheckedChanged: deviceManagerWrapper.protoBinConsoled = protoBinConsoled.checked
                    Component.onCompleted: deviceManagerWrapper.protoBinConsoled = protoBinConsoled.checked

                    Settings {
                        property alias protoBinConsoled: protoBinConsoled.checked
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            DelegateModel {
                id: visualModel

                model: core.consoleList

                groups: [
                    DelegateModelGroup { name: "selected" }
                ]

                delegate: RowLayout {

                    TextEdit  {
                        Layout.fillWidth: true
                        text: time + "  " + payload
                        height: 16
                        font.pointSize: 10
                        font.family: "Console"
                        color: theme.textColor
                        readOnly: true
                        selectByMouse: true
                    }
                }
            }

            ListView {
                model: visualModel
                Layout.margins: 12
                Layout.fillWidth: true
                Layout.fillHeight: true

                onCountChanged: {
                    if(consScrollEnable.checked) {
                        Qt.callLater( positionViewAtEnd )
                    }
                }

                ScrollBar.vertical: ScrollBar { }

                focus: true

            }
        }
    }
}

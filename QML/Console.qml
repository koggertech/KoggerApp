import QtQuick 2.6
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQml.Models 2.12
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
                    text: "auto scroll"
                    Layout.alignment: Qt.AlignRight

                    Settings {
                        property alias consScrollEnable: consScrollEnable.checked
                    }
                }

                CCheck {
                    id: protoBinConsoled
                    checked: false
                    text: "Binnary"
                    Layout.alignment: Qt.AlignRight

                    onCheckedChanged: devs.protoBinConsoled = protoBinConsoled.checked
                    Component.onCompleted: devs.protoBinConsoled = protoBinConsoled.checked

                    Settings {
                        property alias protoBinConsoled: protoBinConsoled.checked
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            ListModel {
                id: colorCategory

                ListElement {
                    name: "QtDebugMsg"
                    color: "gray"
                }
                ListElement {
                    name: "QtWarningMsg"
                    color: "orange"
                }
                ListElement {
                    name: "QtCriticalMsg"
                    color: "red"
                }
                ListElement {
                    name: "QtFatalMsg"
                    color: "red"
                }
                ListElement {
                    name: "QtInfoMsg"
                    color: "#DDDDDD"
                }
            }

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
//                        color: colorCategory.get(category).color
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

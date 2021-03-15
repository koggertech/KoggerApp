import QtQuick 2.6
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQml.Models 2.12
import Qt.labs.settings 1.1

Rectangle {
    id: consoleOut
    width: parent.width
    height: parent.height
    color: "#404040"

    ColumnLayout {
        width: parent.width
        height: parent.height



        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#505050"

            RowLayout {
                Layout.alignment: Qt.AlignRight

                CCheck {
                    id: consScrollEnable
                    checked: false
                    font.pixelSize: 14
                    text: "auto scroll"
                    Layout.alignment: Qt.AlignRight

                    Settings {
                        property alias consScrollEnable: consScrollEnable.checked
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
                        height: 20
                        color: colorCategory.get(category).color
                        font.pointSize: 12
                        font.family: "Console"
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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import QtQml.Models 2.15


DevSettingsBox {
    id: control
    isActive: dev ? dev.isRecorder : false

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: "Files"

//            DelegateModel {
//                id: filesList


//                model: core.consoleList

//                groups: [
//                    DelegateModelGroup { name: "selected" }
//                ]

//                delegate: RowLayout {
//                    Layout.maximumWidth: 100
//                    CText  {
//                        Layout.fillWidth: true

//                        text: time + "  " + payload
//                    }

//                    CCheck {

//                    }
//                }
//            }



            Component {
                id: fileItem

                Item {
                    id: wrapper
                    width: filesList.width; height: 28

                    RowLayout {
                        id: rowItem
                        spacing: 0
                        anchors.fill: parent
//                        margins: 4
                        CTextField {
                            text: "#" + id
                            implicitWidth: 70
                            background:  Rectangle {
                                color: recordState === 3 ? "red" : "transparent"
                                border.width: 1
                                border.color: theme.controlBorderColor
                            }
                        }

                        CTextField {
                            text: "31.12.21 11:11"
                            implicitWidth: 170
                            background:  Rectangle {
                                color: "transparent"
                                border.width: 1
                                border.color: theme.controlBorderColor
                            }
                        }

                        CTextField {
                            Layout.fillWidth: true
                            text: Math.ceil(doneSize/(1024*1024)) + qsTr("MB / ") + Math.ceil(size/(1024*1024)) + qsTr(" MB")
                            background:  Item {

                                Rectangle {
                                    height: parent.height
                                    anchors.bottom: parent.bottom
                                    color: "green"
                                    width: (parent.width)*doneSize / (size + 1)
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    border.width: 1
                                    border.color: theme.controlBorderColor
                                }
                            }

                            Timer {

                            }
                        }

                        CButton {
                            text: "D"
                            implicitWidth: 26
                            implicitHeight: 26
                            Layout.leftMargin: 4

//                            background: Rectangle {
//                                color:   theme.controlBorderColor
//                            }

                            onClicked: {
                                filesList.currentIndex = index
                                dev.requestStream(id);
                            }
                        }
                    }
                }
            }

            ListView {
                id: filesList
                model: deviceManagerWrapper.streamsList
                Layout.margins: 0
                Layout.topMargin: 30
                Layout.bottomMargin: 30
                Layout.fillWidth: true
                Layout.fillHeight: true
                height: 400
                delegate: fileItem
                focus: true
//                flickableDirection: Flickable.AutoFlickDirection

//                onCurrentIndexChanged: {
//                    console.log(filesList.currentIndex);
//                }

//                contentWidth: 320

//                highlight: Rectangle { color: "lightsteelblue"; radius: 5 }

//                onCountChanged: {
//                    if(consScrollEnable.checked) {
//                        Qt.callLater( positionViewAtEnd )
//                    }
//                }

//                ScrollBar.vertical: ScrollBar { }

            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Update list")

                    onClicked: {
                        dev.requestStreamList();
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Download")

                    onClicked: {
//                        dev.resetSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: qsTr("Pause")

                    onClicked: {
//                        dev.reboot()
                    }
                }
            }
        }


        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CCombo  {
                id: baudrateCombo
                Layout.fillWidth: true
                model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000]
                currentIndex: 4
            }

            CButton {
                text: qsTr("Set baudrate")

                onClicked: {
                    dev.baudrate = Number(baudrateCombo.currentText)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Flash settings")

                onClicked: {
                    dev.flashSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Erase settings")

                onClicked: {
                    dev.resetSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 60
                text: qsTr("Reboot")

                onClicked: {
                    dev.reboot()
                }
            }
        }

    }
}

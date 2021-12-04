import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import QtQml.Models 2.12


DevSettingsBox {
    id: control
    isActive: dev.isRecorder

    MenuBlock {
    }

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

            ListView {
                id: filesList
                model: devs.streamsList
                Layout.margins: 0
                Layout.topMargin: 30
                Layout.bottomMargin: 30
                Layout.fillWidth: true
                Layout.fillHeight: true
                height: 400
                delegate: fileItem

                Component {
                    id: fileItem



                    Item {
                        width: filesList.width; height: 26

//                        Rectangle {
//                            anchors.fill: parent
//                            color: recordState == 3 ? "#DD2211" : "transparent"
//                            opacity: 0.1
//                        }

                        RowLayout {

                            spacing: 0
                            anchors.fill: parent
                            CTextField {
//                                Layout.fillWidth: true
                                text: "#" + id
                                implicitWidth: 80
                                background:  Rectangle {
                                    color: recordState == 3 ? "#AA1111" : theme.controlBackColor
                                    border.width: 1
                                    border.color: theme.controlBorderColor
                                }
                            }

                            CTextField {
//                                Layout.fillWidth: true
                                text: "31.12.21 11:11"
                                implicitWidth: 170
                                background:  Rectangle {
                                    color: recordState == 3 ? "#AA1111" : theme.controlBackColor
                                    border.width: 1
                                    border.color: theme.controlBorderColor
                                }
                            }

                            CTextField {
                                Layout.fillWidth: true
                                text: Math.ceil(size/(1024*1024)) + " MB"
                                background:  Rectangle {
                                    color: recordState == 3 ? "#AA1111" : theme.controlBackColor
                                    border.width: 1
                                    border.color: theme.controlBorderColor
                                }
                            }
                        }


                    }
                }


//                onCountChanged: {
//                    if(consScrollEnable.checked) {
//                        Qt.callLater( positionViewAtEnd )
//                    }
//                }

                ScrollBar.vertical: ScrollBar { }

                focus: true

            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Update list"

                    onClicked: {
                        dev.requestStreamList();
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Download"

                    onClicked: {
//                        dev.resetSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: "Pause"

                    onClicked: {
//                        dev.reboot()
                    }
                }
            }
        }

    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: true

        FileDialog {
            id: fileDialog
            title: qsTr("Please choose a file")
            folder: shortcuts.home
            nameFilters: ["Upgrade files (*.bin)"]
            onAccepted: {
                pathText.text = fileDialog.fileUrl.toString()
            }
            onRejected: {
            }
        }

        Settings {
            property alias upgradeFolder: fileDialog.folder
        }

    //MenuBlock {
    //}

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

//        TitleMenuBox {
//            titleText: "Factory"
//        }


        ParamGroup {
            groupName: qsTr("Factory")

            ParamSetup {
                paramName: qsTr("PN")

                CTextField {
                    id:pn_nbr
                    hoverEnabled: true
                    Layout.fillWidth: true

                    placeholderText: qsTr("Enter PN")

                    background: Rectangle {
                        color: "#505050"
                    }

                    onTextChanged: {

                    }

                    Component.onCompleted: {

                    }

                    Settings {
                        property alias factory_pn: pn_nbr.text
                    }
                }
            }

            //        RowLayout {
            //            Layout.fillWidth: true
            //            Layout.margins: 10
            //            spacing: 10


            //            Text {
            //                text: "SN"
            //                color: "#808080"
            //                font.pixelSize: 16
            //            }

            //            SpinBoxCustom {
            //                id:sn_nbr
            //                width: 200
            //                Layout.fillWidth: true
            //                from: 0
            //                value: 3001001
            //                to: 2000000000
            //                stepSize: 1
            //                onValueChanged: {
            //                }

            //                Settings {
            //                    property alias factory_sn: sn_nbr.value
            //                }
            //            }

            //            TextField {
            //                id:pn_nbr
            //                text:"OEM0369005"
            //                hoverEnabled: true
            //                Layout.fillWidth: true
            //                height: control.height
            //                padding: 4
            //                rightPadding: 40
            //                font.family: "Bahnschrift"; font.pointSize: 14;
            //                color: "#E07000"

            //                placeholderText: qsTr("Enter PN")

            //                background: Rectangle {
            //                    color: "#505050"
            //                }

            //                Settings {
            //                    property alias factory_pn: pn_nbr.text
            //                }
            //            }
            //        }

            //        RowLayout {
            //            Layout.fillWidth: true
            //            Layout.margins: 10
            //            width: control.width

            //            FileDialog {
            //                id: factoryDialog
            //                title: "Please choose a file"
            //                folder: shortcuts.home
            //                nameFilters: ["Factory file (*.kff)"]
            //                onAccepted: {
            //                    pathFactory.text = factoryDialog.fileUrl.toString()
            //                }
            //                onRejected: {
            //                }


            //            }

            //            Settings {
            //                property alias factoryFolder: factoryDialog.folder
            //            }

            //            TextField {
            //                id: pathFactory
            //                hoverEnabled: true
            //                Layout.fillWidth: true
            //                height: control.height
            //                padding: 4
            //                rightPadding: 40
            //                font.family: "Bahnschrift"; font.pointSize: 14;
            //                color: "#E07000"

            //                text: ""
            //                placeholderText: qsTr("Factory FW")

            //                background: Rectangle {
            //                    color: "#505050"
            //                }
            //            }

            //            CButton {
            //                text: "..."
            //                Layout.fillWidth: false
            //                implicitHeight: 30
            //                onClicked: {
            //                    factoryDialog.open()
            //                }
            //            }
            //        }

            //        RowLayout {
            //            Layout.fillWidth: true
            //            Layout.margins: 10
            //            spacing: 10

            //            ColumnLayout {
            //                Layout.fillWidth: true
            //                Text {
            //                    text: "Write progress"
            //                    color: "#808080"
            //                    font.pixelSize: 16
            //                }

            //                CProgress {
            //                    Layout.leftMargin: 0
            //                    Layout.preferredWidth: 100
            //                    Layout.preferredHeight: 20
            //                    Layout.fillWidth: true
            //                    Layout.fillHeight: true
            //                    from: 0
            //                    to: 100
            //                    value: flasher.writeProgress
            //                }
            //            }

            //            ColumnLayout {
            //                Layout.fillWidth: true
            //                Text {
            //                    text: "Read progress"
            //                    color: "#808080"
            //                    font.pixelSize: 16
            //                }

            //                CProgress {
            //                    Layout.leftMargin: 0
            //                    Layout.preferredWidth: 100
            //                    Layout.preferredHeight: 20
            //                    Layout.fillWidth: true
            //                    Layout.fillHeight: true
            //                    from: 0
            //                    to: 100
            //                    value: flasher.readProgress
            //                }
            //            }

            //            ColumnLayout {
            //                Layout.fillWidth: true
            //                Text {
            //                    text: "Check"
            //                    color: "#808080"
            //                    font.pixelSize: 16
            //                }

            //                CProgress {
            //                    Layout.leftMargin: 0
            //                    Layout.preferredWidth: 100
            //                    Layout.preferredHeight: 20
            //                    Layout.fillWidth: true
            //                    Layout.fillHeight: true
            //                    from: 0
            //                    to: 100
            //                    value: flasher.checkProgress
            //                }
            //            }
            //        }

            ParamSetup {
                paramName: qsTr("Write")

                CProgress {
                    Layout.leftMargin: 0
                    Layout.preferredWidth: 300
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    from: 0
                    to: 100
                    // value: flasher.writeProgress // TODO
                }
            }

            ParamSetup {
                paramName: qsTr("Read")

                CProgress {
                    Layout.leftMargin: 0
                    Layout.preferredWidth: 300
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    from: 0
                    to: 100
                    // value: flasher.readProgress // TODO
                }
            }

            ParamSetup {
                paramName: qsTr("Check")

                CProgress {
                    Layout.leftMargin: 0
                    Layout.preferredWidth: 300
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    from: 0
                    to: 100
                    // value: flasher.checkProgress // TODO
                }
            }

            ParamSetup {
                paramName: qsTr("Upgrade")

                CProgress {
                    Layout.leftMargin: 20
                    Layout.preferredWidth: 300
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    from: -1
                    to: 101
                    value: dev ? dev.upgradeFWStatus : 0
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                width: control.width

                CButton {
                    text: qsTr("FLASH")
                    Layout.fillWidth: true
                    implicitHeight: 30
                    onClicked: {
                        core.factoryFlash("", 0, pn_nbr.text, dev)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                width: control.width

                CButton {
                    text: qsTr("Simple FLASH")
                    Layout.fillWidth: true
                    implicitHeight: 30
                    onClicked: {
                        core.simpleFlash(fileDialog.fileUrl.toString())
                    }
                }

                CButton {
                    text: "..."
//                    Layout.fillWidth: true
                    implicitHeight: 30
                    onClicked: {
                        fileDialog.open()
                    }
                }
            }

        }


    }
}

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

ColumnLayout {
    property var dev: null
    property var devList: deviceManagerWrapper.devs

    Layout.margins: 0
    spacing: 10


    onDevListChanged: {
        if (devList.length > 0) {
            dev = devList[0]
        }
    }

    Connections {
        target: core

        function onConnectionChanged() {
            connectionButton.connection = core.isOpenConnection()
            dev = null
        }
    }

    MenuRow {
        // Layout.margins: 0
        Component {
            id: fileItem

            Item {
                id: wrapper
                width: filesList.width; height: theme.controlHeight+4

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    // anchors.verticalCenter: parent
                    color: ConnectionStatus ? "#005000" : IsNotAvailable ? "#500000" : theme.controlBackColor
                    border.width: 0
                    border.color: theme.controlBorderColor
                    radius: 2
                }

                RowLayout {
                    spacing: 1
                    anchors.fill: parent
                    anchors.margins: 1

                    CheckButton {
                        id: linkSettingsButton
                        width: theme.controlHeight
                        height: theme.controlHeight
                        icon.source: "./icons/settings.svg"
                        borderWidth: 0
                        implicitWidth: theme.controlHeight

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Settings")
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/pin.svg"
                        checked: IsPinned
                        implicitWidth: theme.controlHeight

                        onToggled: {
                            linkManagerWrapper.sendUpdatePinnedState(Uuid, checked)
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: checked ? qsTr("Unpin") : qsTr("Pin")
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/repeat.svg"
                        checked: ControlType
                        // text: "Auto"
                        implicitWidth: theme.controlHeight

                        onToggled: {
                            linkManagerWrapper.sendUpdateControlType(Uuid, Number(checked))
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Auto")
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked && LinkType == 2
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/x.svg"
                        checked: false
                        implicitWidth: theme.controlHeight

                        onToggled: {
                            if(checked) {
                                linkManagerWrapper.deleteLink(Uuid)
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Delete")
                    }

                    // CCheck {
                    //     visible: linkSettingsButton.checked
                    //     leftPadding: 6
                    //     rightPadding: 2
                    //     Layout.fillWidth: true
                    //     text: "Auto"
                    //     checked: ControlType
                    //     background:  Rectangle {
                    //         color: "transparent"
                    //         border.width: 0
                    //         border.color: theme.controlBorderColor
                    //     }

                    //     onToggled: {
                    //         linkManagerWrapper.sendUpdateControlType(Uuid, Number(checked))
                    //     }
                    // }

                    CTextField {
                        width: 40
                        Layout.fillWidth: true
                        selectByMouse: true
                        readOnly: true
                        visible: LinkType == 1
                        text: PortName

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }
                    }

                    CCombo  {
                        property bool isStartup: true

                        id: baudrateCombo
                        implicitWidth: 150
                        // Layout.fillWidth: true
                        visible: LinkType == 1
                        model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000]
                        currentIndex: 7
                        displayText: Baudrate

                        onCurrentTextChanged: {
                            if (LinkType == 1 && !isStartup) {
                                console.info("baudrateCombo: onCurrentTextChanged: currentText: " + Number(baudrateCombo.currentText))
                                linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
                            }
                            isStartup = false
                        }

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }
                    }

                    CText {
                        visible: LinkType == 2
                        small: true
                        leftPadding: 6
                        rightPadding: 0
                        text: qsTr("ip:")
                    }

                    CTextField {
                        id: ipAddressText
                        visible: LinkType == 2
                        hoverEnabled: true
                        selectByMouse: true
                        Layout.fillWidth: true
                        leftPadding: 0
                        rightPadding: 6

                        text: Address
                        // placeholderText: "ip"

                        onTextEdited: {
                            linkManagerWrapper.sendUpdateAddress(Uuid, ipAddressText.text)
                        }

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }

                        Keys.onPressed: {
                            if (event.key === 16777220) {
                                console.info(ipAddressText.text)
                            }
                        }

                        //Settings {
                        //    property alias ipAddressText: ipAddressText.text
                        //}
                    }

                    Rectangle {
                        visible: LinkType == 2
                        color: theme.controlBackColor
                        height: parent.height
                        width: 2
                        border.width: 2
                        border.color: theme.controlBorderColor
                        radius: 0
                    }

                    CText {
                        visible: LinkType == 2
                        small: true
                        leftPadding: 4
                        rightPadding: 0
                        text: qsTr("src:")
                    }

                    CTextField {
                        id: ipPortText
                        visible: LinkType == 2
                        hoverEnabled: true
                        Layout.fillWidth: false
                        implicitWidth: {
                            if (Qt.platform.os == "android") {
                                return 100;
                            }
                            else {
                                return 60;
                            }
                        }

                        leftPadding: 2
                        rightPadding: 2


                        text: SourcePort
                        placeholderText: qsTr("")

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }

                        onTextEdited: {
                            linkManagerWrapper.sendUpdateSourcePort(Uuid, ipPortText.text)
                        }
                        //Settings {
                        //    property alias ipPortText: ipPortText.text
                        //}
                    }

                    Rectangle {
                        visible: LinkType == 2
                        color: theme.controlBackColor
                        height: parent.height
                        width: 2
                        border.width: 2
                        border.color: theme.controlBorderColor
                        radius: 0
                    }

                    CText {
                        visible: LinkType == 2
                        small: true
                        leftPadding: 4
                        rightPadding: 0
                        text: qsTr("dst:")
                    }

                    CTextField {
                        id: ipPort2Text
                        visible: LinkType == 2
                        hoverEnabled: true
                        Layout.fillWidth: false
                        implicitWidth: {
                            if (Qt.platform.os == "android") {
                                return 100;
                            }
                            else {
                                return 60;
                            }
                        }
                        leftPadding: 2
                        rightPadding: 2

                        text: DestinationPort
                        placeholderText: qsTr("")

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }

                        onTextEdited: {
                            linkManagerWrapper.sendUpdateDestinationPort(Uuid, ipPort2Text.text)
                        }

                        //Settings {
                        //    property alias ipPortText: ipPort2Text.text
                        //}
                    }

                    CButton {
                        Layout.alignment: Qt.AlignRight
                        text: ConnectionStatus ? qsTr("Close") : qsTr("Open")
                        backColor: ConnectionStatus ? "green" : theme.controlSolidBackColor
                        borderRadius: 2

                        onClicked: {
                            if (ConnectionStatus) {
                                linkManagerWrapper.closeLink(Uuid)
                            }
                            else {
                                switch(LinkType) {
                                case 1:
                                    core.closeLogFile();
                                    linkManagerWrapper.openAsSerial(Uuid)
                                    break
                                case 2:
                                    core.closeLogFile();
                                    linkManagerWrapper.openAsUdp(Uuid, ipAddressText.text, Number(ipPortText.text), Number(ipPort2Text.text))
                                    break
                                case 3:
                                    core.closeLogFile();
                                    linkManagerWrapper.openAsTcp(Uuid)
                                    break
                                default:
                                    console.log("Undefined type")
                                    break
                                }

                            }
                        }
                    }
                }
            }
        }

        ListView {
            id: filesList
            model: linkManagerWrapper.linkListModel
            visible: count > 0
            Layout.margins: 0
            Layout.topMargin: 0
            Layout.bottomMargin: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            height: count*theme.controlHeight
            Layout.preferredHeight: count*(theme.controlHeight+4)
            Layout.maximumHeight: 10*(theme.controlHeight+4)
            delegate: fileItem
            focus: true

            onCountChanged: {
                // qDebug("sasa");
                // console.log(filesList.count)
                Qt.callLater( positionViewAtEnd )
            }

            //                flickableDirection: Flickable.AutoFlickDirection

            //                onCurrentIndexChanged: {
            //                    console.log(filesList.currentIndex);
            //                }

            //                highlight: Rectangle { color: "lightsteelblue"; radius: 5 }

            //                onCountChanged: {
            //                    if(consScrollEnable.checked) {
            //                        Qt.callLater( positionViewAtEnd )
            //                    }
            //                }

            ScrollBar.vertical: ScrollBar { }

        }

    }

    MenuRow {
        // Layout.margins: 0
        Layout.topMargin: 0
        Layout.fillWidth: false

        CButton {
            text: qsTr("+UDP")
            Layout.fillWidth: false

            onClicked: {
                linkManagerWrapper.createAsUdp("", 0, 0)
            }
        }

        CButton {
            id: mavlinkProxy
            text: qsTr("MAVProxy")
            Layout.fillWidth: false
            checkable: true

            onToggled : {
                console.info("1")
                if (checked) {

                    console.info("2")
                    linkManagerWrapper.sendCreateAndOpenAsUdpProxy("127.0.0.1", 14551, 14550)
                }
                else {

                    console.info("3")
                    linkManagerWrapper.sendCloseUdpProxy()
                }
            }
        }

        CheckButton {
            id: loggingCheck
            text: qsTr("REC KLF")
            checkedColor: "red"
            color: "red"

            Layout.alignment: Qt.AlignRight

            onCheckedChanged: core.loggingKlf = loggingCheck.checked
            Component.onCompleted: core.loggingKlf = loggingCheck.checked

            Settings {
                property alias loggingCheck: loggingCheck.checked
            }

            icon.source: checked ? "./icons/record_fill.svg": "./icons/record.svg"

            // ToolTip.visible: hovered
            // ToolTip.text: "Recording"
        }

        CheckButton {
            id: loggingCheck2
            text: qsTr("REC CSV")
            checkedColor: "red"
            color: "red"

            Layout.alignment: Qt.AlignRight

            onCheckedChanged: core.loggingCsv = loggingCheck2.checked
            Component.onCompleted: core.loggingCsv = loggingCheck2.checked

            Settings {
                property alias loggingCheck2: loggingCheck2.checked
            }

            icon.source: checked ? "./icons/record_fill.svg": "./icons/record.svg"

            // ToolTip.visible: hovered
            // ToolTip.text: "Recording"
        }
    }

    MenuRow {
        spacing: 4
        CheckButton {
            id: zeroingPosButton
            icon.source: "./icons/propeller-off.svg"
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight
            visible: theme.isFakeCoords

            onCheckedChanged: {
                if (theme.isFakeCoords) {
                    core.setPosZeroing(checked);
                }
            }

            Component.onCompleted: { // maybe need del
                if (theme.isFakeCoords) {
                    core.setPosZeroing(checked);
                }
            }

            Settings {
                property alias zeroingPosButtonCheched: zeroingPosButton.checked
            }
        }

        CTextField {
            id: pathText
            hoverEnabled: true
            Layout.fillWidth: true

            text: core.filePath
            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    core.openLogFile(pathText.text, false, false);
                }
            }

            Settings {
                property alias pathText: pathText.text
            }
        }

        CheckButton {
            icon.source: "./icons/file.svg"
            checkable: false
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight

            onClicked: {
                newFileDialog.open()
            }

            FileDialog {
                id: newFileDialog
                title: qsTr("Please choose a file")
                folder: shortcuts.home

                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]

                onAccepted: {
                    pathText.text = newFileDialog.fileUrl.toString().replace("file:///", Qt.platform.os === "windows" ? "" : "/")

                    var name_parts = newFileDialog.fileUrl.toString().split('.')

                    core.openLogFile(pathText.text, false, false);
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: newFileDialog.folder
            }
        }

        CheckButton {
            icon.source: "./icons/file-plus.svg"
            checkable: false
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight

            onClicked: {
                appendFileDialog.open()
            }

            FileDialog {
                id: appendFileDialog
                title: qsTr("Please choose a file")
                folder: shortcuts.home

                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]

                onAccepted: {
                    pathText.text = newFileDialog.fileUrl.toString().replace("file:///", Qt.platform.os === "windows" ? "" : "/")

                    var name_parts = appendFileDialog.fileUrl.toString().split('.')

                    //deviceManagerWrapper.sendOpenFile(pathText.text, true)
                    core.openLogFile(pathText.text, true, false);
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: appendFileDialog.folder
            }
        }

        CheckButton {
            icon.source: "./icons/file-off.svg"
            checkable: false
            backColor: theme.controlSolidBackColor
            borderWidth: 0
            implicitWidth: theme.controlHeight

            onClicked: {
                core.closeLogFile();
            }
        }
    }

    MenuRow {
        visible: devList.length > 0
        spacing: 10

        Repeater {
            model: devList
            delegate: CButton {
                text: modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]"
                Layout.fillWidth: true
                opacity: dev === modelData ? 1 : 0.5
                visible: modelData.devType === 0 ? false : true

                onClicked: {
                    dev = modelData
                }
            }
        }
    }
}

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

    Connections {
        target: core

        function onConnectionChanged() {
            connectionButton.connection = core.isOpenConnection()
            dev = null
        }
    }

    onDevListChanged: {
        devTab0.visible = deviceManagerWrapper.isCreatedId(0)
        if(deviceManagerWrapper.isCreatedId(0)) { dev = devList[0] }
        devTab1.visible = deviceManagerWrapper.isCreatedId(1)
        devTab2.visible = deviceManagerWrapper.isCreatedId(2)
    }

    // MenuBlock {
    // }

    // ColumnLayout {
    //     id: columnConnectionItem
    //     anchors.fill: parent
    //     // Layout.fillWidth: true
    //     // width: parent.width
    //     // Layout.margins: 10
    //     spacing: 0

    // MenuRow {
    //     CButton {
    //         id: typeSerialTab
    //         Layout.fillWidth: true
    //         Layout.preferredWidth: parent.width/typeConnectionButtonGroup.buttons.length
    //         checkable: true
    //         checked: true
    //         ButtonGroup.group: typeConnectionButtonGroup
    //         text: "Serial"
    //         onClicked: {
    //             linkManagerWrapper.addLink()
    //         }
    //     }

    //     CButton {
    //         id: typeIpTab
    //         Layout.fillWidth: true
    //         Layout.preferredWidth: parent.width/typeConnectionButtonGroup.buttons.length
    //         checkable: true
    //         ButtonGroup.group: typeConnectionButtonGroup
    //         text: "IP"
    //     }

    //     CButton {
    //         id: typeFileTab
    //         Layout.fillWidth: true
    //         Layout.preferredWidth: parent.width/typeConnectionButtonGroup.buttons.length
    //         checkable: true
    //         ButtonGroup.group: typeConnectionButtonGroup
    //         text: "File"
    //     }


    //     ButtonGroup {
    //         property bool buttonChangeFlag : false
    //         id: typeConnectionButtonGroup
    //         onCheckedButtonChanged: buttonChangeFlag = true
    //         onClicked: {
    //             if(!buttonChangeFlag)
    //                 checkedButton = null

    //             buttonChangeFlag = false;
    //         }
    //     }
    // }

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

                        ToolTip.visible: hovered
                        ToolTip.text: "Settings"
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/pin.svg"
                        checked: IsPinned

                        onToggled: {
                            linkManagerWrapper.sendUpdatePinnedState(Uuid, checked)
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: checked ? "Unpin" : "Pin"
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/repeat.svg"
                        checked: ControlType
                        // text: "Auto"

                        onToggled: {
                            linkManagerWrapper.sendUpdateControlType(Uuid, Number(checked))
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: "Auto"
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked && LinkType == 2
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "./icons/x.svg"
                        checked: false

                        onToggled: {
                            if(checked) {
                                linkManagerWrapper.deleteLink(Uuid)
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: "Delete"
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
                        text: "ip:"
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
                        text: "src:"
                    }

                    CTextField {
                        id: ipPortText
                        visible: LinkType == 2
                        hoverEnabled: true
                        Layout.fillWidth: false
                        implicitWidth: 60

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
                        text: "dst:"
                    }

                    CTextField {
                        id: ipPort2Text
                        visible: LinkType == 2
                        hoverEnabled: true
                        Layout.fillWidth: false
                        implicitWidth: 60
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
                        text: ConnectionStatus ? "Close" : "Open"
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
            text: "+UDP"
            Layout.fillWidth: false

            onClicked: {
                linkManagerWrapper.createAsUdp("", 0, 0)
            }
        }



        CButton {
            id: mavlinkProxy
            text: "MAVProxy"
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
            text: "REC"
            checkedColor: "red"
            color: "red"

            Layout.alignment: Qt.AlignRight

            onCheckedChanged: core.logging = loggingCheck.checked
            Component.onCompleted: core.logging = loggingCheck.checked

            Settings {
                property alias loggingCheck: loggingCheck.checked
            }

            icon.source: checked ? "./icons/record_fill.svg": "./icons/record.svg"

            // ToolTip.visible: hovered
            // ToolTip.text: "Recording"
        }
    }
    /*
    MenuRow {
        CText {
            Layout.fillWidth: true
            text: "FileReader:"
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: core.fileReaderProgress
        }

        CButton {
            text: "start"
            Layout.fillWidth: true
            onClicked: {
                //core.startFileReader(pathText.text)
                deviceManagerWrapper.sendOpenFile(pathText.text)
            }
        }

        CButton {
            text: "stop"
            Layout.fillWidth: true
            onClicked: {
                core.stopFileReader()
            }
        }
    }
    */

    MenuRow {
        spacing: 4
        CTextField {
            id: pathText
            hoverEnabled: true
            Layout.fillWidth: true

            text: ""
            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    core.openLogFile(pathText.text, false);
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

            onClicked: {
                newFileDialog.open()
            }

            FileDialog {
                id: newFileDialog
                title: "Please choose a file"
                folder: shortcuts.home

                nameFilters: ["Logs (*.klf *.ubx *.xtf)", "Kogger log files (*.klf)", "U-blox (*.ubx)"]

                onAccepted: {
                    pathText.text = newFileDialog.fileUrl.toString()

                    var name_parts = newFileDialog.fileUrl.toString().split('.')

                    core.openLogFile(pathText.text, false);
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

            onClicked: {
                appendFileDialog.open()
            }

            FileDialog {
                id: appendFileDialog
                title: "Please choose a file"
                folder: shortcuts.home

                nameFilters: ["Logs (*.klf *.ubx *.xtf)", "Kogger log files (*.klf)", "U-blox (*.ubx)"]

                onAccepted: {
                    pathText.text = appendFileDialog.fileUrl.toString()

                    var name_parts = appendFileDialog.fileUrl.toString().split('.')

                    //deviceManagerWrapper.sendOpenFile(pathText.text, true)
                    core.openLogFile(pathText.text, true);
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

            onClicked: {
                core.closeLogFile();
            }
        }
    }


    // MenuRow {
    //     CText {
    //         text: "Options:"
    //     }

    //     // CCheck {
    //     //     id: loggingCheck
    //     //     visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"
    //     //     text: "Logging"
    //     //     checked: false

    //     //     onCheckedChanged: core.logging = loggingCheck.checked
    //     //     Component.onCompleted: core.logging = loggingCheck.checked

    //     //     Settings {
    //     //         property alias loggingCheck: loggingCheck.checked
    //     //     }

    //     // }

    //     // CCheck {
    //     //     id: appendCheck
    //     //     // visible: connectionTypeCombo.currentText === "File"
    //     //     text: "Append"
    //     //     checked: false
    //     // }

    //     CCheck {
    //         id: proxyMenuCheck
    //         // visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"
    //         text: "Proxy"
    //         checked: false
    //     }

    //     CCheck {
    //         id: importCheck
    //         visible: connectionTypeCombo.currentText === "File"
    //         text: "Import"
    //         checked: false
    //     }
    // }

    // ParamGroup {
    //     groupName: "Proxy"
    //     visible: proxyMenuCheck.checked && (connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP")
    //     Layout.margins: 24

    //     RowLayout {
    //         Layout.fillWidth: true
    //         //                Layout.margins: 10
    //         spacing: 10

    //         CCheck {
    //             id: proxyCheck
    //             Layout.fillWidth: true
    //             text: "Kogger"
    //             checked: false

    //             onCheckedChanged: {
    //                 if(proxyCheck.checked) {
    //                     deviceManagerWrapper.openProxyLink(proxyAddressText.text, Number(proxyBindPortText.text), Number(proxyPortText.text))
    //                 } else {
    //                     deviceManagerWrapper.closeProxyLink()
    //                 }
    //             }
    //         }

    //         CTextField {
    //             id: proxyBindPortText
    //             hoverEnabled: true
    //             Layout.fillWidth: false
    //             implicitWidth: 92

    //             text: "14444"
    //             placeholderText: qsTr("Port")

    //             Settings {
    //                 property alias proxyBindPortText: proxyBindPortText.text
    //             }
    //         }

    //         CTextField {
    //             id: proxyAddressText
    //             hoverEnabled: true
    //             implicitWidth: 135

    //             text: "10.0.0.3"
    //             placeholderText: ""

    //             Keys.onPressed: {
    //                 if (event.key === 16777220) {
    //                     console.info(proxyAddressText.text)
    //                 }
    //             }

    //             Settings {
    //                 property alias proxyAddressText: proxyAddressText.text
    //             }
    //         }

    //         CTextField {
    //             id: proxyPortText
    //             hoverEnabled: true
    //             Layout.fillWidth: false
    //             implicitWidth: 92

    //             text: "14444"
    //             placeholderText: qsTr("Port")

    //             Settings {
    //                 property alias proxyPortText: proxyPortText.text
    //             }
    //         }

    //         CCombo  {
    //             visible: false
    //             model: ["UDP"]
    //         }
    //     }

    //     RowLayout {
    //         Layout.fillWidth: true
    //         //                Layout.margins: 10
    //         //                Layout.topMargin: -5
    //         spacing: 10

    //         CCheck {
    //             id: proxyNavCheck
    //             Layout.fillWidth: true
    //             text: "Nav"
    //             checked: false

    //             onCheckedChanged: {
    //                 if(proxyNavCheck.checked) {
    //                     deviceManagerWrapper.openProxyNavLink(proxyNavAddressText.text, Number(proxyNavBindPortText.text), Number(proxyNavPortText.text))
    //                 } else {
    //                     deviceManagerWrapper.closeProxyNavLink()
    //                 }
    //             }
    //         }

    //         CTextField {
    //             id: proxyNavBindPortText
    //             hoverEnabled: true
    //             Layout.fillWidth: false
    //             implicitWidth: 92

    //             text: "14551"
    //             placeholderText: qsTr("Port")

    //             Settings {
    //                 property alias proxyNavBindPortText: proxyNavBindPortText.text
    //             }
    //         }

    //         CTextField {
    //             id: proxyNavAddressText
    //             hoverEnabled: true
    //             //                    Layout.fillWidth: true
    //             implicitWidth: 135

    //             text: "10.0.0.3"
    //             placeholderText: ""

    //             Keys.onPressed: {
    //                 if (event.key === 16777220) {
    //                     console.info(proxyNavAddressText.text)
    //                 }
    //             }

    //             Settings {
    //                 property alias proxyNavAddressText: proxyNavAddressText.text
    //             }
    //         }

    //         CTextField {
    //             id: proxyNavPortText
    //             hoverEnabled: true
    //             Layout.fillWidth: false
    //             implicitWidth: 92

    //             text: "14550"
    //             placeholderText: qsTr("Port")

    //             Settings {
    //                 property alias proxyNavPortText: proxyNavPortText.text
    //             }
    //         }

    //         CCombo  {
    //             visible: false
    //             model: ["UDP"]
    //         }
    //     }

    // }

    // ParamGroup {
    //     groupName: "CSV import"
    //     visible: importCheck.checked
    //     Layout.margins: 24

    //     RowLayout {
    //         ParamSetup {
    //             paramName: "Separator: "
    //             Layout.fillWidth: true

    //             CCombo  {
    //                 id: separatorCombo
    //                 //                    Layout.fillWidth: true
    //                 model: ["Comma", "Tab", "Space", "SemiColon"]

    //                 Settings {
    //                     property alias separatorCombo: separatorCombo.currentIndex
    //                 }
    //             }
    //         }

    //         ParamSetup {
    //             paramName: "Row: "
    //             Layout.fillWidth: true

    //             SpinBoxCustom {
    //                 id:firstRow
    //                 implicitWidth: 100
    //                 from: 1
    //                 to: 100
    //                 stepSize: 1
    //                 value: 1

    //                 Settings {
    //                     property alias importCSVfirstRow: firstRow.value
    //                 }
    //             }
    //         }
    //     }


    //     RowLayout {
    //         CCheck {
    //             id: timeEnable
    //             //                    Layout.fillWidth: true
    //             //                        Layout.preferredWidth: 150
    //             checked: true
    //             text: "Time"

    //             Settings {
    //                 property alias importCSVtimeEnable: timeEnable.checked
    //             }
    //         }

    //         //                CTextField {
    //         //                    id: timeFormater
    //         //                    text: "yyyy-MM-dd hh:mm:ss,zzz"
    //         //                    Settings {
    //         //                        property alias importCSVtimeFormater: timeFormater.text
    //         //                    }
    //         //                }

    //         SpinBoxCustom {
    //             id:timeColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 6

    //             Settings {
    //                 property alias importCSVtimeColumn: timeColumn.value
    //             }
    //         }

    //         CCombo  {
    //             id: utcGpsCombo
    //             //                    Layout.fillWidth: true
    //             model: ["UTC time", "GPS time"]

    //             Settings {
    //                 property alias utcGpsCombo: utcGpsCombo.currentIndex
    //             }
    //         }
    //     }

    //     //            RowLayout {
    //     //                CCheck {
    //     //                    id: utcTime
    //     //                    Layout.fillWidth: true
    //     //                    checked: true
    //     //                    text: "UTC time"

    //     //                    Settings {
    //     //                        property alias utcTime: utcTime.checked
    //     //                    }
    //     //                }

    //     //            }


    //     RowLayout {
    //         CCheck {
    //             id: latLonEnable
    //             Layout.fillWidth: true
    //             //                        Layout.preferredWidth: 150
    //             checked: true
    //             text: "Lat/Lon/Alt"

    //             Settings {
    //                 property alias importCSVlatLonEnable: latLonEnable.checked
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: latColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 2

    //             Settings {
    //                 property alias importCSVlatColumn: latColumn.value
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: lonColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 3

    //             Settings {
    //                 property alias importCSVlonColumn: lonColumn.value
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: altColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 4

    //             Settings {
    //                 property alias importCSValtColumn: altColumn.value
    //             }
    //         }
    //     }

    //     RowLayout {
    //         CCheck {
    //             id: xyzEnable
    //             Layout.fillWidth: true
    //             //                        Layout.preferredWidth: 150
    //             checked: true
    //             text: "NEU"

    //             Settings {
    //                 property alias importCSVxyzEnable: xyzEnable.checked
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: northColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 2

    //             Settings {
    //                 property alias importCSVnorthColumn: northColumn.value
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: eastColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 3

    //             Settings {
    //                 property alias importCSVeastColumn: eastColumn.value
    //             }
    //         }

    //         SpinBoxCustom {
    //             id: upColumn
    //             implicitWidth: 100
    //             from: 1
    //             to: 100
    //             stepSize: 1
    //             value: 4

    //             Settings {
    //                 property alias importCSVupColumn: upColumn.value
    //             }
    //         }
    //     }

    //     RowLayout {
    //         CTextField {
    //             id: importPathText
    //             hoverEnabled: true
    //             Layout.fillWidth: true
    //             //                    visible: connectionTypeCombo.currentText === "File"

    //             text: ""
    //             placeholderText: qsTr("Enter path")

    //             Keys.onPressed: {
    //                 if (event.key === 16777220) {
    //                     importTrackFileDialog.openCSV();
    //                 }
    //             }

    //             Settings {
    //                 property alias importPathText: importPathText.text
    //             }
    //         }

    //         CButton {
    //             text: "..."
    //             Layout.fillWidth: false
    //             visible: connectionTypeCombo.currentText === "File"
    //             implicitHeight: theme.controlHeight
    //             implicitWidth: implicitHeight*1.1
    //             onClicked: {
    //                 importTrackFileDialog.open()
    //             }

    //             FileDialog {
    //                 id: importTrackFileDialog
    //                 title: "Please choose a file"
    //                 folder: shortcuts.home
    //                 //                    fileMode: FileDialog.OpenFiles

    //                 nameFilters: ["Logs (*.csv *.txt)"]

    //                 function openCSV() {
    //                     core.openCSV(importPathText.text, separatorCombo.currentIndex, firstRow.value, timeColumn.value, utcGpsCombo.currentIndex == 0,
    //                                  latColumn.value*latLonEnable.checked, lonColumn.value*latLonEnable.checked, altColumn.value*latLonEnable.checked,
    //                                  northColumn.value*xyzEnable.checked, eastColumn.value*xyzEnable.checked, upColumn.value*xyzEnable.checked);
    //                 }

    //                 onAccepted: {
    //                     importPathText.text = importTrackFileDialog.fileUrl.toString()

    //                     openCSV();
    //                 }
    //                 onRejected: {
    //                 }
    //             }

    //             Settings {
    //                 property alias importFolder: importTrackFileDialog.folder
    //             }
    //         }
    //     }


    // }

    //        devList[0].devName + " " + devList[0].fwVersion + " [" + devList[0].devSN + "]"

    // MenuRow {
    //     // visible: connectionTypeCombo.currentText === "File" && dev !== null

    //     ParamGroup {
    //         visible: dev ? dev.devName !== "" : false
    //         groupName: dev ? dev.devName : "undefined"

    //         // ParamSetup {
    //         //     paramName: "SN: " + (dev ? (dev.devSN ? dev.devSN : "undefined") : "undefined")
    //         // }

    //         ParamSetup {
    //             visible: dev ? dev.fwVersion !== "" : false
    //             paramName: "FW version: " + (dev ? (dev.fwVersion ? dev.fwVersion : "undefined") : "undefined")
    //         }
    //     }
    // }

    //        RowLayout {
    //            Layout.fillWidth: true
    //            ListView {
    //                height: 320
    //                width: 100
    //                Layout.fillWidth: true
    //                model: Qt.fontFamilies()

    //                delegate: Item {
    //                    height: 40;
    //                    width: ListView.view.width
    //                    Text {
    //                        anchors.centerIn: parent
    //                        text: modelData;
    //                        color: "#FFFFFF"
    //                    }
    //                }
    //            }
    //        }

    MenuRow {
        visible: devList.length > 0

        CButton {
            id: devTab0
            text: devList[0] ? devList[0].devName + " " + devList[0].fwVersion + " [" + devList[0].devSN + "]" : "undefined"
            Layout.fillWidth: true
            opacity: dev === devList[0] ? 1 : 0.5

            onClicked: {
                dev = devList[0]
            }
        }

        CButton {
            id: devTab1
            text: devList[1] ? devList[1].devName + " " + devList[1].fwVersion + " [" + devList[1].devSN + "]" : "undefined"
            Layout.fillWidth: true
            opacity: dev === devList[1] ? 1 : 0.5

            onClicked: {
                dev = devList[1]
            }
        }

        CButton {
            id: devTab2
            text: devList[2] ? devList[2].devName + " " + devList[2].fwVersion + " [" + devList[2].devSN + "]" : "undefined"
            Layout.fillWidth: true
            opacity: dev === devList[2] ? 1 : 0.5

            onClicked: {
                dev = devList[2]
            }
        }
    }
}

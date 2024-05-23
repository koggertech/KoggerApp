import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

ColumnLayout {
    property var dev: null
    property var devList: devs.devs

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
        devTab0.visible = devs.isCreatedId(0)
        if(devs.isCreatedId(0)) { dev = devList[0] }
        devTab1.visible = devs.isCreatedId(1)
        devTab2.visible = devs.isCreatedId(2)
    }

    MenuBlock {
    }

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
                    anchors.verticalCenter: parent
                    color: theme.controlBackColor
                    border.width: 1
                    border.color: theme.controlBorderColor
                    radius: 2
                }

                RowLayout {
                    spacing: 1
                    // Layout.fillWidth: true
                    anchors.fill: parent
                    // anchors.topMargin: 2
                    // anchors.bottomMargin: 2
                    anchors.verticalCenter: parent
                    anchors.margins: 1

                    CCheck {
                        width: 72
                        implicitWidth: 72
                        //readOnly: true
                        //selectByMouse: false
                        leftPadding: 6
                        rightPadding: 2
                        // textEdited: false
                        text: LinkType == 1 ? "COM" : LinkType == 2 ? "UDP" : LinkType == 2 ? "TCP" : "???"
                        checked: IsPinned
                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }

                        onToggled: {
                            linkManagerWrapper.sendUpdatePinnedState(Uuid, checked)
                        }
                    }

                    Rectangle {
                        color: theme.controlBackColor
                        height: parent.height
                        width: 2
                        border.width: 2
                        border.color: theme.controlBorderColor
                        radius: 0
                    }

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
                        id: baudrateCombo
                        implicitWidth: 150
                        // Layout.fillWidth: true
                        visible: LinkType == 1
                        model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000]
                        currentIndex: 7
                        displayText: Baudrate

                        onCurrentTextChanged: {
                            linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
                        }

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }

                        Component.onCompleted: {
                            linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
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
                                    linkManagerWrapper.openAsSerial(Uuid)
                                    break
                                case 2:
                                    linkManagerWrapper.openAsUdp(Uuid, ipAddressText.text, Number(ipPortText.text), Number(ipPort2Text.text))
                                    break
                                case 3:
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
        Layout.topMargin: 0
        Layout.fillWidth: false

        CButton {
            text: "+File"
            Layout.fillWidth: false

            onClicked: {
                // fileReaderWrapper
            }
        }

        CButton {
            text: "+WiFiBridge"
            Layout.fillWidth: false

            onClicked: {
                linkManagerWrapper.createAsUdp("", 0, 0)
            }
        }

        CButton {
            text: "+UDP"
            Layout.fillWidth: false

            onClicked: {
                linkManagerWrapper.createAsUdp("", 0, 0)
            }
        }
    }


//     MenuRow {
//         visible: typeSerialTab.checked
//         CCombo  {
//             id: baudrateCombo
//             Layout.fillWidth: true
//             // visible: connectionTypeCombo.currentText === "Serial"
//             model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000]
//             currentIndex: 4

// //                onCurrentTextChanged: {
// //                    if(connectionButton.connection) {
// //                        dev.baudrate = Number(baudrateCombo.currentText)
// //                        core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText), false)
// //                    }
// //                }

//             Settings {
//                 property alias serialBaudrate: baudrateCombo.currentIndex
//                 property alias serialBaudrateText: baudrateCombo.currentText
//             }
//         }

//         CCombo  {
//             id: portCombo
//             Layout.fillWidth: true
//             // visible: connectionTypeCombo.currentText === "Serial"
//             onPressedChanged: {
//                 if(pressed) {
//                     model = core.availableSerialName()
//                 }
//             }

//             Component.onCompleted: {
//                 model = core.availableSerialName()
//             }

//             Settings {
//                 property alias connectionPortText: portCombo.currentText
//             }
//         }
//     }

    MenuRow {
        // visible: typeFileTab.checked
        CTextField {
            id: pathText
            hoverEnabled: true
            Layout.fillWidth: true
            // visible: connectionTypeCombo.currentText === "File"

            text: ""
            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    core.openConnectionAsFile(1, pathText.text, appendCheck.checked);
                }
            }

            Settings {
                property alias pathText: pathText.text
            }
        }

        CButton {
            visible: typeFileTab.checked
            text: "Open new"
            Layout.fillWidth: true
            onClicked: {
                logFileDialog.open()
            }

            FileDialog {
                id: logFileDialog
                title: "Please choose a file"
                folder: shortcuts.home

                nameFilters: ["Logs (*.klf *.ubx *.xtf)", "Kogger log files (*.klf)", "U-blox (*.ubx)"]

                onAccepted: {
                    pathText.text = logFileDialog.fileUrl.toString()

                    var name_parts = logFileDialog.fileUrl.toString().split('.')

                    core.openConnectionAsFile(1, pathText.text, appendCheck.checked);
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: logFileDialog.folder
            }
        }
    }

    // MenuRow {
    //     visible: typeIpTab.checked
    //     CCombo  {
    //         id: ipTypeCombo
    //         Layout.fillWidth: true
    //         // visible: connectionTypeCombo.currentText === "IP"
    //         model: ["UDP", "TCP"]


    //         Settings {
    //             property alias ipTypeCombo: ipTypeCombo.currentIndex
    //         }
    //     }

    //     CTextField {
    //         id: ipAddressText
    //         hoverEnabled: true
    //         Layout.fillWidth: true
    //         // visible: connectionTypeCombo.currentText === "IP"

    //         text: "192.168.4.1"
    //         placeholderText: ""

    //         Keys.onPressed: {
    //             if (event.key === 16777220) {
    //                 console.info(ipAddressText.text)
    //             }
    //         }

    //         Settings {
    //             property alias ipAddressText: ipAddressText.text
    //         }
    //     }

    //     CTextField {
    //         id: ipPortText
    //         hoverEnabled: true
    //         Layout.fillWidth: false
    //         implicitWidth: 80
    //         // visible: connectionTypeCombo.currentText === "IP"

    //         text: "14444"
    //         placeholderText: qsTr("Port")

    //         Settings {
    //             property alias ipPortText: ipPortText.text
    //         }
    //     }
    // }

    // MenuRow {
    //     CButton {
    //         visible: !typeFileTab.checked
    //         id: openNewConnectionButton
    //         Layout.fillWidth: true
    //         checkable: false
    //         text: "Open Port"

    //         function openConnection() {
    //             if(typeSerialTab.checked) {
    //                 core.openConnectionAsSerial(1, autoconnectionCheck.checked, portCombo.currentText, Number(baudrateCombo.currentText), false)
    //             }

    //             if(typeIpTab.checked) {
    //                 core.openConnectionAsIP(1, autoconnectionCheck.checked, ipAddressText.text, Number(ipPortText.text), ipTypeCombo.currentText === "TCP");
    //             }
    //         }

    //         onClicked: {
    //             openNewConnectionButton.openConnection()
    //         }
    //     }

    //     CButton {
    //         visible: typeFileTab.checked
    //         text: "Open File"
    //         Layout.fillWidth: true
    //         onClicked: {
    //             logFileDialog.open()
    //         }

    //         FileDialog {
    //             id: logFileDialog
    //             title: "Please choose a file"
    //             folder: shortcuts.home

    //             nameFilters: ["Logs (*.klf *.ubx *.xtf)", "Kogger log files (*.klf)", "U-blox (*.ubx)"]

    //             onAccepted: {
    //                 pathText.text = logFileDialog.fileUrl.toString()

    //                 var name_parts = logFileDialog.fileUrl.toString().split('.')

    //                 core.openConnectionAsFile(1, pathText.text, appendCheck.checked);
    //             }
    //             onRejected: {
    //             }
    //         }

    //         Settings {
    //             property alias logFolder: logFileDialog.folder
    //         }
    //     }
    // }


        RowLayout {
            // Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            width: parent.width

            Layout.margins: 10
            Layout.topMargin: -5
            spacing: 5
            visible: true

            CText {
                text: "Options:"
            }

            CCheck {
                id: loggingCheck
                visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"
                text: "Logging"
                checked: false

                onCheckedChanged: core.logging = loggingCheck.checked
                Component.onCompleted: core.logging = loggingCheck.checked

                Settings {
                    property alias loggingCheck: loggingCheck.checked
                }

            }

            CCheck {
                id: autoconnectionCheck
                visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"
                text: "Auto"
                checked: false

                onCheckedChanged: {
                    if(checked) {
                        connectionButton.openConnection()
                    }
                }

                Component.onCompleted: {
                    if(checked) {
                        connectionButton.openConnection()
                    }
                }

                Settings {
                    property alias autoconnectionCheck: autoconnectionCheck.checked
                }
            }

            CCheck {
                id: appendCheck
                visible: connectionTypeCombo.currentText === "File"
                text: "Append"
                checked: false
            }

            CCheck {
                id: proxyMenuCheck
                visible: connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP"
                text: "Proxy"
                checked: false
            }

            CCheck {
                id: importCheck
                visible: connectionTypeCombo.currentText === "File"
                text: "Import"
                checked: false
            }


        }

        ParamGroup {
            groupName: "Proxy"
            visible: proxyMenuCheck.checked && (connectionTypeCombo.currentText === "Serial" || connectionTypeCombo.currentText === "IP")
            Layout.margins: 24

            RowLayout {
                Layout.fillWidth: true
//                Layout.margins: 10
                spacing: 10

                CCheck {
                    id: proxyCheck
                    Layout.fillWidth: true
                    text: "Kogger"
                    checked: false

                    onCheckedChanged: {
                        if(proxyCheck.checked) {
                            devs.openProxyLink(proxyAddressText.text, Number(proxyBindPortText.text), Number(proxyPortText.text))
                        } else {
                            devs.closeProxyLink()
                        }
                    }
                }

                CTextField {
                    id: proxyBindPortText
                    hoverEnabled: true
                    Layout.fillWidth: false
                    implicitWidth: 92

                    text: "14444"
                    placeholderText: qsTr("Port")

                    Settings {
                        property alias proxyBindPortText: proxyBindPortText.text
                    }
                }

                CTextField {
                    id: proxyAddressText
                    hoverEnabled: true
                    implicitWidth: 135

                    text: "10.0.0.3"
                    placeholderText: ""

                    Keys.onPressed: {
                        if (event.key === 16777220) {
                            console.info(proxyAddressText.text)
                        }
                    }

                    Settings {
                        property alias proxyAddressText: proxyAddressText.text
                    }
                }

                CTextField {
                    id: proxyPortText
                    hoverEnabled: true
                    Layout.fillWidth: false
                    implicitWidth: 92

                    text: "14444"
                    placeholderText: qsTr("Port")

                    Settings {
                        property alias proxyPortText: proxyPortText.text
                    }
                }

                CCombo  {
                    visible: false
                    model: ["UDP"]
                }
            }

            RowLayout {
                Layout.fillWidth: true
//                Layout.margins: 10
//                Layout.topMargin: -5
                spacing: 10

                CCheck {
                    id: proxyNavCheck
                    Layout.fillWidth: true
                    text: "Nav"
                    checked: false

                    onCheckedChanged: {
                        if(proxyNavCheck.checked) {
                            devs.openProxyNavLink(proxyNavAddressText.text, Number(proxyNavBindPortText.text), Number(proxyNavPortText.text))
                        } else {
                            devs.closeProxyNavLink()
                        }
                    }
                }

                CTextField {
                    id: proxyNavBindPortText
                    hoverEnabled: true
                    Layout.fillWidth: false
                    implicitWidth: 92

                    text: "14551"
                    placeholderText: qsTr("Port")

                    Settings {
                        property alias proxyNavBindPortText: proxyNavBindPortText.text
                    }
                }

                CTextField {
                    id: proxyNavAddressText
                    hoverEnabled: true
//                    Layout.fillWidth: true
                    implicitWidth: 135

                    text: "10.0.0.3"
                    placeholderText: ""

                    Keys.onPressed: {
                        if (event.key === 16777220) {
                            console.info(proxyNavAddressText.text)
                        }
                    }

                    Settings {
                        property alias proxyNavAddressText: proxyNavAddressText.text
                    }
                }

                CTextField {
                    id: proxyNavPortText
                    hoverEnabled: true
                    Layout.fillWidth: false
                    implicitWidth: 92

                    text: "14550"
                    placeholderText: qsTr("Port")

                    Settings {
                        property alias proxyNavPortText: proxyNavPortText.text
                    }
                }

                CCombo  {
                    visible: false
                    model: ["UDP"]
                }
            }

        }

        ParamGroup {
            groupName: "CSV import"
            visible: importCheck.checked
            Layout.margins: 24

            RowLayout {
                ParamSetup {
                    paramName: "Separator: "
                    Layout.fillWidth: true

                    CCombo  {
                        id: separatorCombo
    //                    Layout.fillWidth: true
                        model: ["Comma", "Tab", "Space", "SemiColon"]

                        Settings {
                            property alias separatorCombo: separatorCombo.currentIndex
                        }
                    }
                }

                ParamSetup {
                    paramName: "Row: "
                    Layout.fillWidth: true

                    SpinBoxCustom {
                        id:firstRow
                        implicitWidth: 100
                        from: 1
                        to: 100
                        stepSize: 1
                        value: 1

                        Settings {
                            property alias importCSVfirstRow: firstRow.value
                        }
                    }
                }
            }


            RowLayout {
                CCheck {
                    id: timeEnable
//                    Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    checked: true
                    text: "Time"

                    Settings {
                        property alias importCSVtimeEnable: timeEnable.checked
                    }
                }

//                CTextField {
//                    id: timeFormater
//                    text: "yyyy-MM-dd hh:mm:ss,zzz"
//                    Settings {
//                        property alias importCSVtimeFormater: timeFormater.text
//                    }
//                }

                SpinBoxCustom {
                    id:timeColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 6

                    Settings {
                        property alias importCSVtimeColumn: timeColumn.value
                    }
                }

                CCombo  {
                    id: utcGpsCombo
//                    Layout.fillWidth: true
                    model: ["UTC time", "GPS time"]

                    Settings {
                        property alias utcGpsCombo: utcGpsCombo.currentIndex
                    }
                }
            }

//            RowLayout {
//                CCheck {
//                    id: utcTime
//                    Layout.fillWidth: true
//                    checked: true
//                    text: "UTC time"

//                    Settings {
//                        property alias utcTime: utcTime.checked
//                    }
//                }

//            }


            RowLayout {
                CCheck {
                    id: latLonEnable
                    Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    checked: true
                    text: "Lat/Lon/Alt"

                    Settings {
                        property alias importCSVlatLonEnable: latLonEnable.checked
                    }
                }

                SpinBoxCustom {
                    id: latColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 2

                    Settings {
                        property alias importCSVlatColumn: latColumn.value
                    }
                }

                SpinBoxCustom {
                    id: lonColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 3

                    Settings {
                        property alias importCSVlonColumn: lonColumn.value
                    }
                }

                SpinBoxCustom {
                    id: altColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 4

                    Settings {
                        property alias importCSValtColumn: altColumn.value
                    }
                }
            }



            RowLayout {
                CCheck {
                    id: xyzEnable
                    Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    checked: true
                    text: "NEU"

                    Settings {
                        property alias importCSVxyzEnable: xyzEnable.checked
                    }
                }

                SpinBoxCustom {
                    id: northColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 2

                    Settings {
                        property alias importCSVnorthColumn: northColumn.value
                    }
                }

                SpinBoxCustom {
                    id: eastColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 3

                    Settings {
                        property alias importCSVeastColumn: eastColumn.value
                    }
                }

                SpinBoxCustom {
                    id: upColumn
                    implicitWidth: 100
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 4

                    Settings {
                        property alias importCSVupColumn: upColumn.value
                    }
                }
            }



            RowLayout {
                CTextField {
                    id: importPathText
                    hoverEnabled: true
                    Layout.fillWidth: true
//                    visible: connectionTypeCombo.currentText === "File"

                    text: ""
                    placeholderText: qsTr("Enter path")

                    Keys.onPressed: {
                        if (event.key === 16777220) {
                            importTrackFileDialog.openCSV();
                        }
                    }

                    Settings {
                        property alias importPathText: importPathText.text
                    }
                }

                CButton {
                    text: "..."
                    Layout.fillWidth: false
                    visible: connectionTypeCombo.currentText === "File"
                    implicitHeight: theme.controlHeight
                    implicitWidth: implicitHeight*1.1
                    onClicked: {
                        importTrackFileDialog.open()
                    }

                    FileDialog {
                        id: importTrackFileDialog
                        title: "Please choose a file"
                        folder: shortcuts.home
                        //                    fileMode: FileDialog.OpenFiles

                        nameFilters: ["Logs (*.csv *.txt)"]

                        function openCSV() {
                            core.openCSV(importPathText.text, separatorCombo.currentIndex, firstRow.value, timeColumn.value, utcGpsCombo.currentIndex == 0,
                                         latColumn.value*latLonEnable.checked, lonColumn.value*latLonEnable.checked, altColumn.value*latLonEnable.checked,
                                         northColumn.value*xyzEnable.checked, eastColumn.value*xyzEnable.checked, upColumn.value*xyzEnable.checked);
                        }

                        onAccepted: {
                            importPathText.text = importTrackFileDialog.fileUrl.toString()

                            openCSV();
                        }
                        onRejected: {
                        }
                    }

                    Settings {
                        property alias importFolder: importTrackFileDialog.folder
                    }
                }
            }


        }

//        devList[0].devName + " " + devList[0].fwVersion + " [" + devList[0].devSN + "]"

        ColumnLayout {
            spacing: 24
            Layout.margins: 24
            visible: connectionTypeCombo.currentText === "File" && dev !== null

            ParamGroup {
                visible: dev ? dev.devName !== "" : false
                groupName: dev ? dev.devName : "undefined"

                ParamSetup {
                    paramName: "SN: " + (dev ? (dev.devSN ? dev.devSN : "undefined") : "undefined")
                }

                ParamSetup {
                    visible: dev ? dev.fwVersion !== "" : false
                    paramName: "FW version: " + (dev ? (dev.fwVersion ? dev.fwVersion : "undefined") : "undefined")
                }
            }
        }

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

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10
            visible: connectionButton.connection

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
    // }
}

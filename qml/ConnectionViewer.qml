import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtCore


ColumnLayout {
    property var dev: null
    property var devList: deviceManagerWrapper.devs
    property string filePath: pathText.text

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
                    color: ConnectionStatus ? (ReceivesData ? "#005000" : "#505000") : IsNotAvailable ? "#500000" : theme.controlBackColor
                    border.width: 0
                    border.color: theme.controlBorderColor
                    radius: 2
                }

                RowLayout {
                    spacing: 1
                    anchors.fill: parent
                    anchors.margins: 1
                    enabled: !IsUpgradingState

                    CheckButton {
                        id: linkSettingsButton
                        width: theme.controlHeight
                        height: theme.controlHeight
                        icon.source: "qrc:/icons/ui/settings.svg"
                        borderWidth: 0
                        implicitWidth: theme.controlHeight

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Settings")
                    }

                    CheckButton {
                        visible: linkSettingsButton.checked
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "qrc:/icons/ui/pin.svg"
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
                        icon.source: "qrc:/icons/ui/repeat.svg"
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
                        visible: linkSettingsButton.checked && (LinkType === 2 || LinkType === 3)
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "qrc:/icons/ui/x.svg"
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
                        id: baudrateCombo
                        implicitWidth: 150
                        visible: LinkType == 1
                        model: linkManagerWrapper.baudrateModel
                        currentIndex: 8
                        displayText: Baudrate

                        onActivated: {
                            linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
                            autoSpeedCheckBox.checked = false
                        }

                        background:  Rectangle {
                            color: "transparent"
                            border.width: 0
                            border.color: theme.controlBorderColor
                        }
                    }

                    CheckButton {
                        id: autoSpeedCheckBox
                        visible: LinkType == 1
                        icon.source: "qrc:/icons/ui/refresh.svg"
                        implicitWidth: theme.controlHeight

                        checked: AutoSpeedSelection

                        onCheckedChanged: {
                            if (!checked) {
                                linkManagerWrapper.sendAutoSpeedSelection(Uuid, false)
                            }
                        }

                        onToggled: {
                            linkManagerWrapper.sendAutoSpeedSelection(Uuid, checked)
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Auto search baudrate")
                    }

                    CText {
                        visible: LinkType === 2 || LinkType === 3
                        small: true
                        leftPadding: 6
                        rightPadding: 0
                        text: LinkType === 2 ? qsTr("UDP ip:") : qsTr("TCP ip:")
                    }

                    CTextField {
                        id: ipAddressText
                        visible: LinkType === 2 || LinkType === 3
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

                        Keys.onPressed: function(event) {
                            if (event.key === 16777220) {
                                console.info(ipAddressText.text)
                            }
                        }

                        //Settings {
                        //    property alias ipAddressText: ipAddressText.text
                        //}
                    }

                    Rectangle {
                        visible: LinkType === 2
                        color: theme.controlBackColor
                        height: parent.height
                        width: 2
                        border.width: 2
                        border.color: theme.controlBorderColor
                        radius: 0
                    }

                    CText {
                        visible: LinkType === 2
                        small: true
                        leftPadding: 4
                        rightPadding: 0
                        text: qsTr("src:")
                    }

                    CTextField {
                        id: ipPortText
                        visible: LinkType === 2
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
                        visible: LinkType === 2 || LinkType === 3
                        color: theme.controlBackColor
                        height: parent.height
                        width: 2
                        border.width: 2
                        border.color: theme.controlBorderColor
                        radius: 0
                    }

                    CText {
                        visible: LinkType === 2 || LinkType === 3
                        small: true
                        leftPadding: 4
                        rightPadding: 0
                        text: LinkType === 2 ? qsTr("dst:") : qsTr("srv:")
                    }

                    CTextField {
                        id: ipPort2Text
                        visible: LinkType === 2 || LinkType === 3
                        hoverEnabled: true
                        Layout.fillWidth: false
                        implicitWidth: {
                            if (Qt.platform.os === "android") {
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
                        backColor: ConnectionStatus ? (ReceivesData ? "green" : "#adad00") : theme.controlSolidBackColor
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
                                    linkManagerWrapper.openAsTcp(Uuid, ipAddressText.text, Number(ipPortText.text), Number(ipPort2Text.text))
                                    break
                                default:
                                    console.log("Undefined type")
                                    break
                                }

                            }
                        }
                    }
                }

                Rectangle {
                    id: firmwareBackground
                    anchors.fill: parent
                    visible: IsUpgradingState
                    color: "#30ffffff"
                    z: 10

                    Image {
                        anchors.fill: parent
                        source: "qrc:/icons/ui/diagonal_stripe.png"
                        fillMode: Image.Tile
                        opacity: 0.5
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
            text: qsTr("+TCP")
            Layout.fillWidth: false

            onClicked: {
                linkManagerWrapper.createAsTcp("", 0, 0)
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
            text: qsTr("KLF")
            checkedColor: "red"
            color: "red"

            Layout.alignment: Qt.AlignRight

            onCheckedChanged: core.loggingKlf = loggingCheck.checked
            Component.onCompleted: core.loggingKlf = loggingCheck.checked

            Settings {
                property alias loggingCheck: loggingCheck.checked
            }

            icon.source: checked ? "qrc:/icons/ui/record_fill.svg": "qrc:/icons/ui/record.svg"

            // ToolTip.visible: hovered
            // ToolTip.text: "Recording"
        }

        CheckButton {
            id: loggingCheck2
            text: qsTr("CSV")
            checkedColor: "red"
            color: "red"

            Layout.alignment: Qt.AlignRight

            onCheckedChanged: core.loggingCsv = loggingCheck2.checked
            Component.onCompleted: core.loggingCsv = loggingCheck2.checked

            Settings {
                property alias loggingCheck2: loggingCheck2.checked
            }

            icon.source: checked ? "qrc:/icons/ui/record_fill.svg": "qrc:/icons/ui/record.svg"
        }

        CheckButton {
           id: importCheck
           text: "Import"
           checked: false
        }

        CheckButton {
            visible: false
            id: gpsCheckButton
            text: qsTr("GPS")
            checkedBackColor: core.isGPSAlive ? "green" : "red"

            Layout.alignment: Qt.AlignRight
            onCheckedChanged: {

                core.useGPS = checked
            }

            Component.onCompleted: {
                core.useGPS = checked
            }

            Settings {
                property alias gpsCheckButton: gpsCheckButton.checked
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

                Keys.onPressed: function(event) {
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
                //visible: true // connectionTypeCombo.currentText === "File"
                implicitHeight: theme.controlHeight
                implicitWidth: implicitHeight*1.1
                onClicked: {
                    importTrackFileDialog.open()
                }

                FileDialog {
                    id: importTrackFileDialog
                    title: "Please choose a file"
                    currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
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
                    property alias importFolder: importTrackFileDialog.currentFolder
                }
            }
        }


    }


    MenuRow {
        visible: core.isFactoryMode
        CheckButton {
            id: flasherStart
            text: "Flash Firmware"
            Layout.fillWidth: true
            checkable: false

            onClicked: {
                core.connectOpenedLinkAsFlasher(flasherPnText.text)
            }
        }

        CheckButton {
            id: flasherDataRefresh
            Layout.fillWidth: false
            checkable: true
            icon.source: "qrc:/icons/ui/refresh.svg"

            onCheckedChanged: {
                flasherDataInput.text = ""
            }
        }
    }

    MenuRow {
        visible: flasherDataRefresh.checked && core.isFactoryMode
        CTextField {
            id: flasherDataInput
            Layout.fillWidth: true
            onVisibleChanged: {
                if(visible) {
                    focus = true
                }
            }
        }

        CheckButton {
            Layout.fillWidth: false
            checkable: false
            visible: flasherDataRefresh.checked
            icon.source: "qrc:/icons/ui/file_download.svg"

            onClicked: {
                if(flasherDataInput.text !== "") {
                    core.setFlasherData(flasherDataInput.text)
                    flasherDataInput.text = ""
                    flasherDataRefresh.checked = false
                }
            }
        }
    }

    MenuRow {
        visible: core.isFactoryMode
        CText {
            text: "Part Number:"
        }

        CTextField {
            id: flasherPnText
            Layout.fillWidth: true
        }

        Settings {
            property alias flasherPartNumber: flasherPnText.text
        }
    }

    MenuRow {
        visible: core.isFactoryMode
        CText {
            text: FLASHER_STATE ? core.flasherTextInfo : ""
        }
    }

    MenuRow {
        spacing: 4
        CheckButton {
            id: zeroingPosButton
            icon.source: "qrc:/icons/ui/propeller_off.svg"
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

            Keys.onPressed: function(event) {
                if (event.key === 16777220 || event.key === Qt.Key_Enter) {
                    core.openLogFile(pathText.text, false, false);
                }
            }

            Settings {
                property alias pathText: pathText.text
            }
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/file.svg"
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
                currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]

                onAccepted: {
                    const file = newFileDialog.selectedFile
                    if (!file) {
                        return
                    }

                    const fileStr = file.toString()
                    pathText.text = fileStr.replace("file:///", Qt.platform.os === "windows" ? "" : "/")

                    var name_parts = fileStr.split('.')

                    core.openLogFile(pathText.text, false, false)
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: newFileDialog.currentFolder
            }
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/file_plus.svg"
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
                currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]

                onAccepted: {
                    pathText.text = appendFileDialog.fileUrl.toString().replace("file:///", Qt.platform.os === "windows" ? "" : "/")

                    var name_parts = appendFileDialog.fileUrl.toString().split('.')

                    //deviceManagerWrapper.sendOpenFile(pathText.text, true)
                    core.openLogFile(pathText.text, true, false);
                }
                onRejected: {
                }
            }

            Settings {
                property alias logFolder: appendFileDialog.currentFolder
            }
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/file_off.svg"
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
                text: modelData ? (modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]") : qsTr("Undefined")
                Layout.fillWidth: true
                opacity: dev === modelData ? 1 : 0.5
                visible: modelData ? (modelData.devType === 0 ? false : true) : false

                onClicked: {
                    dev = modelData
                }
            }
        }
    }
}

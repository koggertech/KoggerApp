import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    Layout.fillWidth: true
    Layout.preferredHeight: columnConnectionItem.height

    property var dev: null
    property var devList: devs.devs

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

    ColumnLayout {
        id: columnConnectionItem
        width: parent.width
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10

            CCombo  {
                id: connectionTypeCombo
                Layout.fillWidth: true
                model: ["Serial", "IP", "File"]

                Settings {
                    property alias connectionType: connectionTypeCombo.currentIndex
                }
            }

            CCombo  {
                id: portCombo
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "Serial"
                onPressedChanged: {
                    if(pressed) {
                        model = core.availableSerialName()
                    }
                }

                Component.onCompleted: {
                    model = core.availableSerialName()
                }

                Settings {
                    property alias connectionPortText: portCombo.currentText
                }
            }

            CCombo  {
                id: baudrateCombo
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "Serial"
                model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000]
                currentIndex: 4

//                onCurrentTextChanged: {
//                    if(connectionButton.connection) {
//                        dev.baudrate = Number(baudrateCombo.currentText)

//                        core.openConnectionAsSerial(portCombo.currentText, Number(baudrateCombo.currentText), false)
//                    }
//                }

                Settings {
                    property alias serialBaudrate: baudrateCombo.currentIndex
                    property alias serialBaudrateText: baudrateCombo.currentText
                }
            }

            CTextField {
                id: pathText
                hoverEnabled: true
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "File"

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

            CCombo  {
                id: ipTypeCombo
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "IP"
                model: ["UDP", "TCP"]


                Settings {
                    property alias ipTypeCombo: ipTypeCombo.currentIndex
                }
            }

            CTextField {
                id: ipAddressText
                hoverEnabled: true
                Layout.fillWidth: true
                visible: connectionTypeCombo.currentText === "IP"

                text: "192.168.4.1"
                placeholderText: ""

                Keys.onPressed: {
                    if (event.key === 16777220) {
                        console.info(ipAddressText.text)
                    }
                }

                Settings {
                    property alias ipAddressText: ipAddressText.text
                }
            }

            CTextField {
                id: ipPortText
                hoverEnabled: true
                Layout.fillWidth: false
                implicitWidth: 80
                visible: connectionTypeCombo.currentText === "IP"

                text: "14444"
                placeholderText: qsTr("Port")

                Settings {
                    property alias ipPortText: ipPortText.text
                }
            }

            CButton {
                text: "..."
                Layout.fillWidth: false
                visible: connectionTypeCombo.currentText === "File"
                implicitHeight: theme.controlHeight
                implicitWidth: implicitHeight*1.1
                onClicked: {
                    logFileDialog.open()
                }

                FileDialog {
                    id: logFileDialog
                    title: "Please choose a file"
                    folder: shortcuts.home
//                    fileMode: FileDialog.OpenFiles

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



            CButton {
                id: connectionButton
                property bool connection: false
                implicitWidth: implicitHeight + 3
                visible: connectionTypeCombo.currentText !== "File"

                function openConnection() {
                    if(connectionTypeCombo.currentText === "Serial") {
                        core.openConnectionAsSerial(1, autoconnectionCheck.checked, portCombo.currentText, Number(baudrateCombo.currentText), false)
                    } else if(connectionTypeCombo.currentText === "IP") {
                        core.openConnectionAsIP(1, autoconnectionCheck.checked, ipAddressText.text, Number(ipPortText.text), ipTypeCombo.currentText === "TCP");
                    }
                }

                text: ""

//                ToolTip {
//                    id: control
//                    text: qsTr("A descriptive tool tip of what the button does")
//                    visible: connectionButton.hovered
//                    implicitWidth: 100
//                    implicitHeight: 100

//                    contentItem: Text {
//                        text: control.text
//                        font: control.font
//                        color: "#21be2b"
//                    }

//                    background: Rectangle {
//                        border.color: "#21be2b"
//                    }
//                }

                onClicked: {
                    if(connection) {
                        core.closeConnection()
                    } else {
                        connectionButton.openConnection()
                    }
                }

                Component.onCompleted: {

                }

                onConnectionChanged: {
                    canvas.requestPaint()
                }

                indicator: Canvas {
                    id: canvas
                    x: connectionButton.width - width - connectionButton.rightPadding
                    y: connectionButton.topPadding + (connectionButton.availableHeight - height) / 2
                    width: connectionButton.availableWidth
                    height: connectionButton.availableHeight
                    contextType: "2d"

                    Connections {
                        target: connectionButton

                        function onPressedChanged() {
                            canvas.requestPaint()
                        }
                    }

                    onPaint: {
                        context.reset();

                        if(connectionButton.connection) {
                            context.moveTo(0, 0);
                            context.lineTo(width, 0);
                            context.lineTo(width, height);
                            context.lineTo(0, height);
                            context.closePath();
                        } else {
                            context.moveTo(0, 0);
                            context.lineTo(width, height/2);
                            context.lineTo(0, height);
                            context.closePath();
                        }

                        context.fillStyle = connectionButton.connection ? "#E05040" : "#40E050"
                        context.fill();
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
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
                visible: dev.devName !== ""
                groupName: dev.devName

                ParamSetup {
                    paramName: "SN: " + dev.devSN
                }

                ParamSetup {
                    visible: dev.fwVersion !== ""
                    paramName: "FW version: "  + dev.fwVersion
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
                text: devList[0].devName + " " + devList[0].fwVersion + " [" + devList[0].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[0] ? 1 : 0.5

                onClicked: {
                    dev = devList[0]
                }
            }

            CButton {
                id: devTab1
                text: devList[1].devName + " " + devList[1].fwVersion + " [" + devList[1].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[1] ? 1 : 0.5

                onClicked: {
                    dev = devList[1]
                }
            }

            CButton {
                id: devTab2
                text: devList[2].devName + " " + devList[2].fwVersion + " [" + devList[2].devSN + "]"
                Layout.fillWidth: true
                opacity: dev === devList[2] ? 1 : 0.5

                onClicked: {
                    dev = devList[2]
                }
            }
        }
    }
}

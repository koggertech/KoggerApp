import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import app 1.0

Column {
    id: connectionViewer
    property var dev: null
    property var devList: deviceManagerWrapper.devs
    property string filePath: currentLogPath()
    property var lastLogFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var lastImportTrackFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var recentOpenedFiles: []
    property string selectedLogPathSource: ""
    property string importTrackPathSource: ""

    Settings {
        property alias logFolder:           connectionViewer.lastLogFolder
        property alias importTrackFolder:   connectionViewer.lastImportTrackFolder
        property alias recentOpenedFiles:   connectionViewer.recentOpenedFiles
        property alias pathText:            connectionViewer.selectedLogPathSource
        property alias importPathText:      connectionViewer.importTrackPathSource
    }

    Connections {
        target: core
        function onFileOpenFailed(path) { connectionViewer.removeRecentFile(path) }
    }

    function urlSource(value) {
        if (!value) return ""
        if (typeof value === "string") {
            if (value.startsWith("file:///"))
                return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
            if (value.startsWith("file://"))
                return value.slice(7)
            return value
        }
        var lp = value.toLocalFile ? value.toLocalFile() : ""
        return lp.length ? lp : value.toString()
    }

    function urlDisplay(value) {
        var s = urlSource(value)
        if (!s.length) return ""
        try { return decodeURIComponent(s) } catch(e) { return s }
    }

    function effectiveSource(displayText, storedSource) {
        if (!displayText || !displayText.length) return ""
        if (storedSource && displayText === urlDisplay(storedSource)) return storedSource
        return displayText
    }

    function toLocalPath(path) { return urlSource(path) }

    function setLogPath(path) {
        selectedLogPathSource = urlSource(path)
        pathText.text = urlDisplay(selectedLogPathSource)
    }

    function currentLogPath() {
        return effectiveSource(pathText.text, selectedLogPathSource)
    }

    function setImportTrackPath(path) {
        importTrackPathSource = urlSource(path)
        importPathText.text = urlDisplay(importTrackPathSource)
    }

    function currentImportTrackPath() {
        return effectiveSource(importPathText.text, importTrackPathSource)
    }

    function pushRecentOpenedFile(path) {
        var lp = urlSource(path)
        if (!lp.length) return
        var updated = [lp]
        for (var i = 0; i < recentOpenedFiles.length; ++i) {
            var item = recentOpenedFiles[i]
            if (item && item !== lp) updated.push(item)
            if (updated.length >= 3) break
        }
        recentOpenedFiles = updated
    }

    function openRecentFile(path) {
        var lp = urlSource(path)
        if (!lp.length) return
        setLogPath(lp)
        core.openLogFile(lp, false, false)
        pushRecentOpenedFile(lp)
    }

    function openNewFileDialog() {
        newFileDialog.currentFolder = connectionViewer.lastLogFolder
        newFileDialog.open()
    }

    function removeRecentFile(path) {
        var lp = urlSource(path)
        if (!lp.length) return
        var updated = []
        for (var i = 0; i < recentOpenedFiles.length; ++i) {
            var item = recentOpenedFiles[i]
            if (item && item !== lp) updated.push(item)
        }
        recentOpenedFiles = updated
    }

    function importSettingsToAllDevices(path) {
        if (!path || !path.length) return
        for (var i = 0; i < devList.length; ++i) {
            var device = devList[i]
            if (device && device.importSettingsFromXML)
                device.importSettingsFromXML(path)
        }
    }

    spacing: 8

    Component.onCompleted: {
        setLogPath(selectedLogPathSource.length ? selectedLogPathSource : core.filePath)
        setImportTrackPath(importTrackPathSource)
    }

    onDevListChanged: {
        if (devList.length > 0) dev = devList[0]
    }

    Connections {
        target: core
        function onConnectionChanged() { dev = null }
        function onFilePathChanged()   { connectionViewer.setLogPath(core.filePath) }
    }

    // ── Inline components ─────────────────────────────────────────────────

    component IconBtn: Rectangle {
        id: ib
        property bool checked: false
        property bool checkable: false
        property string iconSource: ""
        property string toolTipText: ""
        signal clicked()
        signal toggled(bool val)

        width: 28; height: 28; radius: 5
        color: checked ? AppPalette.accentBg : (ibMa.pressed ? AppPalette.bgDeep : (ibMa.containsMouse ? AppPalette.cardHover : AppPalette.card))
        border.width: 1
        border.color: (checked || ibMa.containsMouse) ? AppPalette.borderHover : AppPalette.border

        Behavior on color { ColorAnimation { duration: 80 } }

        Image {
            anchors.centerIn: parent
            width: 14; height: 14
            source: ib.iconSource
            fillMode: Image.PreserveAspectFit
            opacity: ib.checked ? 1.0 : 0.7
        }

        MouseArea {
            id: ibMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (ib.checkable) { ib.checked = !ib.checked; ib.toggled(ib.checked) }
                ib.clicked()
            }
        }

        KToolTip { text: ib.toolTipText; targetItem: ib; shown: ibMa.containsMouse && ib.toolTipText.length > 0 }
    }

    component SmallCheck: Item {
        id: sc
        property bool checked: false
        signal toggled(bool val)
        width: 18; height: 18

        Rectangle {
            anchors.fill: parent; radius: 4
            color: sc.checked ? AppPalette.accentBg : AppPalette.bg
            border.width: 1
            border.color: sc.checked ? AppPalette.accentBorder : AppPalette.borderHover
            Text {
                anchors.centerIn: parent; text: "✓"; color: AppPalette.accentBorder
                font.pixelSize: 11; font.bold: true; visible: sc.checked
            }
        }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: { sc.checked = !sc.checked; sc.toggled(sc.checked) }
        }
    }

    component CsvSpin: Rectangle {
        id: cs
        property int value: 1
        property int from: 1
        property int to: 100

        width: 70; height: 26; radius: 4
        color: AppPalette.bg; border.width: 1; border.color: AppPalette.border

        Row {
            anchors.fill: parent

            Rectangle {
                width: 20; height: parent.height; radius: 4
                color: dMa.pressed ? AppPalette.bgDeep : (dMa.containsMouse ? AppPalette.cardHover : "transparent")
                Text { anchors.centerIn: parent; text: "−"; color: AppPalette.textMuted; font.pixelSize: 13; font.bold: true }
                MouseArea {
                    id: dMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (cs.value > cs.from) cs.value-- }
                    onPressAndHold: dTim.start()
                    onReleased: dTim.stop(); onCanceled: dTim.stop()
                }
                Timer { id: dTim; interval: 80; repeat: true; onTriggered: { if (cs.value > cs.from) cs.value-- } }
            }

            Text {
                width: parent.width - 40; height: parent.height
                text: cs.value; color: AppPalette.text; font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                width: 20; height: parent.height; radius: 4
                color: uMa.pressed ? AppPalette.bgDeep : (uMa.containsMouse ? AppPalette.cardHover : "transparent")
                Text { anchors.centerIn: parent; text: "+"; color: AppPalette.textMuted; font.pixelSize: 13; font.bold: true }
                MouseArea {
                    id: uMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (cs.value < cs.to) cs.value++ }
                    onPressAndHold: uTim.start()
                    onReleased: uTim.stop(); onCanceled: uTim.stop()
                }
                Timer { id: uTim; interval: 80; repeat: true; onTriggered: { if (cs.value < cs.to) cs.value++ } }
            }
        }
    }

    // ── Link list ─────────────────────────────────────────────────────────

    ListView {
        id: filesList
        width: parent.width
        visible: count > 0
        height: Math.min(count * 34, 10 * 34)
        spacing: 3
        clip: true
        model: linkManagerWrapper.linkListModel

        delegate: Item {
            width: filesList.width
            height: 30

            readonly property bool isConnected: ConnectionStatus
            readonly property bool receivesData: ReceivesData
            readonly property bool notAvailable: IsNotAvailable

            Rectangle {
                anchors.fill: parent; radius: 6; clip: true
                color: isConnected ? (receivesData ? "#0D2D1A" : "#2D2200") : (notAvailable ? "#2D0D0D" : AppPalette.card)
                border.width: 1
                border.color: isConnected ? (receivesData ? "#10B981" : "#F59E0B") : (notAvailable ? "#EF4444" : AppPalette.border)
                opacity: IsUpgradingState ? 0.55 : 1.0

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4; anchors.rightMargin: 4
                    spacing: 3
                    enabled: !IsUpgradingState

                    IconBtn {
                        id: gearBtn
                        checkable: true
                        iconSource: "qrc:/icons/ui/settings.svg"
                        toolTipText: "Settings"
                        Layout.alignment: Qt.AlignVCenter
                        width: 26; height: 26
                    }

                    IconBtn {
                        visible: gearBtn.checked
                        checked: IsPinned; checkable: true
                        iconSource: "qrc:/icons/ui/pin.svg"
                        toolTipText: checked ? "Unpin" : "Pin"
                        Layout.alignment: Qt.AlignVCenter; width: 26; height: 26
                        onToggled: function(v) { linkManagerWrapper.sendUpdatePinnedState(Uuid, v) }
                    }

                    IconBtn {
                        visible: gearBtn.checked
                        checked: ControlType; checkable: true
                        iconSource: "qrc:/icons/ui/repeat.svg"
                        toolTipText: "Auto reconnect"
                        Layout.alignment: Qt.AlignVCenter; width: 26; height: 26
                        onToggled: function(v) { linkManagerWrapper.sendUpdateControlType(Uuid, Number(v)) }
                    }

                    IconBtn {
                        visible: gearBtn.checked && (LinkType === 2 || LinkType === 3)
                        iconSource: "qrc:/icons/ui/x.svg"; toolTipText: "Delete"
                        Layout.alignment: Qt.AlignVCenter; width: 26; height: 26
                        onClicked: linkManagerWrapper.deleteLink(Uuid)
                    }

                    // ── Serial ──
                    Text {
                        visible: LinkType === 1
                        text: PortName; color: AppPalette.textSecond; font.pixelSize: 11
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 64; elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: LinkType === 1
                        Layout.preferredWidth: 82; Layout.preferredHeight: 24
                        radius: 4; color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
                        Layout.alignment: Qt.AlignVCenter
                        ComboBox {
                            id: baudrateCombo
                            anchors.fill: parent
                            model: linkManagerWrapper.baudrateModel
                            currentIndex: 8; displayText: Baudrate
                            font.pixelSize: 11
                            background: Rectangle { color: "transparent"; border.width: 0 }
                            contentItem: Text {
                                leftPadding: 4; text: baudrateCombo.displayText
                                color: AppPalette.text; font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                            }
                            onActivated: {
                                linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(currentText))
                                autoSpeedBtn.checked = false
                            }
                        }
                    }

                    IconBtn {
                        id: autoSpeedBtn
                        visible: LinkType === 1
                        checked: AutoSpeedSelection; checkable: true
                        iconSource: "qrc:/icons/ui/refresh.svg"; toolTipText: "Auto search baudrate"
                        Layout.alignment: Qt.AlignVCenter; width: 26; height: 26
                        onToggled: function(v) { linkManagerWrapper.sendAutoSpeedSelection(Uuid, v) }
                        onCheckedChanged: { if (!checked) linkManagerWrapper.sendAutoSpeedSelection(Uuid, false) }
                    }

                    // ── UDP / TCP ──
                    Text {
                        visible: LinkType === 2 || LinkType === 3
                        text: LinkType === 2 ? "UDP" : "TCP"
                        color: AppPalette.borderFocus; font.pixelSize: 10; font.bold: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Rectangle {
                        visible: LinkType === 2 || LinkType === 3
                        Layout.fillWidth: true; Layout.minimumWidth: 58
                        Layout.preferredHeight: 24
                        radius: 4; color: AppPalette.bg; border.width: 1
                        border.color: ipField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                        Layout.alignment: Qt.AlignVCenter
                        TextInput {
                            id: ipField
                            anchors.fill: parent; anchors.margins: 4
                            verticalAlignment: TextInput.AlignVCenter
                            color: AppPalette.text; font.pixelSize: 11; clip: true
                            text: Address
                            onTextEdited: linkManagerWrapper.sendUpdateAddress(Uuid, text)
                        }
                    }

                    Text {
                        visible: LinkType === 2; text: "src:"
                        color: AppPalette.borderFocus; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter
                    }

                    Rectangle {
                        visible: LinkType === 2
                        Layout.preferredWidth: 50; Layout.preferredHeight: 24
                        radius: 4; color: AppPalette.bg; border.width: 1
                        border.color: srcPortField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                        Layout.alignment: Qt.AlignVCenter
                        TextInput {
                            id: srcPortField
                            anchors.fill: parent; anchors.margins: 4
                            verticalAlignment: TextInput.AlignVCenter
                            color: AppPalette.text; font.pixelSize: 11
                            text: SourcePort
                            onTextEdited: linkManagerWrapper.sendUpdateSourcePort(Uuid, text)
                        }
                    }

                    Text {
                        visible: LinkType === 2 || LinkType === 3
                        text: LinkType === 2 ? "dst:" : "srv:"
                        color: AppPalette.borderFocus; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter
                    }

                    Rectangle {
                        visible: LinkType === 2 || LinkType === 3
                        Layout.preferredWidth: 50; Layout.preferredHeight: 24
                        radius: 4; color: AppPalette.bg; border.width: 1
                        border.color: dstPortField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                        Layout.alignment: Qt.AlignVCenter
                        TextInput {
                            id: dstPortField
                            anchors.fill: parent; anchors.margins: 4
                            verticalAlignment: TextInput.AlignVCenter
                            color: AppPalette.text; font.pixelSize: 11
                            text: DestinationPort
                            onTextEdited: linkManagerWrapper.sendUpdateDestinationPort(Uuid, text)
                        }
                    }

                    // Open / Close
                    KButton {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 58; Layout.preferredHeight: 26
                        text: isConnected ? "Close" : "Open"
                        fontPixelSize: 11; bold: false
                        normalBg: AppPalette.card
                        checkedBg: "#134E2E"; checkedBorder: "#10B981"
                        onClicked: {
                            if (isConnected) {
                                linkManagerWrapper.closeLink(Uuid)
                            } else {
                                switch (LinkType) {
                                case 1: core.closeLogFile(); linkManagerWrapper.openAsSerial(Uuid); break
                                case 2: core.closeLogFile(); linkManagerWrapper.openAsUdp(Uuid, ipField.text, Number(srcPortField.text), Number(dstPortField.text)); break
                                case 3: core.closeLogFile(); linkManagerWrapper.openAsTcp(Uuid, ipField.text, 0, Number(dstPortField.text)); break
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent; radius: 6
                    visible: IsUpgradingState; color: "#40FFFFFF"; z: 10
                    Image { anchors.fill: parent; source: "qrc:/icons/ui/diagonal_stripe.png"; fillMode: Image.Tile; opacity: 0.4 }
                }
            }
        }

        ScrollBar.vertical: ScrollBar { }
        onCountChanged: Qt.callLater(positionViewAtEnd)
    }

    // ── Action buttons ────────────────────────────────────────────────────

    Flow {
        width: parent.width; spacing: 6

        KButton {
            text: "+UDP"; height: 30; fontPixelSize: 12
            onClicked: linkManagerWrapper.createAsUdp("", 0, 0)
        }

        KButton {
            text: "+TCP"; height: 30; fontPixelSize: 12
            onClicked: linkManagerWrapper.createAsTcp("", 0, 0)
        }

        KButton {
            id: mavlinkProxy
            text: "MAVProxy"; height: 30; fontPixelSize: 12; checkable: true
            onToggled: {
                if (checked) linkManagerWrapper.sendCreateAndOpenAsUdpProxy("127.0.0.1", 14551, 14550)
                else         linkManagerWrapper.sendCloseUdpProxy()
            }
        }

        KButton {
            id: loggingCheck
            text: "● KLF"; height: 30; fontPixelSize: 12; checkable: true
            checkedBg: "#7F1D1D"; checkedBorder: "#EF4444"
            onCheckedChanged: {
                core.setKlfLogging(checked)
                if (checked !== core.loggingKlf) checked = core.loggingKlf
            }
            Component.onCompleted: {
                core.setKlfLogging(checked)
                if (checked !== core.loggingKlf) checked = core.loggingKlf
            }
            Settings { property alias loggingCheck: loggingCheck.checked }
        }

        KButton {
            id: loggingCheck2
            text: "● CSV"; height: 30; fontPixelSize: 12; checkable: true
            checkedBg: "#7F1D1D"; checkedBorder: "#EF4444"
            onCheckedChanged: {
                core.setCsvLogging(checked)
                if (checked !== core.loggingCsv) checked = core.loggingCsv
            }
            Component.onCompleted: {
                core.setCsvLogging(checked)
                if (checked !== core.loggingCsv) checked = core.loggingCsv
            }
            Settings { property alias loggingCheck2: loggingCheck2.checked }
        }

        KButton {
            id: importCheck
            text: "Import"; height: 30; fontPixelSize: 12; checkable: true
        }
    }

    // ── CSV import panel ──────────────────────────────────────────────────

    Column {
        visible: importCheck.checked
        width: parent.width; spacing: 6

        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

        Row {
            width: parent.width; height: 28; spacing: 8
            Text { text: "Separator:"; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            Rectangle {
                width: 100; height: 26; radius: 4; color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
                anchors.verticalCenter: parent.verticalCenter
                ComboBox {
                    id: separatorCombo; anchors.fill: parent; model: ["Comma", "Tab", "Space", "SemiColon"]; font.pixelSize: 11
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    contentItem: Text { leftPadding: 6; text: separatorCombo.displayText; color: AppPalette.text; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
                    Settings { property alias separatorCombo: separatorCombo.currentIndex }
                }
            }
            Text { text: "Ряд:"; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: firstRow; value: 1; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVfirstRow: firstRow.value } }
        }

        Row {
            width: parent.width; height: 28; spacing: 8
            SmallCheck {
                id: timeEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { property alias importCSVtimeEnable: timeEnable.checked }
            }
            Text { text: "Time col:"; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: timeColumn; value: 6; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVtimeColumn: timeColumn.value } }
            Rectangle {
                width: 100; height: 26; radius: 4; color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
                anchors.verticalCenter: parent.verticalCenter
                ComboBox {
                    id: utcGpsCombo; anchors.fill: parent; model: ["UTC time", "GPS time"]; font.pixelSize: 11
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    contentItem: Text { leftPadding: 6; text: utcGpsCombo.displayText; color: AppPalette.text; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
                    Settings { property alias utcGpsCombo: utcGpsCombo.currentIndex }
                }
            }
        }

        Row {
            width: parent.width; height: 28; spacing: 8
            SmallCheck {
                id: latLonEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { property alias importCSVlatLonEnable: latLonEnable.checked }
            }
            Text { text: "Lat/Lon/Alt:"; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: latColumn;  value: 2; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVlatColumn:  latColumn.value  } }
            CsvSpin { id: lonColumn;  value: 3; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVlonColumn:  lonColumn.value  } }
            CsvSpin { id: altColumn;  value: 4; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSValtColumn:  altColumn.value  } }
        }

        Row {
            width: parent.width; height: 28; spacing: 8
            SmallCheck {
                id: xyzEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { property alias importCSVxyzEnable: xyzEnable.checked }
            }
            Text { text: "NEU:"; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: northColumn; value: 2; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVnorthColumn: northColumn.value } }
            CsvSpin { id: eastColumn;  value: 3; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVeastColumn:  eastColumn.value  } }
            CsvSpin { id: upColumn;    value: 4; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { property alias importCSVupColumn:    upColumn.value    } }
        }

        Row {
            width: parent.width; height: 30; spacing: 6

            Rectangle {
                width: parent.width - 50 - 6; height: 30; radius: 6
                color: AppPalette.bg; border.width: 1; border.color: importPathText.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: importPathText
                    anchors.fill: parent; anchors.margins: 8
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text; font.pixelSize: 12; clip: true
                    Text { visible: !importPathText.text.length; text: "Путь к CSV..."; color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Keys.onPressed: function(e) {
                        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter)
                            importTrackFileDialog.openCSV()
                    }
                }
            }

            KButton {
                width: 50; height: 30; text: "..."; fontPixelSize: 12
                onClicked: {
                    importTrackFileDialog.currentFolder = connectionViewer.lastImportTrackFolder
                    importTrackFileDialog.open()
                }

                FileDialog {
                    id: importTrackFileDialog
                    title: "Please choose a file"
                    currentFolder: connectionViewer.lastImportTrackFolder
                    nameFilters: ["Logs (*.csv *.txt)"]
                    onCurrentFolderChanged: connectionViewer.lastImportTrackFolder = currentFolder

                    function openCSV() {
                        const importPath = connectionViewer.currentImportTrackPath()
                        core.openCSV(importPath, separatorCombo.currentIndex, firstRow.value,
                                     timeColumn.value, utcGpsCombo.currentIndex === 0,
                                     latColumn.value * latLonEnable.checked,
                                     lonColumn.value * latLonEnable.checked,
                                     altColumn.value * latLonEnable.checked,
                                     northColumn.value * xyzEnable.checked,
                                     eastColumn.value * xyzEnable.checked,
                                     upColumn.value * xyzEnable.checked)
                    }

                    onAccepted: {
                        connectionViewer.lastImportTrackFolder = importTrackFileDialog.currentFolder
                        connectionViewer.setImportTrackPath(importTrackFileDialog.selectedFile)
                        openCSV()
                    }
                }
            }
        }
    }

    // ── File row ──────────────────────────────────────────────────────────

    Row {
        width: parent.width; height: 30; spacing: 6

        IconBtn {
            id: zeroingPosButton
            checkable: true; iconSource: "qrc:/icons/ui/route_crossed_out.svg"; toolTipText: "Pos zeroing"
            width: 30; height: 30; anchors.verticalCenter: parent.verticalCenter
            onToggled: function(v) { core.setPosZeroing(v) }
            Component.onCompleted: core.setPosZeroing(checked)
            Settings { property alias zeroingPosButtonCheched: zeroingPosButton.checked }
        }

        IconBtn {
            id: zeroingBottomTrackButton
            checkable: true; iconSource: "qrc:/icons/ui/double_route_crossed_out.svg"; toolTipText: "Bottom track zeroing"
            width: 30; height: 30; anchors.verticalCenter: parent.verticalCenter
            onToggled: function(v) { core.setBottomTrackZeroing(v) }
            Component.onCompleted: core.setBottomTrackZeroing(checked)
            Settings { property alias zeroingBottomTrackButtonChecked: zeroingBottomTrackButton.checked }
        }

        Rectangle {
            width: parent.width - 30 - 30 - 30 - 30 - 30 - 5 * 6
            height: 30; radius: 6; color: AppPalette.bg; border.width: 1
            border.color: pathText.activeFocus ? AppPalette.accentBorder : AppPalette.border

            TextInput {
                id: pathText
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                verticalAlignment: TextInput.AlignVCenter
                color: AppPalette.text; font.pixelSize: 12; clip: true
                Text {
                    visible: !pathText.text.length; text: "Путь к файлу..."
                    color: AppPalette.textMuted; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter
                }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        const lp = connectionViewer.currentLogPath()
                        connectionViewer.setLogPath(lp)
                        connectionViewer.pushRecentOpenedFile(lp)
                        core.openLogFile(lp, false, false)
                    }
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file.svg"; toolTipText: "Открыть файл"
            width: 30; height: 30; anchors.verticalCenter: parent.verticalCenter
            onClicked: connectionViewer.openNewFileDialog()

            FileDialog {
                id: newFileDialog
                title: "Please choose a file"
                currentFolder: connectionViewer.lastLogFolder
                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]
                onCurrentFolderChanged: connectionViewer.lastLogFolder = currentFolder
                onAccepted: {
                    const file = newFileDialog.selectedFile
                    if (!file) return
                    connectionViewer.lastLogFolder = newFileDialog.currentFolder
                    const lp = urlSource(file)
                    connectionViewer.setLogPath(lp)
                    connectionViewer.pushRecentOpenedFile(lp)
                    core.openLogFile(lp, false, false)
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_plus.svg"; toolTipText: "Добавить файл"
            width: 30; height: 30; anchors.verticalCenter: parent.verticalCenter
            onClicked: { appendFileDialog.currentFolder = connectionViewer.lastLogFolder; appendFileDialog.open() }

            FileDialog {
                id: appendFileDialog
                title: "Please choose a file"
                currentFolder: connectionViewer.lastLogFolder
                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]
                onCurrentFolderChanged: connectionViewer.lastLogFolder = currentFolder
                onAccepted: {
                    const lp = urlSource(appendFileDialog.selectedFile)
                    connectionViewer.setLogPath(lp)
                    connectionViewer.lastLogFolder = appendFileDialog.currentFolder
                    connectionViewer.pushRecentOpenedFile(lp)
                    core.openLogFile(lp, true, false)
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_off.svg"; toolTipText: "Закрыть файл"
            width: 30; height: 30; anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                if (core.openedFilePath.length > 0) {
                    core.closeLogFile();
                } else {
                    core.onRequestClearing();
                }
            }
        }
    }

    // ── Device buttons ────────────────────────────────────────────────────

    Flow {
        visible: devList.length > 0
        width: parent.width; spacing: 6

        Repeater {
            model: devList
            delegate: KButton {
                required property var modelData
                text: modelData ? (modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]") : "Undefined"
                height: 30; fontPixelSize: 11
                opacity: dev === modelData ? 1.0 : 0.5
                visible: modelData ? (modelData.devType !== 0) : false
                onClicked: dev = modelData
            }
        }
    }

    // ── Factory mode ──────────────────────────────────────────────────────

    Row {
        visible: core.isFactoryMode; width: parent.width; height: 30; spacing: 6

        KButton {
            text: "Flash Firmware"; height: 30; fontPixelSize: 12
            onClicked: core.connectOpenedLinkAsFlasher(flasherPnText.text)
        }

        IconBtn {
            id: flasherDataRefresh; checkable: true
            iconSource: "qrc:/icons/ui/refresh.svg"; width: 30; height: 30
            onToggled: function(v) { if (!v) flasherDataInput.text = "" }
        }
    }

    Row {
        visible: flasherDataRefresh.checked && core.isFactoryMode
        width: parent.width; height: 30; spacing: 6

        Rectangle {
            width: parent.width - 36; height: 30; radius: 6
            color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
            TextInput {
                id: flasherDataInput
                anchors.fill: parent; anchors.margins: 8
                verticalAlignment: TextInput.AlignVCenter; color: AppPalette.text; font.pixelSize: 12
                onVisibleChanged: if (visible) focus = true
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_download.svg"; width: 30; height: 30
            onClicked: {
                if (flasherDataInput.text !== "") {
                    core.setFlasherData(flasherDataInput.text)
                    flasherDataInput.text = ""
                    flasherDataRefresh.checked = false
                }
            }
        }
    }

    Row {
        visible: core.isFactoryMode; width: parent.width; height: 30; spacing: 8

        Text { text: "Part Number:"; color: AppPalette.textSecond; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }

        Rectangle {
            width: parent.width - 92 - 8; height: 30; radius: 6
            color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
            TextInput {
                id: flasherPnText
                anchors.fill: parent; anchors.margins: 8
                verticalAlignment: TextInput.AlignVCenter; color: AppPalette.text; font.pixelSize: 12
                Settings { property alias flasherPartNumber: flasherPnText.text }
            }
        }
    }

    Text {
        visible: core.isFactoryMode && FLASHER_STATE
        text: core.isFactoryMode && FLASHER_STATE ? core.flasherTextInfo : ""
        color: AppPalette.textMuted; font.pixelSize: 12; width: parent.width; wrapMode: Text.WordWrap
    }

    // ── Recent files ──────────────────────────────────────────────────────

    Column {
        visible: recentOpenedFiles.length > 0
        width: parent.width
        spacing: 3

        Text {
            text: qsTr("Recently opened:")
            color: AppPalette.textMuted
            font.pixelSize: 11
            leftPadding: 2
        }

        Repeater {
            model: Math.min(recentOpenedFiles.length, 3)

            Row {
                width: parent.width
                spacing: 4

                property string filePath: recentOpenedFiles[index] || ""

                Rectangle {
                    width: parent.width - 34
                    height: 28; radius: 6
                    color: recentMa.containsMouse ? AppPalette.cardHover : AppPalette.card
                    border.width: 1; border.color: AppPalette.border
                    Behavior on color { ColorAnimation { duration: 80 } }

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        text: connectionViewer.urlDisplay(parent.parent.filePath)
                        color: AppPalette.text; font.pixelSize: 11
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideLeft
                    }

                    MouseArea {
                        id: recentMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: connectionViewer.openRecentFile(parent.parent.filePath)
                    }
                }

                IconBtn {
                    iconSource: "qrc:/icons/ui/x.svg"
                    width: 28; height: 28
                    toolTipText: qsTr("Remove")
                    onClicked: connectionViewer.removeRecentFile(parent.filePath)
                }
            }
        }
    }

    // ── Device settings ───────────────────────────────────────────────────

    DeviceSettingsPage {
        visible: connectionViewer.dev !== null
        width: parent.width
        dev: connectionViewer.dev
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import app 1.0

Column {
    id: connectionViewer

    // Injected by ConnectionsSettingsPage.onLoaded.
    property var store: null

    property var devList: deviceManagerWrapper.devs

    // Resolved via store.activeDeviceIndex (devSN not unique), falls back to devList[0].
    readonly property var dev: {
        if (!devList || devList.length === 0) return null
        var idx = store ? store.activeDeviceIndex : -1
        if (idx >= 0 && idx < devList.length)
            return devList[idx]
        return devList[0]
    }
    property var lastImportTrackFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property string importTrackPathSource: ""

    Settings {
        category: "main/csvImport"
        property alias importTrackFolder:   connectionViewer.lastImportTrackFolder
        property alias importPathText:      connectionViewer.importTrackPathSource
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

    function escapeHtml(s) {
        return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    function setImportTrackPath(path) {
        importTrackPathSource = urlSource(path)
        importPathText.text = urlDisplay(importTrackPathSource)
    }

    function currentImportTrackPath() {
        return effectiveSource(importPathText.text, importTrackPathSource)
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
        setImportTrackPath(importTrackPathSource)
    }

    onDevListChanged: syncActiveDevice()

    function syncActiveDevice() {
        if (!store) return
        if (!devList || devList.length === 0) {
            store.setActiveDeviceIndex(-1)
            return
        }
        var idx = store.activeDeviceIndex
        if (idx >= 0 && idx < devList.length)
            return
        store.setActiveDeviceIndex(0)
    }

    onStoreChanged: {
        syncActiveDevice()
        _maybeScrollToDevice()
    }

    Connections {
        target: core
        function onConnectionChanged() {
            if (store) store.setActiveDeviceIndex(-1)
        }
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

        width: Math.round(28 * AppPalette.scale); height: Math.round(28 * AppPalette.scale); radius: Tokens.radiusSm + 1
        color: checked ? AppPalette.accentBg : (ibMa.pressed ? AppPalette.bgDeep : (ibMa.containsMouse ? Qt.lighter(AppPalette.controlRaised, 1.2) : AppPalette.controlRaised))
        border.width: Tokens.cardBorderWidth
        border.color: (checked || ibMa.containsMouse) ? AppPalette.borderHover : AppPalette.border

        Behavior on color { ColorAnimation { duration: 80 } }

        activeFocusOnTab: enabled
        function _activate() {
            if (ib.checkable) { ib.checked = !ib.checked; ib.toggled(ib.checked) }
            ib.clicked()
        }
        Keys.onReturnPressed: ib._activate()
        Keys.onEnterPressed:  ib._activate()
        Keys.onSpacePressed:  ib._activate()

        KFocusRing { id: focusRing }

        Image {
            anchors.centerIn: parent
            // Proportional to outer — scales reliably regardless of consumer's
            // width/height override (gear, autoSpeed, etc. set their own size).
            width: Math.round(ib.width * 0.55)
            height: Math.round(ib.height * 0.55)
            source: ib.iconSource
            fillMode: Image.PreserveAspectFit
            opacity: ib.checked ? 1.0 : 0.7
            smooth: true
        }

        MouseArea {
            id: ibMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: focusRing.suppress()
            onClicked: { ib.forceActiveFocus(); ib._activate() }
        }

        KToolTip { text: ib.toolTipText; targetItem: ib; shown: ibMa.containsMouse && ib.toolTipText.length > 0 }
    }

    // Inline toggle switch (matches KSwitch's indicator size, same as
    // AppSettingsPage SmallCheck — consistent across the app, easy to tap).
    component SmallCheck: Item {
        id: sc
        property bool checked: false
        signal toggled(bool val)

        readonly property int _knobMargin: Math.max(2, Math.round(2 * AppPalette.scale))

        width: Math.round(44 * AppPalette.scale)
        height: Math.round(24 * AppPalette.scale)

        activeFocusOnTab: true
        function _toggle() { sc.checked = !sc.checked; sc.toggled(sc.checked) }
        Keys.onReturnPressed: sc._toggle()
        Keys.onEnterPressed:  sc._toggle()
        Keys.onSpacePressed:  sc._toggle()

        Rectangle {
            id: scTrack
            anchors.fill: parent
            radius: height / 2
            color: sc.checked ? AppPalette.accentBg : AppPalette.trackOff
            border.width: 1
            border.color: sc.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder
            Behavior on color { ColorAnimation { duration: 120 } }

            Rectangle {
                width: parent.height - 2 * sc._knobMargin
                height: width
                radius: width / 2
                y: sc._knobMargin
                x: sc.checked ? parent.width - width - sc._knobMargin : sc._knobMargin
                color: AppPalette.knob
                border.width: 1
                border.color: "#00000022"
                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }
        }

        KFocusRing { id: focusRing; target: scTrack; focusItem: sc; inset: 3 }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: focusRing.suppress()
            onClicked: { sc.forceActiveFocus(); sc._toggle() }
        }
    }

    component CsvSpin: Rectangle {
        id: cs
        property int value: 1
        property int from: 1
        property int to: 100

        width: Math.round(70 * AppPalette.scale); height: Math.round(26 * AppPalette.scale); radius: Tokens.radiusSm
        color: AppPalette.bg
        border.width: 1
        border.color: (cs.activeFocus && !cs._ringSuppressed) ? AppPalette.accentBorder : AppPalette.border

        activeFocusOnTab: true
        property bool _ringSuppressed: false
        onActiveFocusChanged: if (!activeFocus) _ringSuppressed = false
        Keys.onUpPressed:   function(e) { if (cs.value < cs.to)   cs.value++; e.accepted = true }
        Keys.onDownPressed: function(e) { if (cs.value > cs.from) cs.value--; e.accepted = true }

        Row {
            anchors.fill: parent

            Rectangle {
                width: Math.round(20 * AppPalette.scale); height: parent.height; radius: Tokens.radiusSm
                color: dMa.pressed ? AppPalette.bgDeep : (dMa.containsMouse ? AppPalette.cardHover : "transparent")
                Text { anchors.centerIn: parent; text: "−"; color: AppPalette.textMuted; font.pixelSize: Tokens.fontMd; font.bold: true }
                MouseArea {
                    id: dMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onPressed: cs._ringSuppressed = true
                    onClicked: { cs.forceActiveFocus(); if (cs.value > cs.from) cs.value-- }
                    onPressAndHold: dTim.start()
                    onReleased: dTim.stop(); onCanceled: dTim.stop()
                }
                Timer { id: dTim; interval: 80; repeat: true; onTriggered: { if (cs.value > cs.from) cs.value-- } }
            }

            Text {
                width: parent.width - 40; height: parent.height
                text: cs.value; color: AppPalette.text; font.pixelSize: Tokens.fontXs
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                width: Math.round(20 * AppPalette.scale); height: parent.height; radius: Tokens.radiusSm
                color: uMa.pressed ? AppPalette.bgDeep : (uMa.containsMouse ? AppPalette.cardHover : "transparent")
                Text { anchors.centerIn: parent; text: "+"; color: AppPalette.textMuted; font.pixelSize: Tokens.fontMd; font.bold: true }
                MouseArea {
                    id: uMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onPressed: cs._ringSuppressed = true
                    onClicked: { cs.forceActiveFocus(); if (cs.value < cs.to) cs.value++ }
                    onPressAndHold: uTim.start()
                    onReleased: uTim.stop(); onCanceled: uTim.stop()
                }
                Timer { id: uTim; interval: 80; repeat: true; onTriggered: { if (cs.value < cs.to) cs.value++ } }
            }
        }
    }

    // ── Link list ─────────────────────────────────────────────────────────

    Text {
        visible: linkRepeater.count > 0
        text: qsTr("Connections:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    Column {
        id: linkList
        width: parent.width
        visible: linkRepeater.count > 0
        spacing: Tokens.spaceXxs + 1
        property string expandedUuid: ""

        Repeater {
            id: linkRepeater
            model: linkManagerWrapper.linkListModel

        delegate: Item {
            id: connRow
            width: linkList.width

            activeFocusOnTab: true
            Keys.onReturnPressed: connRow._toggleEdit()
            Keys.onEnterPressed:  connRow._toggleEdit()
            Keys.onSpacePressed:  connRow._toggleEdit()
            function _toggleEdit() {
                var willOpen = !connRow.editing
                linkList.expandedUuid = willOpen ? String(Uuid) : ""
                if (willOpen)
                    connectionViewer._requestExpandScroll(connRow.rowIndex)
            }

            readonly property bool isConnected: ConnectionStatus
            readonly property bool receivesData: ReceivesData
            readonly property bool notAvailable: IsNotAvailable
            readonly property bool editing: linkList.expandedUuid === String(Uuid)
            readonly property int rowIndex: index
            readonly property int vPad: Tokens.spaceXs   // fixed — no inward shift on expand (matches recRow)
            readonly property string typeLabel: LinkType === 1 ? PortName : (LinkType === 2 ? "UDP" : "TCP")

            height: content.implicitHeight + 2 * vPad

            Rectangle {
                anchors.fill: parent; radius: Tokens.radiusMd; clip: true
                color: isConnected ? (receivesData ? AppPalette.linkOkBg : AppPalette.linkIdleBg) : (notAvailable ? AppPalette.linkDownBg : AppPalette.card)
                border.width: connRow.editing ? 2 : Tokens.cardBorderWidth
                border.color: connRow.editing ? AppPalette.accentBorder
                       : isConnected ? (receivesData ? AppPalette.linkOkBorder : AppPalette.linkIdleBorder) : (notAvailable ? AppPalette.linkDownBorder : AppPalette.border)
                opacity: IsUpgradingState ? 0.55 : 1.0
                Behavior on color { ColorAnimation { duration: Anim.fadeMs } }
                Behavior on border.color { ColorAnimation { duration: Anim.fadeMs } }

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onPressed: function(mouse) {
                        focusRing.suppress()
                        connRow.forceActiveFocus()
                        mouse.accepted = false
                    }
                }

                KFocusRing { id: focusRing; focusItem: connRow; z: 15 }

                Column {
                    id: content
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.leftMargin: connRow.vPad; anchors.rightMargin: connRow.vPad
                    anchors.topMargin: connRow.vPad
                    spacing: 0
                    enabled: !IsUpgradingState

                    // ── Заголовок (одна строка, всегда) ──
                    RowLayout {
                        width: parent.width
                        height: Tokens.controlHMd
                        spacing: Tokens.spaceXs

                        IconBtn {
                            id: gearBtn
                            checked: connRow.editing
                            iconSource: "qrc:/icons/ui/settings.svg"
                            toolTipText: qsTr("Settings")
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                            onClicked: connRow._toggleEdit()
                        }

                        Text {
                            text: connectionViewer.escapeHtml(connRow.typeLabel)
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontXl; font.bold: true
                            textFormat: Text.StyledText
                            elide: Text.ElideRight
                            Layout.fillHeight: true
                            verticalAlignment: Text.AlignVCenter
                            Layout.maximumWidth: Math.round((LinkType === 1 ? 80 : 44) * AppPalette.scale)
                        }

                        // Компактный read-only итог (тип уже отдельным бейджем выше)
                        Text {
                            visible: !connRow.editing
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontLg
                            textFormat: Text.StyledText
                            text: {
                                var muted = AppPalette.textMuted
                                var esc = connectionViewer.escapeHtml
                                if (LinkType === 1)
                                    return '<font color="' + muted + '">' + esc(Baudrate) + '</font>'
                                var a = esc((Address && Address.length) ? Address : "—")
                                if (LinkType === 2)
                                    return a + '  <font color="' + muted + '">·  ' + esc(SourcePort) + ' → ' + esc(DestinationPort) + '</font>'
                                return a + '  <font color="' + muted + '">·  ' + esc(DestinationPort) + '</font>'
                            }
                        }

                        // Доп.кнопки режима редактирования
                        IconBtn {
                            visible: connRow.editing
                            checked: IsPinned; checkable: true
                            iconSource: "qrc:/icons/ui/pin.svg"
                            toolTipText: checked ? qsTr("Unpin") : qsTr("Pin")
                            Layout.alignment: Qt.AlignVCenter; Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                            onToggled: function(v) { linkManagerWrapper.sendUpdatePinnedState(Uuid, v) }
                        }
                        IconBtn {
                            visible: connRow.editing
                            checked: ControlType; checkable: true
                            iconSource: "qrc:/icons/ui/repeat.svg"
                            toolTipText: qsTr("Auto reconnect")
                            Layout.alignment: Qt.AlignVCenter; Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                            onToggled: function(v) { linkManagerWrapper.sendUpdateControlType(Uuid, Number(v)) }
                        }
                        IconBtn {
                            visible: connRow.editing && (LinkType === 2 || LinkType === 3)
                            iconSource: "qrc:/icons/ui/x.svg"; toolTipText: qsTr("Delete")
                            Layout.alignment: Qt.AlignVCenter; Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                            onClicked: linkManagerWrapper.deleteLink(Uuid)
                        }

                        Item { visible: connRow.editing; Layout.fillWidth: true; Layout.preferredHeight: 1 }

                        // Открыть / Закрыть — всегда; читает сохранённые значения из модели
                        KButton {
                            readonly property int openCloseW: Math.round(84 * AppPalette.scale)
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: openCloseW
                            Layout.minimumWidth: openCloseW
                            Layout.maximumWidth: openCloseW
                            Layout.preferredHeight: Tokens.controlHMd
                            text: isConnected ? qsTr("Close") : qsTr("Open")
                            fontPixelSize: Tokens.fontSm; bold: false
                            normalBg: AppPalette.controlRaised
                            hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
                            checkedBg: "#134E2E"; checkedBorder: "#10B981"
                            onClicked: {
                                if (isConnected) {
                                    linkManagerWrapper.closeLink(Uuid)
                                } else {
                                    switch (LinkType) {
                                    case 1: core.closeLogFile(); linkManagerWrapper.openAsSerial(Uuid); break
                                    case 2: core.closeLogFile(); linkManagerWrapper.openAsUdp(Uuid, Address, Number(SourcePort), Number(DestinationPort)); break
                                    case 3: core.closeLogFile(); linkManagerWrapper.openAsTcp(Uuid, Address, 0, Number(DestinationPort)); break
                                    }
                                }
                            }
                        }
                    }

                    // ── Тело редактора (раскрывается анимированно) ──
                    Item {
                        id: editBody
                        width: parent.width
                        clip: true
                        height: connRow.editing ? bodyCol.implicitHeight + connRow.vPad : 0
                        opacity: connRow.editing ? 1 : 0
                        visible: editBody.height > 0.5
                        Behavior on height { NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing } }
                        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }

                        ColumnLayout {
                            id: bodyCol
                            y: connRow.vPad
                            width: parent.width
                            spacing: Tokens.spaceXs

                            // UDP/TCP — адрес
                            RowLayout {
                                visible: LinkType === 2 || LinkType === 3
                                Layout.fillWidth: true
                                spacing: Tokens.spaceXs
                                Text { text: qsTr("IP"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontBase; Layout.preferredWidth: Math.round(34 * AppPalette.scale) }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Tokens.controlHMd
                                    radius: Tokens.radiusMd; color: AppPalette.bg
                                    border.width: ipField.activeFocus ? 1 : Tokens.cardBorderWidth
                                    border.color: ipField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                                    TextInput {
                                        id: ipField
                                        property int _prevLen: 0
                                        activeFocusOnTab: true
                                        anchors.fill: parent
                                        anchors.leftMargin: Tokens.spaceSm; anchors.rightMargin: Tokens.spaceXs
                                        anchors.topMargin: Tokens.spaceXxs; anchors.bottomMargin: Tokens.spaceXxs
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: AppPalette.text; font.pixelSize: Tokens.fontBase; clip: true
                                        maximumLength: 15
                                        inputMethodHints: Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                                        validator: RegularExpressionValidator {
                                            regularExpression: /^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){0,3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)?$/
                                        }
                                        text: Address
                                        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: ipField.selectAll() }
                                        onTextEdited: {
                                            var parts = text.split('.')
                                            var lastSeg = parts[parts.length - 1]
                                            if (text.length > ipField._prevLen
                                                    && cursorPosition === text.length
                                                    && parts.length < 4
                                                    && lastSeg.length === 3
                                                    && text.charAt(text.length - 1) !== '.') {
                                                text = text + '.'
                                                cursorPosition = text.length
                                            }
                                            ipField._prevLen = text.length
                                            linkManagerWrapper.sendUpdateAddress(Uuid, text)
                                        }
                                    }
                                }
                            }

                            // UDP — порты src/dst во всю строку
                            RowLayout {
                                visible: LinkType === 2
                                Layout.fillWidth: true
                                spacing: Tokens.spaceXs
                                Text { text: qsTr("src"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontBase; Layout.preferredWidth: Math.round(34 * AppPalette.scale) }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Tokens.controlHMd
                                    radius: Tokens.radiusMd; color: AppPalette.bg
                                    border.width: srcPortField.activeFocus ? 1 : Tokens.cardBorderWidth
                                    border.color: srcPortField.activeFocus ? AppPalette.accentBorder : AppPalette.border
                                    TextInput {
                                        id: srcPortField
                                        activeFocusOnTab: true
                                        anchors.fill: parent
                                        anchors.leftMargin: Tokens.spaceSm; anchors.rightMargin: Tokens.spaceXs
                                        anchors.topMargin: Tokens.spaceXxs; anchors.bottomMargin: Tokens.spaceXxs
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: AppPalette.text; font.pixelSize: Tokens.fontBase
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        maximumLength: 5
                                        validator: IntValidator { bottom: 0; top: 65535 }
                                        text: SourcePort
                                        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: srcPortField.selectAll() }
                                        onTextEdited: linkManagerWrapper.sendUpdateSourcePort(Uuid, text)
                                    }
                                }
                                Text { text: qsTr("dst"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontBase }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Tokens.controlHMd
                                    radius: Tokens.radiusMd; color: AppPalette.bg
                                    border.width: dstPortFieldUdp.activeFocus ? 1 : Tokens.cardBorderWidth
                                    border.color: dstPortFieldUdp.activeFocus ? AppPalette.accentBorder : AppPalette.border
                                    TextInput {
                                        id: dstPortFieldUdp
                                        activeFocusOnTab: true
                                        anchors.fill: parent
                                        anchors.leftMargin: Tokens.spaceSm; anchors.rightMargin: Tokens.spaceXs
                                        anchors.topMargin: Tokens.spaceXxs; anchors.bottomMargin: Tokens.spaceXxs
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: AppPalette.text; font.pixelSize: Tokens.fontBase
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        maximumLength: 5
                                        validator: IntValidator { bottom: 0; top: 65535 }
                                        text: DestinationPort
                                        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: dstPortFieldUdp.selectAll() }
                                        onTextEdited: linkManagerWrapper.sendUpdateDestinationPort(Uuid, text)
                                    }
                                }
                            }

                            // TCP — порт сервера в половину строки
                            RowLayout {
                                visible: LinkType === 3
                                Layout.fillWidth: true
                                spacing: Tokens.spaceXs
                                Text { text: qsTr("srv"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontBase; Layout.preferredWidth: Math.round(34 * AppPalette.scale) }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Tokens.controlHMd
                                    radius: Tokens.radiusMd; color: AppPalette.bg
                                    border.width: dstPortFieldTcp.activeFocus ? 1 : Tokens.cardBorderWidth
                                    border.color: dstPortFieldTcp.activeFocus ? AppPalette.accentBorder : AppPalette.border
                                    TextInput {
                                        id: dstPortFieldTcp
                                        activeFocusOnTab: true
                                        anchors.fill: parent
                                        anchors.leftMargin: Tokens.spaceSm; anchors.rightMargin: Tokens.spaceXs
                                        anchors.topMargin: Tokens.spaceXxs; anchors.bottomMargin: Tokens.spaceXxs
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: AppPalette.text; font.pixelSize: Tokens.fontBase
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        maximumLength: 5
                                        validator: IntValidator { bottom: 0; top: 65535 }
                                        text: DestinationPort
                                        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: dstPortFieldTcp.selectAll() }
                                        onTextEdited: linkManagerWrapper.sendUpdateDestinationPort(Uuid, text)
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // Serial — бодрейт + автопоиск
                            RowLayout {
                                visible: LinkType === 1
                                Layout.fillWidth: true
                                spacing: Tokens.spaceXs
                                Text { text: qsTr("baudrate"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontBase; Layout.preferredWidth: Math.round(70 * AppPalette.scale) }
                                KCombo {
                                    id: baudrateCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Tokens.controlHMd
                                    model: linkManagerWrapper.baudrateModel
                                    currentIndex: 8
                                    displayTextOverride: Baudrate
                                    fontPixelSize: Tokens.fontBase
                                    bold: false
                                    maxVisibleItems: 9
                                    onActivated: {
                                        linkManagerWrapper.sendUpdateBaudrate(Uuid, Number(baudrateCombo.currentText))
                                        autoSpeedBtn.checked = false
                                    }
                                }
                                IconBtn {
                                    id: autoSpeedBtn
                                    checked: AutoSpeedSelection; checkable: true
                                    iconSource: "qrc:/icons/ui/refresh.svg"; toolTipText: qsTr("Auto search baudrate")
                                    Layout.alignment: Qt.AlignVCenter; Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                                    onToggled: function(v) { linkManagerWrapper.sendAutoSpeedSelection(Uuid, v) }
                                    onCheckedChanged: { if (!checked) linkManagerWrapper.sendAutoSpeedSelection(Uuid, false) }
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
        }
    }

    // ── Action buttons (4 per row, equal width) ───────────────────────────

    Text {
        text: qsTr("Add connection:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    Grid {
        id: actionsGrid
        width: parent.width
        readonly property int cellMinW: Math.round(100 * AppPalette.scale)
        columns: Tokens.gridColumns(width, cellMinW, Tokens.spaceSm, 6)
        rowSpacing: Tokens.spaceSm
        columnSpacing: Tokens.spaceSm
        readonly property real cellW: Math.max(0, (width - columnSpacing * (columns - 1)) / columns)

        KButton {
            width: actionsGrid.cellW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm
            text: qsTr("+UDP")
            onClicked: linkManagerWrapper.createAsUdp("", 0, 0)
        }

        KButton {
            width: actionsGrid.cellW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm
            text: qsTr("+TCP")
            onClicked: linkManagerWrapper.createAsTcp("", 0, 0)
        }

        KButton {
            id: mavlinkProxy
            width: actionsGrid.cellW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm; checkable: true
            text: qsTr("MAVProxy")
            onToggled: {
                if (checked) linkManagerWrapper.sendCreateAndOpenAsUdpProxy("127.0.0.1", 14551, 14550)
                else         linkManagerWrapper.sendCloseUdpProxy()
            }
        }

        KButton {
            id: importCheck
            visible: false
            width: actionsGrid.cellW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm; checkable: true
            text: qsTr("Import")
        }
    }

    // ── Recording row (gear + status marquee + size/time + REC) ───────────

    Text {
        text: qsTr("Recording:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    Rectangle {
        id: recRow
        width: parent.width
        radius: Tokens.radiusMd
        readonly property bool active: !!(core.loggingKlf || core.loggingCsv)
        color: active ? "#7F1D1D" : AppPalette.card
        border.width: recGear.checked ? 2 : Tokens.cardBorderWidth
        border.color: recGear.checked ? AppPalette.accentBorder : (active ? "#EF4444" : AppPalette.border)
        implicitHeight: recCol.implicitHeight + 2 * Tokens.spaceXs
        Behavior on color { ColorAnimation { duration: Anim.fadeMs } }
        Behavior on border.color { ColorAnimation { duration: Anim.fadeMs } }

        property int recSeconds: 0
        property int recBytes: 0
        function _refresh() {
            recSeconds = core.activeLogDurationSecs()
            recBytes = core.activeLogSizeBytes()
        }
        onActiveChanged: { _refresh(); if (active) recGear.checked = false }   // collapse + lock settings during recording
        Component.onCompleted: _refresh()
        Timer {
            interval: 1000; repeat: true; running: recRow.active
            onTriggered: recRow._refresh()
        }

        function _fmtDur(s) {
            var m = Math.floor(s / 60), ss = s % 60
            return (m < 10 ? "0" : "") + m + ":" + (ss < 10 ? "0" : "") + ss
        }
        function _fmtSize(b) {
            if (b < 1024) return b + " B"
            if (b < 1048576) return (b / 1024).toFixed(1) + " KB"
            return (b / 1048576).toFixed(1) + " MB"
        }
        function _activePath() {
            if (core.loggingKlf) return core.klfLogFilePath()
            if (core.loggingCsv) return core.csvLogFilePath()
            return ""
        }

        Column {
            id: recCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.leftMargin: Tokens.spaceXs; anchors.rightMargin: Tokens.spaceXs; anchors.topMargin: Tokens.spaceXs
            spacing: Tokens.spaceXs

            RowLayout {
                width: parent.width
                height: Tokens.controlHMd
                spacing: Tokens.spaceXs

                IconBtn {
                    id: recGear
                    checkable: true
                    visible: !recRow.active                 // hidden while recording
                    iconSource: "qrc:/icons/ui/settings.svg"
                    toolTipText: qsTr("Recording settings")
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: Tokens.controlHMd; Layout.preferredHeight: Tokens.controlHMd
                }

                Item {
                    id: marqueeClip
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    readonly property int pad: Tokens.spaceMd

                    Text {
                        id: recStatus
                        anchors.verticalCenter: parent.verticalCenter
                        text: recRow.active ? recRow._activePath()
                              : (store.recordKlf && store.recordCsv ? qsTr("Press REC to record KLF and CSV logs")
                                 : store.recordCsv ? qsTr("Press REC to record CSV log")
                                 : qsTr("Press REC to record KLF log"))
                        color: recRow.active ? "#FFFFFF" : AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        readonly property bool overflow: (width + 2 * marqueeClip.pad) > marqueeClip.width
                        readonly property real leftEnd: marqueeClip.width - width - marqueeClip.pad   // symmetric right gap; fade is off at this end so the tail stays readable
                        x: marqueeClip.pad
                        onOverflowChanged: if (!overflow) x = marqueeClip.pad
                        // ping-pong: pause right → glide left → pause left → glide back (same speed)
                        SequentialAnimation on x {
                            running: recStatus.overflow
                            loops: Animation.Infinite
                            PauseAnimation { duration: 1500 }
                            NumberAnimation { to: recStatus.leftEnd; duration: Math.max(1500, recStatus.width * 6); easing.type: Easing.InOutSine }
                            PauseAnimation { duration: 1500 }
                            NumberAnimation { to: marqueeClip.pad; duration: Math.max(1500, recStatus.width * 6); easing.type: Easing.InOutSine }
                        }
                    }

                    // edge fade-out — only on the side where text is still hidden
                    Rectangle {
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                        width: marqueeClip.pad * 2
                        visible: recStatus.overflow && recStatus.x < marqueeClip.pad - 1   // start scrolled past left edge
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: recRow.color }
                            GradientStop { position: 1.0; color: "transparent" }
                        }
                    }
                    Rectangle {
                        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                        width: marqueeClip.pad * 2
                        visible: recStatus.overflow && recStatus.x > recStatus.leftEnd + 1   // tail still past right edge
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 1.0; color: recRow.color }
                        }
                    }
                }

                Text {
                    visible: recRow.active
                    text: recRow._fmtSize(recRow.recBytes) + "  •  " + recRow._fmtDur(recRow.recSeconds)
                    color: "#FFFFFF"
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: Tokens.spaceXxs
                }

                KButton {
                    id: recBtn
                    checkable: true
                    checked: recRow.active                  // follows real recording state (no race)
                    text: recRow.active ? qsTr("■ STOP") : qsTr("● REC")
                    fontPixelSize: Tokens.fontSm
                    normalBg: AppPalette.controlRaised
                    hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
                    checkedBg: "#B91C1C"; checkedBorder: "#EF4444"
                    Layout.preferredWidth: recBtn.implicitWidth
                    Layout.preferredHeight: Tokens.controlHMd
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: {
                        store.setRecording(!recRow.active)
                        checked = Qt.binding(function() { return recRow.active })   // click toggled it; rebind to truth
                    }
                }
            }

            Column {
                visible: recGear.checked
                onVisibleChanged: if (visible) logPathInput.syncFromStore()
                width: parent.width
                spacing: Tokens.spaceXs
                topPadding: Tokens.spaceXxs

                Text {
                    text: qsTr("Log folder:")
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontXs
                }

                Row {
                    width: parent.width
                    spacing: Tokens.spaceSm

                    Rectangle {
                        width: logBrowseBtn.visible ? parent.width - logBrowseBtn.width - parent.spacing : parent.width
                        height: Tokens.controlHMd
                        radius: Tokens.radiusSm
                        color: AppPalette.bg
                        border.width: logPathInput.activeFocus ? 1 : Tokens.cardBorderWidth
                        border.color: logPathInput.activeFocus ? AppPalette.accentBorder : AppPalette.border
                        TextInput {
                            id: logPathInput
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.spaceSm; anchors.rightMargin: Tokens.spaceSm
                            verticalAlignment: TextInput.AlignVCenter
                            clip: true
                            readOnly: Qt.platform.os === "android"   // Android: fixed default dir, no manual path
                            activeFocusOnTab: !readOnly
                            selectByMouse: !readOnly
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontSm
                            // Show the effective save location — the custom path, or the
                            // default (Documents/KoggerApp/logs) when none is set.
                            function syncFromStore() {
                                if (activeFocus) return
                                var def = (typeof core !== "undefined" && core) ? core.logDirectory() : ""
                                text = (store && store.recordFolder && store.recordFolder.length) ? store.recordFolder : def
                            }
                            Component.onCompleted: syncFromStore()
                            onEditingFinished: if (store) store.recordFolder = text.trim()
                            TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: logPathInput.selectAll() }
                            Connections {
                                target: store
                                function onRecordFolderChanged() { logPathInput.syncFromStore() }
                            }
                        }
                    }

                    KButton {
                        id: logBrowseBtn
                        visible: Qt.platform.os !== "android"   // Android: fixed default dir, no folder picker
                        text: qsTr("Browse…")
                        normalBg: AppPalette.controlRaised
                        hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
                        fontPixelSize: Tokens.fontSm
                        height: Tokens.controlHMd
                        width: Math.round(96 * AppPalette.scale)
                        onClicked: {
                            core.setLogDirectory(store.recordFolder)            // sync selection (empty = default)
                            logFolderDialog.currentFolder = core.logDirectoryUrl()  // existing dir as start location
                            logFolderDialog.open()
                        }
                    }
                }

                FolderDialog {
                    id: logFolderDialog
                    title: qsTr("Select log folder")
                    onAccepted: {
                        var p = connectionViewer.urlSource("" + selectedFolder)   // url → local path (strips file://)
                        store.recordFolder = p
                        core.setLogDirectory(p)
                    }
                }

                KSwitch {
                    width: parent.width
                    text: qsTr("KLF")
                    backgroundColor: AppPalette.bg   // recessed on the card recording strip
                    checked: store.recordKlf
                    onToggled: {
                        if (!checked && !store.recordCsv) { checked = Qt.binding(function() { return store.recordKlf }); return }   // keep at least one type
                        store.recordKlf = checked
                    }
                }
                KSwitch {
                    width: parent.width
                    text: qsTr("CSV")
                    backgroundColor: AppPalette.bg   // recessed on the card recording strip
                    checked: store.recordCsv
                    onToggled: {
                        if (!checked && !store.recordKlf) { checked = Qt.binding(function() { return store.recordCsv }); return }
                        store.recordCsv = checked
                    }
                }
            }
        }
    }

    // ── CSV import panel ──────────────────────────────────────────────────

    Column {
        visible: importCheck.checked
        width: parent.width; spacing: 6

        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

        Row {
            width: parent.width; height: Tokens.controlHMd - Tokens.spaceXxs; spacing: Tokens.spaceMd
            Text { text: qsTr("Separator:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            Rectangle {
                width: Math.round(100 * AppPalette.scale); height: Math.round(26 * AppPalette.scale); radius: Tokens.radiusSm; color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
                anchors.verticalCenter: parent.verticalCenter
                ComboBox {
                    id: separatorCombo; anchors.fill: parent; model: ["Comma", "Tab", "Space", "SemiColon"]; font.pixelSize: Tokens.fontXs
                    focusPolicy: Qt.StrongFocus
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    contentItem: Text { leftPadding: 6; text: separatorCombo.displayText; color: AppPalette.text; font.pixelSize: Tokens.fontXs; verticalAlignment: Text.AlignVCenter }
                    Settings { category: "main/csvImport"; property alias separatorCombo: separatorCombo.currentIndex }
                }
            }
            Text { text: qsTr("Row:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: firstRow; value: 1; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVfirstRow: firstRow.value } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd - Tokens.spaceXxs; spacing: Tokens.spaceMd
            SmallCheck {
                id: timeEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { category: "main/csvImport"; property alias importCSVtimeEnable: timeEnable.checked }
            }
            Text { text: qsTr("Time col:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: timeColumn; value: 6; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVtimeColumn: timeColumn.value } }
            Rectangle {
                width: Math.round(100 * AppPalette.scale); height: Math.round(26 * AppPalette.scale); radius: Tokens.radiusSm; color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
                anchors.verticalCenter: parent.verticalCenter
                ComboBox {
                    id: utcGpsCombo; anchors.fill: parent; model: ["UTC time", "GPS time"]; font.pixelSize: Tokens.fontXs
                    focusPolicy: Qt.StrongFocus
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    contentItem: Text { leftPadding: 6; text: utcGpsCombo.displayText; color: AppPalette.text; font.pixelSize: Tokens.fontXs; verticalAlignment: Text.AlignVCenter }
                    Settings { category: "main/csvImport"; property alias utcGpsCombo: utcGpsCombo.currentIndex }
                }
            }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd - Tokens.spaceXxs; spacing: Tokens.spaceMd
            SmallCheck {
                id: latLonEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { category: "main/csvImport"; property alias importCSVlatLonEnable: latLonEnable.checked }
            }
            Text { text: qsTr("Lat/Lon/Alt:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: latColumn;  value: 2; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVlatColumn:  latColumn.value  } }
            CsvSpin { id: lonColumn;  value: 3; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVlonColumn:  lonColumn.value  } }
            CsvSpin { id: altColumn;  value: 4; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSValtColumn:  altColumn.value  } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd - Tokens.spaceXxs; spacing: Tokens.spaceMd
            SmallCheck {
                id: xyzEnable; checked: true; anchors.verticalCenter: parent.verticalCenter
                Settings { category: "main/csvImport"; property alias importCSVxyzEnable: xyzEnable.checked }
            }
            Text { text: qsTr("NEU:"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            CsvSpin { id: northColumn; value: 2; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVnorthColumn: northColumn.value } }
            CsvSpin { id: eastColumn;  value: 3; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVeastColumn:  eastColumn.value  } }
            CsvSpin { id: upColumn;    value: 4; from: 1; to: 100; anchors.verticalCenter: parent.verticalCenter; Settings { category: "main/csvImport"; property alias importCSVupColumn:    upColumn.value    } }
        }

        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

            Rectangle {
                width: parent.width - Math.round(50 * AppPalette.scale) - Tokens.spaceSm; height: Tokens.controlHMd; radius: Tokens.radiusMd
                color: AppPalette.bg; border.width: 1; border.color: importPathText.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: importPathText
                    activeFocusOnTab: true
                    anchors.fill: parent; anchors.margins: 8
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: importPathText.selectAll() }
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text; font.pixelSize: Tokens.fontSm; clip: true
                    Text { visible: !importPathText.text.length; text: qsTr("CSV path..."); color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
                    Keys.onPressed: function(e) {
                        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter)
                            importTrackFileDialog.openCSV()
                    }
                }
            }

            KButton {
                width: Math.round(50 * AppPalette.scale); height: Tokens.controlHMd; text: "..."; fontPixelSize: Tokens.fontSm
                onClicked: {
                    importTrackFileDialog.currentFolder = connectionViewer.lastImportTrackFolder
                    importTrackFileDialog.open()
                }

                FileDialog {
                    id: importTrackFileDialog
                    title: qsTr("Please choose a file")
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

    // ── Device tabs ───────────────────────────────────────────────────────

    Column {
        visible: devList.length > 0
        width: parent.width
        spacing: Tokens.spaceSm

        Text {
            text: qsTr("Devices")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm; font.bold: true
        }

        Flow {
            width: parent.width; spacing: Tokens.spaceSm

            Repeater {
                model: devList
                delegate: KButton {
                    required property var modelData
                    required property int index
                    text: modelData ? (modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]") : qsTr("Undefined")
                    height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm
                    checkable: true
                    checked: store && store.activeDeviceIndex === index
                    checkedBorder: AppPalette.accentBorder
                    visible: modelData ? (modelData.devType !== 0) : false
                    onClicked: {
                        if (store)
                            store.setActiveDeviceIndex(index)
                        checked = Qt.binding(function() { return !!(store && store.activeDeviceIndex === index) })
                    }
                }
            }
        }

        // Visual breathing room between device tabs and the settings card below.
        Item { width: 1; height: Tokens.spaceSm }
    }

    // ── Factory mode ──────────────────────────────────────────────────────

    Row {
        visible: core.isFactoryMode; width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

        KButton {
            text: qsTr("Flash Firmware"); height: Tokens.controlHMd; fontPixelSize: Tokens.fontSm
            onClicked: core.connectOpenedLinkAsFlasher(flasherPnText.text)
        }

        IconBtn {
            id: flasherDataRefresh; checkable: true
            iconSource: "qrc:/icons/ui/refresh.svg"; width: Tokens.controlHMd; height: Tokens.controlHMd
            onToggled: function(v) { if (!v) flasherDataInput.text = "" }
        }
    }

    Row {
        visible: flasherDataRefresh.checked && core.isFactoryMode
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

        Rectangle {
            width: parent.width - Tokens.controlHLg; height: Tokens.controlHMd; radius: Tokens.radiusMd
            color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
            TextInput {
                id: flasherDataInput
                activeFocusOnTab: true
                anchors.fill: parent; anchors.margins: 8
                verticalAlignment: TextInput.AlignVCenter; color: AppPalette.text; font.pixelSize: Tokens.fontSm
                onVisibleChanged: if (visible) focus = true
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_download.svg"; width: Tokens.controlHMd; height: Tokens.controlHMd
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

        Text { text: qsTr("Part Number:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd; anchors.verticalCenter: parent.verticalCenter }

        Rectangle {
            width: parent.width - 92 - 8; height: 30; radius: 6
            color: AppPalette.bg; border.width: 1; border.color: AppPalette.border
            TextInput {
                id: flasherPnText
                activeFocusOnTab: true
                anchors.fill: parent; anchors.margins: 8
                verticalAlignment: TextInput.AlignVCenter; color: AppPalette.text; font.pixelSize: Tokens.fontSm
                Settings { category: "main/csvImport"; property alias flasherPartNumber: flasherPnText.text }
            }
        }
    }

    Text {
        visible: core.isFactoryMode && FLASHER_STATE
        text: core.isFactoryMode && FLASHER_STATE ? core.flasherTextInfo : ""
        color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; width: parent.width; wrapMode: Text.WordWrap
    }

    // ── Device settings ───────────────────────────────────────────────────

    DeviceSettingsPage {
        id: deviceSettingsAnchor
        visible: connectionViewer.dev !== null
        width: parent.width
        dev: connectionViewer.dev
    }

    Timer {
        id: scrollToDeviceSettingsTimer
        interval: 240
        repeat: false
        onTriggered: {
            connectionViewer._scrollToDeviceSettings()
            if (connectionViewer.store)
                connectionViewer.store.deviceSettingsScrollPending = false
        }
    }

    function _maybeScrollToDevice() {
        if (connectionViewer.store && connectionViewer.store.deviceSettingsScrollPending === true)
            scrollToDeviceSettingsTimer.restart()
    }

    Connections {
        target: connectionViewer.store
        ignoreUnknownSignals: true
        function onDeviceSettingsScrollPendingChanged() {
            connectionViewer._maybeScrollToDevice()
        }
    }

    function _findAncestorFlickable() {
        var item = connectionViewer.parent
        while (item) {
            if (item.contentY !== undefined
                    && item.contentHeight !== undefined
                    && item.contentWidth !== undefined)
                return item
            item = item.parent
        }
        return null
    }

    function _scrollToDeviceSettings() {
        if (!deviceSettingsAnchor || !deviceSettingsAnchor.visible)
            return
        var flick = _findAncestorFlickable()
        if (!flick) return

        var topInContent = deviceSettingsAnchor.mapToItem(flick.contentItem, 0, 0).y
        var target = Math.max(0, topInContent - Tokens.spaceLg)
        target = Math.min(target, Math.max(0, flick.contentHeight - flick.height))
        if (Math.abs(target - flick.contentY) < 0.5) return

        scrollToDeviceAnim.target = flick
        scrollToDeviceAnim.from = flick.contentY
        scrollToDeviceAnim.to = target
        scrollToDeviceAnim.restart()
    }

    NumberAnimation {
        id: scrollToDeviceAnim
        property: "contentY"
        duration: 240
        easing.type: Easing.OutCubic
    }

    property int _pendingExpandIndex: -1

    Connections {
        target: linkManagerWrapper
        function onLinkCreatedInteractively(uuid) {
            linkList.expandedUuid = String(uuid)
            connectionViewer._requestExpandScroll(linkRepeater.count - 1)
        }
    }

    Timer {
        id: expandScrollTimer
        interval: Anim.disclosureMs + 30
        onTriggered: connectionViewer._scrollExpandedIntoView()
    }

    NumberAnimation {
        id: rowScrollAnim
        property: "contentY"
        duration: 240
        easing.type: Easing.OutCubic
    }

    function _requestExpandScroll(idx) {
        _pendingExpandIndex = idx
        expandScrollTimer.restart()
    }

    function _scrollExpandedIntoView() {
        var idx = _pendingExpandIndex
        if (idx < 0)
            return

        var item = linkRepeater.itemAt(idx)
        var flick = _findAncestorFlickable()
        if (!item || !flick)
            return

        var pad = Tokens.spaceSm
        var rowTop = item.mapToItem(flick.contentItem, 0, 0).y
        var rowH = item.height
        var viewTop = flick.contentY
        var viewBottom = flick.contentY + flick.height

        var target = flick.contentY
        if (rowH >= flick.height || rowTop < viewTop)
            target = rowTop - pad
        else if (rowTop + rowH > viewBottom)
            target = rowTop + rowH - flick.height + pad

        target = Math.max(0, Math.min(target, Math.max(0, flick.contentHeight - flick.height)))
        if (Math.abs(target - flick.contentY) < 0.5)
            return

        rowScrollAnim.stop()
        rowScrollAnim.target = flick
        rowScrollAnim.from = flick.contentY
        rowScrollAnim.to = target
        rowScrollAnim.start()
    }

}

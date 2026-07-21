import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import kqml_types 1.0

Popup {
    id: hotkeysDialog

    // Injected by the host (e.g. AppSettingsPage's Loader). Used to register
    // this dialog as the topmost modal so MainWindow's global Esc closes it
    // instead of unwinding the settings panel.
    property var store: null

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: Math.round(14 * AppPalette.scale)

    width:  Math.min(Math.round(720 * AppPalette.scale), parent ? parent.width  * 0.95 : 720)
    height: Math.min(Math.round(640 * AppPalette.scale), parent ? parent.height * 0.90 : 640)

    closePolicy: listeningIndex >= 0 ? Popup.NoAutoClose
                                     : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int listeningIndex: -1

    // debounce state for parameter editing
    property string _pendingFn:     ""
    property int    _pendingSc:     0
    property int    _pendingParam:  0
    property string _conflictText:  ""

    // effective list content width (leaves room for scrollbar)
    readonly property real _listW: listView.width - Math.round(14 * AppPalette.scale)

    readonly property int _colKey:   Math.round(128 * AppPalette.scale)
    readonly property int _colParam: Math.round(88 * AppPalette.scale)
    readonly property int _rowH:     Math.round(34 * AppPalette.scale)

    function groupName(g) {
        switch (g) {
            case "Application": return qsTr("Application")
            case "Echogram":    return qsTr("Echograms")
            case "3D":          return qsTr("3D")
            case "Mosaic":      return qsTr("Mosaic")
            case "Surface":     return qsTr("Isobaths")
            default:            return g
        }
    }

    Timer {
        id: paramSaveTimer
        interval: 400
        repeat: false
        onTriggered: {
            if (hotkeysController && hotkeysDialog._pendingFn !== "")
                hotkeysController.updateHotkey(hotkeysDialog._pendingFn,
                                               hotkeysDialog._pendingSc,
                                               hotkeysDialog._pendingParam)
        }
    }

    // Desktop-only fixed navigation keys (not rebindable) — shown read-only at
    // the bottom of the "Application" group. The actual scrolling is handled by
    // MainWindow Shortcuts; these rows are for discoverability only.
    readonly property bool _isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    function _fixedAppRows() {
        if (_isMobile)
            return []
        return [
            { group: "Application", fixed: true, scanCode: 0, parameter: 0, functionName: "",
              keyName: "PgUp",   description: qsTr("Scroll up") },
            { group: "Application", fixed: true, scanCode: 0, parameter: 0, functionName: "",
              keyName: "PgDown", description: qsTr("Scroll down") },
            { group: "Application", fixed: true, scanCode: 0, parameter: 0, functionName: "",
              keyName: "Home",   description: qsTr("Scroll to start") },
            { group: "Application", fixed: true, scanCode: 0, parameter: 0, functionName: "",
              keyName: "End",    description: qsTr("Scroll to end") }
        ]
    }

    // Normalize a C++ display entry to the SAME role set as the fixed rows.
    // ListModel derives its roles from the first appended item, so every row
    // must carry `fixed` (else model.fixed is undefined for the fixed rows).
    function _normalize(e) {
        return { group: e.group, scanCode: e.scanCode, keyName: e.keyName,
                 functionName: e.functionName, parameter: e.parameter,
                 description: e.description, fixed: false }
    }

    // hotkeysDisplayList with the fixed rows spliced in at the end of the
    // "Application" group (so they sit under the single Application header).
    function _composedList() {
        var out = []
        var inserted = false
        for (var i = 0; i < hotkeysDisplayList.length; ++i) {
            var e = hotkeysDisplayList[i]
            if (!inserted && e.group !== "Application") {
                var fx = _fixedAppRows()
                for (var j = 0; j < fx.length; ++j) out.push(fx[j])
                inserted = true
            }
            out.push(_normalize(e))
        }
        if (!inserted) {
            var fx2 = _fixedAppRows()
            for (var k = 0; k < fx2.length; ++k) out.push(fx2[k])
        }
        return out
    }

    // Full rebuild — used on open only.
    function rebuildModel() {
        var list = _composedList()
        listModel.clear()
        for (var i = 0; i < list.length; ++i)
            listModel.append(list[i])
        listeningIndex = -1
    }

    // In-place update — preserves scroll position.
    function refreshModel() {
        var list = _composedList()
        for (var i = 0; i < list.length && i < listModel.count; ++i)
            listModel.set(i, list[i])
        listeningIndex = -1
    }

    onOpened: {
        if (listModel.count === 0) {
            rebuildModel()
            _pendingRestore = true
            Qt.callLater(_tryRestore)
        } else {
            refreshModel()
        }
        keyCapture.forceActiveFocus()
        if (store)
            store.activeHotkeysDialog = hotkeysDialog
    }

    onClosed: {
        _saveScroll()
        if (paramSaveTimer.running) {
            paramSaveTimer.stop()
            if (hotkeysController && _pendingFn !== "")
                hotkeysController.updateHotkey(_pendingFn, _pendingSc, _pendingParam)
        }
        if (store && store.activeHotkeysDialog === hotkeysDialog)
            store.activeHotkeysDialog = null
    }

    // Belt-and-braces: if the host Loader unloads while the dialog is open,
    // onClosed never fires but the QObject IS destroyed. Without this hook
    // store.activeHotkeysDialog would keep a dangling reference and the next
    // Esc would invoke .close() on a deleted object.
    Component.onDestruction: {
        _saveScroll()
        if (store && store.activeHotkeysDialog === hotkeysDialog)
            store.activeHotkeysDialog = null
    }

    property bool _pendingRestore: false

    Settings {
        id: hkSettings
        category: "main/ui"
        property real hotkeysScrollY: 0
    }

    NumberAnimation {
        id: scrollTopAnim
        target: listView
        property: "contentY"
        duration: 220
        easing.type: Easing.OutCubic
    }

    // Keyboard scrolling (kind: "up"/"down"/"top"/"bottom") — driven by MainWindow.
    function kbdScroll(kind) {
        var f = listView
        if (!f)
            return
        if ((kind === "up" || kind === "down") && f.contentHeight <= f.height)
            return
        var top  = f.originY
        var maxY = top + Math.max(0, f.contentHeight - f.height)
        var y = kind === "top"    ? top
              : kind === "bottom" ? maxY
              : kind === "down"   ? Math.min(maxY, f.contentY + f.height * 0.9)
              :                     Math.max(top, f.contentY - f.height * 0.9)
        hotkeysDialog._pendingRestore = false
        kbdListAnim.stop()
        kbdListAnim.from = f.contentY
        kbdListAnim.to = y
        kbdListAnim.start()
    }

    NumberAnimation {
        id: kbdListAnim
        target: listView
        property: "contentY"
        duration: 180
        easing.type: Easing.OutCubic
    }

    function _saveScroll() {
        if (!_pendingRestore)
            hkSettings.hotkeysScrollY = listView.contentY
    }
    function _tryRestore() {
        if (!_pendingRestore || listView.contentHeight <= 0)
            return
        var maxY = Math.max(0, listView.contentHeight - listView.height)
        listView.contentY = Math.max(0, Math.min(hkSettings.hotkeysScrollY, maxY))
        _pendingRestore = false
    }

    Connections {
        target: hotkeysController
        function onHotkeysUpdated() { hotkeysDialog.refreshModel() }
    }

    ListModel { id: listModel }

    background: Rectangle {
        color: AppPalette.bgDeep
        border.color: AppPalette.border
        border.width: 1
        radius: 10
    }

    contentItem: Item {
        id: keyCapture
        focus: true

        // ── key capture when listening ────────────────────────────────────────
        Keys.onPressed: function(event) {
            // Esc: cancel listening if active, otherwise close the dialog.
            if (event.key === Qt.Key_Escape) {
                if (hotkeysDialog.listeningIndex >= 0) {
                    hotkeysDialog.listeningIndex = -1
                    hotkeysDialog._conflictText = ""
                } else {
                    hotkeysDialog.close()
                }
                event.accepted = true
                return
            }

            // Not listening: PgUp/PgDn/Home/End scroll the list (a modal Popup
            // blocks the app-level scroll shortcuts, so handle them here).
            if (hotkeysDialog.listeningIndex < 0) {
                if (!hotkeysDialog._isMobile
                        && (event.key === Qt.Key_PageDown || event.key === Qt.Key_PageUp
                            || event.key === Qt.Key_Home || event.key === Qt.Key_End)) {
                    hotkeysDialog.kbdScroll(event.key === Qt.Key_PageDown ? "down"
                                          : event.key === Qt.Key_PageUp   ? "up"
                                          : event.key === Qt.Key_Home     ? "top"
                                          :                                 "bottom")
                    event.accepted = true
                    return
                }
                event.accepted = false
                return
            }

            if (event.key === Qt.Key_Control || event.key === Qt.Key_Shift ||
                event.key === Qt.Key_Alt    || event.key === Qt.Key_Meta) {
                event.accepted = true
                return
            }

            var sc = event.nativeScanCode
            for (var i = 0; i < listModel.count; ++i) {
                if (i === hotkeysDialog.listeningIndex) continue
                if (listModel.get(i).scanCode === sc) {
                    hotkeysDialog._conflictText = qsTr("Already used by: %1").arg(listModel.get(i).description)
                    event.accepted = true
                    return
                }
            }

            hotkeysDialog._conflictText = ""
            var item = listModel.get(hotkeysDialog.listeningIndex)
            if (hotkeysController)
                hotkeysController.updateHotkey(item.functionName, sc, item.parameter)
            event.accepted = true
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            // ── Title ─────────────────────────────────────────────────────────
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Key bindings")
                color: AppPalette.text
                font.pixelSize: Math.round(16 * AppPalette.scale)
                font.bold: true
            }

            // ── Column headers ────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                height: Math.round(28 * AppPalette.scale)
                radius: Tokens.radiusMd
                color: AppPalette.headerBg
                border.color: AppPalette.border
                border.width: Tokens.cardBorderWidth

                Row {
                    anchors.fill: parent

                    Text {
                        width: hotkeysDialog._colKey; height: parent.height
                        text: qsTr("Key")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Rectangle { width: 1; height: parent.height; color: AppPalette.border }
                    Text {
                        width: hotkeysDialog._colParam; height: parent.height
                        text: qsTr("Parameter")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Rectangle { width: 1; height: parent.height; color: AppPalette.border }
                    Text {
                        width: parent.width - hotkeysDialog._colKey - hotkeysDialog._colParam - 2
                        height: parent.height
                        text: qsTr("Action")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: Tokens.spaceLg
                    }
                }
            }

            // ── List ──────────────────────────────────────────────────────────
            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: listModel
                spacing: 2
                ScrollBar.vertical: ScrollBar {
                    active: listView.moving || hovered || pressed || scrollTopAnim.running || kbdListAnim.running
                }

                onContentHeightChanged: hotkeysDialog._tryRestore()
                onMovementStarted: hotkeysDialog._pendingRestore = false

                section.property: "group"
                section.delegate: Item {
                    width: hotkeysDialog._listW
                    height: 26
                    Row {
                        anchors.fill: parent
                        spacing: 8
                        Rectangle {
                            width: Math.max(0, (parent.width - sectionLabel.implicitWidth - 16) / 2)
                            height: 1
                            anchors.verticalCenter: parent.verticalCenter
                            color: AppPalette.border
                        }
                        Text {
                            id: sectionLabel
                            text: hotkeysDialog.groupName(section)
                            color: AppPalette.textSecond
                            font.pixelSize: Math.round(11 * AppPalette.scale); font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Rectangle {
                            width: Math.max(0, (parent.width - sectionLabel.implicitWidth - 16) / 2)
                            height: 1
                            anchors.verticalCenter: parent.verticalCenter
                            color: AppPalette.border
                        }
                    }
                }

                delegate: Item {
                    id: row
                    width: hotkeysDialog._listW
                    implicitHeight: Math.max(hotkeysDialog._rowH, descText.implicitHeight + 10)
                    height: implicitHeight

                    readonly property bool listening: hotkeysDialog.listeningIndex === index
                    readonly property int  cellH: hotkeysDialog._rowH - 6
                    readonly property real cellY: Math.floor((height - cellH) / 2)
                    readonly property bool param3D: model.group === "3D"
                    readonly property int  paramMax: param3D ? 50 : 99999

                    // hover background
                    Rectangle {
                        anchors.fill: parent
                        color: rowMouse.containsMouse ? AppPalette.cardHover : "transparent"
                        radius: 4
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }

                    // -- key badge --
                    Rectangle {
                        x: 4
                        y: row.cellY
                        width: hotkeysDialog._colKey - 8
                        height: row.cellH
                        radius: 6
                        clip: true
                        color: row.listening ? AppPalette.accentBg : AppPalette.card
                        border.color: row.listening ? AppPalette.accentBorder : AppPalette.border
                        border.width: row.listening ? 1 : Tokens.cardBorderWidth

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 8
                            horizontalAlignment: Text.AlignHCenter
                            text: row.listening ? qsTr("…") : (model.keyName || "")
                            color: (model.fixed === true) ? AppPalette.textSecond : AppPalette.text
                            font.pixelSize: Math.round(16 * AppPalette.scale)
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !(model.fixed === true)
                            cursorShape: (model.fixed === true) ? Qt.ArrowCursor : Qt.PointingHandCursor
                            onClicked: {
                                hotkeysDialog.listeningIndex = row.listening ? -1 : index
                                hotkeysDialog._conflictText = ""
                                keyCapture.forceActiveFocus()
                            }
                        }
                    }

                    // -- parameter badge (editable, centered like key badge) --
                    Rectangle {
                        x: hotkeysDialog._colKey + 4
                        y: row.cellY
                        width: hotkeysDialog._colParam - 8
                        height: row.cellH
                        visible: model.parameter > 0
                        radius: 6
                        clip: true
                        color: AppPalette.card
                        border.color: paramInput.activeFocus ? AppPalette.accentBorder : AppPalette.border
                        border.width: paramInput.activeFocus ? 1 : Tokens.cardBorderWidth

                        TextInput {
                            id: paramInput
                            anchors.centerIn: parent
                            width: parent.width - 8
                            horizontalAlignment: TextInput.AlignHCenter
                            color: AppPalette.text
                            font.pixelSize: Math.round(16 * AppPalette.scale)
                            selectionColor: AppPalette.accentBg
                            text: model.parameter > 0 ? model.parameter.toString() : ""
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: RegularExpressionValidator {
                                regularExpression: row.param3D ? /^([1-9]|[1-4][0-9]|50)$/ : /^[1-9][0-9]{0,4}$/
                            }
                            onTextChanged: {
                                if (!acceptableInput) return
                                var val = parseInt(text)
                                if (val < 1 || val > row.paramMax) return
                                hotkeysDialog._pendingFn    = model.functionName
                                hotkeysDialog._pendingSc    = model.scanCode
                                hotkeysDialog._pendingParam = val
                                paramSaveTimer.restart()
                            }
                            onActiveFocusChanged: {
                                if (!activeFocus && (text.length === 0 || !acceptableInput))
                                    text = model.parameter > 0 ? model.parameter.toString() : ""
                            }
                        }
                    }

                    // -- description --
                    Text {
                        id: descText
                        x: hotkeysDialog._colKey + hotkeysDialog._colParam + 10
                        width: row.width - x - 4
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.description || ""
                        color: AppPalette.text
                        font.pixelSize: Math.round(13 * AppPalette.scale)
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ── Bottom bar ────────────────────────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    leftPadding: 4
                    font.pixelSize: Math.round(12 * AppPalette.scale)
                    text: hotkeysDialog._conflictText !== ""
                          ? hotkeysDialog._conflictText
                          : hotkeysDialog.listeningIndex >= 0
                            ? qsTr("Press any key  •  click again to cancel")
                            : qsTr("Click a key to reassign")
                    color: hotkeysDialog._conflictText !== ""
                           ? AppPalette.dangerText
                           : hotkeysDialog.listeningIndex >= 0
                             ? AppPalette.accentBorder
                             : AppPalette.textSecond
                    elide: Text.ElideRight
                }

                KCircleIconButton {
                    Layout.preferredWidth: Math.round(Tokens.controlHMd * 1.4)
                    Layout.preferredHeight: Tokens.controlHMd
                    visible: listView.contentY > 1
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover
                    borderColor: AppPalette.border
                    borderWidth: Tokens.cardBorderWidth
                    iconSource: "qrc:/icons/ui/chevron-up.svg"
                    iconTintColor: AppPalette.text
                    toolTipText: qsTr("Scroll to top")
                    onClicked: {
                        hotkeysDialog._pendingRestore = false
                        scrollTopAnim.stop()
                        scrollTopAnim.from = listView.contentY
                        scrollTopAnim.to = listView.originY
                        scrollTopAnim.restart()
                    }
                }

                KButton {
                    width: 150
                    height: 30
                    text: qsTr("Reset to defaults")
                    onClicked: {
                        if (hotkeysController)
                            hotkeysController.resetToDefaults()
                    }
                }

                KButton {
                    width: 80
                    height: 30
                    text: qsTr("Close")
                    onClicked: hotkeysDialog.close()
                }
            }
        }
    }
}

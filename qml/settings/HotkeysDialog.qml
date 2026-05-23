import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
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
            case "Echogram":    return qsTr("Echogram")
            case "3D":          return qsTr("3D")
            case "Mosaic":      return qsTr("Mosaic")
            case "Surface":     return qsTr("Surface")
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

    // Full rebuild — used on open only.
    function rebuildModel() {
        listModel.clear()
        for (var i = 0; i < hotkeysDisplayList.length; ++i)
            listModel.append(hotkeysDisplayList[i])
        listeningIndex = -1
    }

    // In-place update — preserves scroll position.
    function refreshModel() {
        for (var i = 0; i < hotkeysDisplayList.length && i < listModel.count; ++i)
            listModel.set(i, hotkeysDisplayList[i])
        listeningIndex = -1
    }

    onOpened: {
        rebuildModel()
        keyCapture.forceActiveFocus()
        if (store)
            store.activeHotkeysDialog = hotkeysDialog
    }

    onClosed: {
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
        if (store && store.activeHotkeysDialog === hotkeysDialog)
            store.activeHotkeysDialog = null
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

            // Pass through anything else while not listening for a reassign.
            if (hotkeysDialog.listeningIndex < 0) {
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
                text: qsTr("Keyboard Shortcuts")
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
                border.width: 1

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
                ScrollBar.vertical: ScrollBar {}

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
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 8
                            horizontalAlignment: Text.AlignHCenter
                            text: row.listening ? qsTr("…") : (model.keyName || "")
                            color: AppPalette.text
                            font.pixelSize: Math.round(13 * AppPalette.scale)
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
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
                        border.width: 1

                        TextInput {
                            id: paramInput
                            anchors.centerIn: parent
                            width: parent.width - 8
                            horizontalAlignment: TextInput.AlignHCenter
                            color: AppPalette.text
                            font.pixelSize: Math.round(13 * AppPalette.scale)
                            selectionColor: AppPalette.accentBg
                            text: model.parameter > 0 ? model.parameter.toString() : ""
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 1; top: 99999 }
                            onTextChanged: {
                                if (!acceptableInput) return
                                var val = parseInt(text)
                                if (val < 1 || val > 99999) return
                                hotkeysDialog._pendingFn    = model.functionName
                                hotkeysDialog._pendingSc    = model.scanCode
                                hotkeysDialog._pendingParam = val
                                paramSaveTimer.restart()
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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: hotkeysDialog

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: 12

    width:  Math.min(720, parent ? parent.width  * 0.95 : 720)
    height: Math.min(600, parent ? parent.height * 0.90 : 600)

    closePolicy: listeningIndex >= 0 ? Popup.NoAutoClose
                                     : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int listeningIndex: -1

    // debounce state for parameter editing
    property string _pendingFn:     ""
    property int    _pendingSc:     0
    property int    _pendingParam:  0
    property string _conflictText:  ""

    // effective list content width (leaves room for scrollbar)
    readonly property real _listW: listView.width - 14

    readonly property int  _colKey:   120
    readonly property int  _colParam: 88

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

    // Full rebuild — used on open only
    function rebuildModel() {
        listModel.clear()
        for (var i = 0; i < hotkeysDisplayList.length; ++i)
            listModel.append(hotkeysDisplayList[i])
        listeningIndex = -1
    }

    // In-place update — preserves scroll position
    function refreshModel() {
        for (var i = 0; i < hotkeysDisplayList.length && i < listModel.count; ++i)
            listModel.set(i, hotkeysDisplayList[i])
        listeningIndex = -1
    }

    onOpened: {
        rebuildModel()
        keyCapture.forceActiveFocus()
    }

    onClosed: {
        if (paramSaveTimer.running) {
            paramSaveTimer.stop()
            if (hotkeysController && _pendingFn !== "")
                hotkeysController.updateHotkey(_pendingFn, _pendingSc, _pendingParam)
        }
    }

    Connections {
        target: hotkeysController
        function onHotkeysUpdated() { hotkeysDialog.refreshModel() }
    }

    ListModel { id: listModel }

    background: Rectangle {
        color: theme.menuBackColor
        border.color: theme.controlBorderColor
        border.width: 1
        radius: 2
    }

    contentItem: Item {
        id: keyCapture
        focus: true

        // ---- key capture when listening ----
        Keys.onPressed: function(event) {
            if (hotkeysDialog.listeningIndex < 0) return

            if (event.key === Qt.Key_Control || event.key === Qt.Key_Shift ||
                event.key === Qt.Key_Alt    || event.key === Qt.Key_Meta) {
                event.accepted = true
                return
            }

            // Check for conflict with another hotkey
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
                hotkeysController.updateHotkey(item.functionName,
                                               sc,
                                               item.parameter)
            event.accepted = true
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            // ---- title (ParamGroup style) ----
            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                Rectangle { Layout.fillWidth: true; height: 2; color: "#808080" }
                CText { text: qsTr("Keyboard Shortcuts") }
                Rectangle { Layout.fillWidth: true; height: 2; color: "#808080" }
            }

            // ---- column headers ----
            Item {
                Layout.fillWidth: true
                height: 20

                Row {
                    x: 0; width: hotkeysDialog._listW; height: parent.height

                    CText {
                        width: hotkeysDialog._colKey; height: parent.height
                        text: qsTr("Key"); small: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Rectangle { width: 1; height: parent.height; color: theme.controlBorderColor }
                    CText {
                        width: hotkeysDialog._colParam; height: parent.height
                        text: qsTr("Parameter"); small: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Rectangle { width: 1; height: parent.height; color: theme.controlBorderColor }
                    CText {
                        width: hotkeysDialog._listW - hotkeysDialog._colKey - hotkeysDialog._colParam - 2
                        height: parent.height
                        text: qsTr("Action"); small: true
                        leftPadding: 8
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: theme.controlBorderColor }

            // ---- list ----
            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: listModel
                ScrollBar.vertical: ScrollBar {}

                section.property: "group"
                section.delegate: Item {
                    width: hotkeysDialog._listW
                    height: 24
                    Row {
                        anchors.fill: parent
                        spacing: 8
                        Rectangle { width: (parent.width - sectionLabel.implicitWidth - 16) / 2; height: 2; anchors.verticalCenter: parent.verticalCenter; color: "#808080" }
                        CText { id: sectionLabel; text: hotkeysDialog.groupName(section); small: true; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: (parent.width - sectionLabel.implicitWidth - 16) / 2; height: 2; anchors.verticalCenter: parent.verticalCenter; color: "#808080" }
                    }
                }

                delegate: Item {
                    id: row
                    width: hotkeysDialog._listW
                    implicitHeight: Math.max(theme.controlHeight, descText.implicitHeight + 8)
                    height: implicitHeight

                    readonly property bool listening: hotkeysDialog.listeningIndex === index
                    readonly property int  cellH: theme.controlHeight - 4
                    readonly property real cellY: Math.floor((height - cellH) / 2)

                    // column separator lines (full row height)
                    Rectangle {
                        x: hotkeysDialog._colKey
                        width: 1; height: parent.height
                        color: theme.controlBorderColor
                        opacity: 0.5
                    }
                    Rectangle {
                        x: hotkeysDialog._colKey + 1 + hotkeysDialog._colParam
                        width: 1; height: parent.height
                        color: theme.controlBorderColor
                        opacity: 0.5
                    }

                    // -- key badge --
                    Rectangle {
                        x: 3
                        y: row.cellY
                        width: hotkeysDialog._colKey - 6
                        height: row.cellH
                        radius: 2
                        clip: true
                        color: row.listening ? theme.controlSolidBackColor : theme.controlBackColor
                        border.color: row.listening ? theme.controlSolidBorderColor : theme.controlBorderColor
                        border.width: 1

                        CText {
                            anchors.centerIn: parent
                            width: parent.width - 6
                            horizontalAlignment: Text.AlignHCenter
                            text: row.listening ? qsTr("…") : (model.keyName || "")
                            color: row.listening ? theme.textSolidColor : theme.textColor
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
                        x: hotkeysDialog._colKey + 1 + 3
                        y: row.cellY
                        width: hotkeysDialog._colParam - 6
                        height: row.cellH
                        visible: model.parameter > 0
                        radius: 2
                        clip: true
                        color: theme.controlBackColor
                        border.color: theme.controlBorderColor
                        border.width: 1

                        TextInput {
                            anchors.centerIn: parent
                            width: parent.width - 6
                            horizontalAlignment: TextInput.AlignHCenter
                            color: theme.textColor
                            font: theme.textFont
                            selectionColor: theme.controlSolidBackColor
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
                    CText {
                        id: descText
                        x: hotkeysDialog._colKey + 1 + hotkeysDialog._colParam + 1 + 8
                        width: hotkeysDialog._listW - x
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.description || ""
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: theme.controlBorderColor }

            // ---- bottom bar ----
            RowLayout {
                Layout.fillWidth: true

                CText {
                    Layout.fillWidth: true
                    small: true
                    leftPadding: 4
                    text: hotkeysDialog._conflictText !== ""
                          ? hotkeysDialog._conflictText
                          : hotkeysDialog.listeningIndex >= 0
                            ? qsTr("Press any key  •  click again to cancel")
                            : qsTr("Click a key to reassign")
                    color: hotkeysDialog._conflictText !== ""
                           ? "#e05050"
                           : hotkeysDialog.listeningIndex >= 0
                             ? theme.textSolidColor : theme.textColor
                }

                CButton {
                    width: 130
                    text: qsTr("Reset to defaults")
                    onClicked: {
                        if (hotkeysController)
                            hotkeysController.resetToDefaults()
                    }
                }

                CButton {
                    width: 80
                    text: qsTr("Close")
                    onClicked: hotkeysDialog.close()
                }
            }
        }
    }
}

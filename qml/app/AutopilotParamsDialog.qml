import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import kqml_types 1.0

Popup {
    id: dialog

    property var store: null

    readonly property var params: (typeof autopilotParams !== "undefined") ? autopilotParams : null
    readonly property var dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property bool online: !!params && params.online
    readonly property bool armed: !!dmw && dmw.pilotArmState > 0
    readonly property int cmdReboot: 246
    readonly property int rebootConfirmMs: 3000

    property string selectedName: ""
    property string selectedValue: ""
    property string selectedType: ""
    property string editError: ""
    property string message: ""
    property bool rebootConfirmPending: false

    readonly property string statusText: {
        if (!params) return ""
        if (!params.online) return params.count > 0 ? qsTr("No link to the autopilot; values may be out of date") : qsTr("No link to the autopilot")
        if (params.loading) return params.total > 0 ? qsTr("Loading %1 / %2").arg(params.received).arg(params.total) : qsTr("Requesting the list…")
        if (params.loadFailed) return qsTr("The autopilot did not answer the parameter request")
        if (params.stale) return qsTr("Values may be out of date; refresh")
        var text = qsTr("%1 parameters").arg(params.count)
        if (params.missing > 0) text += " · " + qsTr("not received: %1").arg(params.missing)
        if (params.pendingWrites > 0) text += " · " + qsTr("writing: %1").arg(params.pendingWrites)
        return text
    }

    function select(name, value, type) {
        selectedName = name
        selectedValue = value
        selectedType = type
        editField.text = value
        editError = ""
        editField.forceActiveFocus()
        editField.selectAll()
    }

    function clearSelection() {
        selectedName = ""
        selectedValue = ""
        selectedType = ""
        editError = ""
        editField.text = ""
        editField.focus = false
    }

    function writeSelected() {
        if (!params || !selectedName.length) return
        editError = params.validate(selectedName, editField.text)
        if (editError.length) return
        if (params.setValue(selectedName, editField.text))
            clearSelection()
        else
            editError = params.lastError
    }

    function requestReboot() {
        if (!params || !online || armed) return
        if (!rebootConfirmPending) {
            rebootConfirmPending = true
            rebootTimer.restart()
            return
        }
        rebootConfirmPending = false
        rebootTimer.stop()
        message = qsTr("Reboot requested")
        params.reboot()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: Math.round(14 * AppPalette.scale)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(Math.round(1000 * AppPalette.scale), parent ? parent.width * 0.95 : 1000)
    height: Math.min(Math.round(640 * AppPalette.scale), parent ? parent.height * 0.95 : 640)

    onOpened: {
        message = ""
        if (params && !params.loadFailed)
            params.ensureLoaded()
        if (store)
            store.activeHotkeysDialog = dialog
    }

    onClosed: {
        clearSelection()
        rebootConfirmPending = false
        if (store && store.activeHotkeysDialog === dialog)
            store.activeHotkeysDialog = null
    }

    Component.onDestruction: {
        if (store && store.activeHotkeysDialog === dialog)
            store.activeHotkeysDialog = null
    }

    background: Rectangle {
        color: AppPalette.bgDeep
        border.color: AppPalette.border
        border.width: 1
        radius: 10
    }

    Timer {
        id: rebootTimer
        interval: dialog.rebootConfirmMs
        onTriggered: dialog.rebootConfirmPending = false
    }

    Connections {
        target: dialog.params
        ignoreUnknownSignals: true
        function onWriteFailed(name, actual) {
            dialog.message = qsTr("%1 was not confirmed; the vehicle reports %2").arg(name).arg(actual)
        }
        function onWriteConfirmed(name, value) {
            dialog.message = qsTr("%1 = %2 written").arg(name).arg(value)
        }
        function onStateChanged() {
            if (dialog.opened && dialog.params.online && !dialog.params.loadFailed)
                dialog.params.ensureLoaded()
        }
    }

    Connections {
        target: dialog.dmw
        ignoreUnknownSignals: true
        function onAutopilotCommandAcked(command, result) {
            if (command !== dialog.cmdReboot) return
            if (result === 0)
                dialog.message = qsTr("The autopilot is rebooting")
            else if (result !== 5)
                dialog.message = qsTr("The autopilot refused the reboot (%1)").arg(result)
        }
    }

    component SearchField: Rectangle {
        id: searchField
        property alias text: input.text
        property string placeholder: ""
        implicitHeight: Tokens.controlHMd
        radius: Tokens.radiusMd
        color: AppPalette.bg
        border.width: input.activeFocus ? 1 : Tokens.cardBorderWidth
        border.color: input.activeFocus ? AppPalette.accentBorder : AppPalette.border

        TextInput {
            id: input
            anchors.fill: parent
            anchors.leftMargin: Tokens.spaceMd
            anchors.rightMargin: Tokens.spaceMd
            verticalAlignment: TextInput.AlignVCenter
            clip: true
            selectByMouse: true
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhUppercaseOnly
            Keys.onEscapePressed: { input.text = ""; input.focus = false }

            Text {
                visible: !input.text.length && !input.activeFocus
                anchors.verticalCenter: parent.verticalCenter
                text: searchField.placeholder
                color: AppPalette.textMuted
                font: input.font
            }
        }
    }

    Item {
        id: content
        anchors.fill: parent

        Text {
            id: titleText
            width: parent.width
            text: qsTr("Autopilot parameters")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontXl
            font.bold: true
        }

        Text {
            id: statusLine
            anchors.top: titleText.bottom
            anchors.topMargin: Tokens.spaceXs
            width: parent.width
            text: dialog.statusText
            color: dialog.params && (dialog.params.loadFailed || dialog.params.stale) ? AppPalette.dangerText : AppPalette.textSecond
            font.pixelSize: Tokens.fontMd
            elide: Text.ElideRight
        }

        Row {
            id: toolbar
            anchors.top: statusLine.bottom
            anchors.topMargin: Tokens.spaceMd
            width: parent.width
            spacing: Tokens.spaceSm

            SearchField {
                id: search
                width: toolbar.width - refreshButton.width - saveButton.width - loadButton.width - rebootButton.width - 4 * toolbar.spacing
                placeholder: qsTr("Search by name")
                onTextChanged: if (dialog.params) dialog.params.searchText = text
            }

            KButton {
                id: refreshButton
                height: Tokens.controlHMd
                width: Math.max(Math.round(110 * AppPalette.scale), implicitWidth)
                text: qsTr("Refresh")
                enabled: dialog.online && !!dialog.params && !dialog.params.loading
                onClicked: dialog.params.refresh()
            }

            KButton {
                id: saveButton
                height: Tokens.controlHMd
                width: Math.max(Math.round(110 * AppPalette.scale), implicitWidth)
                text: qsTr("Save…")
                enabled: !!dialog.params && dialog.params.count > 0
                onClicked: saveDialog.open()
            }

            KButton {
                id: loadButton
                height: Tokens.controlHMd
                width: Math.max(Math.round(110 * AppPalette.scale), implicitWidth)
                text: qsTr("Load…")
                enabled: dialog.online && !!dialog.params && dialog.params.loaded
                onClicked: openDialog.open()
            }

            KButton {
                id: rebootButton
                height: Tokens.controlHMd
                width: Math.max(Math.round(150 * AppPalette.scale), implicitWidth)
                danger: dialog.rebootConfirmPending
                text: dialog.armed ? qsTr("Disarm to reboot")
                                   : dialog.rebootConfirmPending ? qsTr("Tap again to reboot") : qsTr("Reboot")
                enabled: dialog.online && !dialog.armed
                onClicked: dialog.requestReboot()
            }
        }

        Rectangle {
            id: groupsPane
            anchors.top: toolbar.bottom
            anchors.topMargin: Tokens.spaceMd
            anchors.bottom: footer.top
            anchors.bottomMargin: Tokens.spaceMd
            width: Math.round(180 * AppPalette.scale)
            radius: Tokens.radiusLg
            color: AppPalette.bg
            border.color: AppPalette.border
            border.width: 1
            opacity: search.text.length ? 0.5 : 1.0

            ListView {
                id: groupList
                anchors.fill: parent
                anchors.margins: Tokens.spaceXs
                clip: true
                model: dialog.params ? dialog.params.groups : []
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    id: groupRow
                    required property var modelData
                    readonly property bool current: !!dialog.params && dialog.params.currentGroup === modelData.key
                    width: groupList.width
                    height: Tokens.rowH
                    radius: Tokens.radiusMd
                    color: current ? AppPalette.accentBg : groupTap.pressed ? AppPalette.cardHover : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: groupCount.left
                        anchors.rightMargin: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        text: groupRow.modelData.title
                        color: groupRow.current ? AppPalette.textStrong : AppPalette.text
                        font.pixelSize: Tokens.fontBase
                        font.bold: groupRow.current
                        elide: Text.ElideRight
                    }

                    Text {
                        id: groupCount
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        text: groupRow.modelData.count
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                    }

                    TapHandler {
                        id: groupTap
                        onTapped: {
                            search.text = ""
                            dialog.params.currentGroup = groupRow.modelData.key
                        }
                    }
                }
            }

            KScrollBar {
                flickable: groupList
                anchors.top: groupList.top
                anchors.bottom: groupList.bottom
                anchors.right: groupList.right
            }
        }

        Rectangle {
            id: listPane
            anchors.top: toolbar.bottom
            anchors.topMargin: Tokens.spaceMd
            anchors.left: groupsPane.right
            anchors.leftMargin: Tokens.spaceMd
            anchors.right: parent.right
            anchors.bottom: editor.top
            anchors.bottomMargin: Tokens.spaceMd
            radius: Tokens.radiusLg
            color: AppPalette.bg
            border.color: AppPalette.border
            border.width: 1

            ListView {
                id: paramList
                anchors.fill: parent
                anchors.margins: Tokens.spaceXs
                clip: true
                model: dialog.params ? dialog.params.params : null
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    id: paramRow
                    required property string name
                    required property string valueText
                    required property string typeText
                    required property bool pending
                    required property bool failed
                    readonly property bool current: dialog.selectedName === name
                    width: paramList.width
                    height: Tokens.rowH
                    radius: Tokens.radiusMd
                    color: current ? AppPalette.accentBg : paramTap.pressed ? AppPalette.cardHover : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: valueLabel.left
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        text: paramRow.name
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontBase
                        font.family: "monospace"
                        elide: Text.ElideRight
                    }

                    Text {
                        id: valueLabel
                        anchors.right: stateLabel.left
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(140 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: paramRow.valueText
                        color: paramRow.failed ? AppPalette.dangerText : paramRow.pending ? AppPalette.accentText : AppPalette.textStrong
                        font.pixelSize: Tokens.fontBase
                        font.bold: true
                        elide: Text.ElideLeft
                    }

                    Text {
                        id: stateLabel
                        anchors.right: typeLabel.left
                        anchors.rightMargin: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(110 * AppPalette.scale)
                        text: paramRow.pending ? qsTr("writing…") : paramRow.failed ? qsTr("not confirmed") : ""
                        color: paramRow.failed ? AppPalette.dangerText : AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        elide: Text.ElideRight
                    }

                    Text {
                        id: typeLabel
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(52 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: paramRow.typeText
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                    }

                    TapHandler {
                        id: paramTap
                        onTapped: dialog.select(paramRow.name, paramRow.valueText, paramRow.typeText)
                    }
                }
            }

            KScrollBar {
                flickable: paramList
                anchors.top: paramList.top
                anchors.bottom: paramList.bottom
                anchors.right: paramList.right
            }

            Text {
                anchors.centerIn: parent
                visible: paramList.count === 0
                width: parent.width - 2 * Tokens.spaceXl
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: !dialog.params ? ""
                      : dialog.params.loading ? qsTr("Loading parameters…")
                      : search.text.length ? qsTr("Nothing matches the search")
                      : qsTr("No parameters")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontBase
            }
        }

        Rectangle {
            id: editor
            anchors.left: listPane.left
            anchors.right: parent.right
            anchors.bottom: footer.top
            anchors.bottomMargin: Tokens.spaceMd
            height: Tokens.controlHMd + 2 * Tokens.spaceMd + (editErrorText.visible ? editErrorText.implicitHeight + Tokens.spaceXs : 0)
            radius: Tokens.radiusLg
            color: AppPalette.card
            border.color: AppPalette.border
            border.width: 1

            Text {
                id: editName
                anchors.left: parent.left
                anchors.leftMargin: Tokens.spaceMd
                anchors.top: parent.top
                anchors.topMargin: Tokens.spaceMd
                height: Tokens.controlHMd
                width: Math.round(200 * AppPalette.scale)
                verticalAlignment: Text.AlignVCenter
                text: dialog.selectedName.length ? dialog.selectedName : qsTr("Tap a parameter to change it")
                color: dialog.selectedName.length ? AppPalette.textStrong : AppPalette.textMuted
                font.pixelSize: Tokens.fontBase
                font.bold: dialog.selectedName.length > 0
                elide: Text.ElideRight
            }

            Rectangle {
                id: editBox
                anchors.left: editName.right
                anchors.leftMargin: Tokens.spaceMd
                anchors.right: writeButton.left
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: editName.verticalCenter
                height: Tokens.controlHMd
                visible: dialog.selectedName.length > 0
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: 1
                border.color: dialog.editError.length ? AppPalette.dangerBorder
                            : editField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: editField
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    selectByMouse: true
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: if (dialog.selectedName.length && dialog.params) dialog.editError = dialog.params.validate(dialog.selectedName, text)
                    onAccepted: dialog.writeSelected()
                    Keys.onEscapePressed: dialog.clearSelection()
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: Tokens.spaceMd
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("now %1 · %2").arg(dialog.selectedValue).arg(dialog.selectedType)
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm
                }
            }

            KButton {
                id: writeButton
                anchors.right: cancelButton.left
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: editName.verticalCenter
                height: Tokens.controlHMd
                width: Math.max(Math.round(100 * AppPalette.scale), implicitWidth)
                visible: dialog.selectedName.length > 0
                enabled: dialog.online && !dialog.editError.length && editField.text.length > 0
                text: qsTr("Write")
                onClicked: dialog.writeSelected()
            }

            KButton {
                id: cancelButton
                anchors.right: parent.right
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: editName.verticalCenter
                height: Tokens.controlHMd
                width: Math.max(Math.round(100 * AppPalette.scale), implicitWidth)
                visible: dialog.selectedName.length > 0
                text: qsTr("Cancel")
                onClicked: dialog.clearSelection()
            }

            Text {
                id: editErrorText
                anchors.left: editBox.left
                anchors.right: parent.right
                anchors.rightMargin: Tokens.spaceMd
                anchors.top: editBox.bottom
                anchors.topMargin: Tokens.spaceXs
                visible: dialog.selectedName.length > 0 && dialog.editError.length > 0
                text: dialog.editError
                color: AppPalette.dangerText
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }
        }

        Item {
            id: footer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Tokens.controlHMd

            Text {
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.rightMargin: Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                text: dialog.message.length ? dialog.message
                                            : qsTr("Names, values and types come from the autopilot; descriptions and limits are not shown yet.")
                color: dialog.message.length ? AppPalette.text : AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                elide: Text.ElideRight
            }

            KButton {
                id: closeButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(124 * AppPalette.scale)
                height: Tokens.controlHMd
                fontPixelSize: Tokens.fontMd
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save autopilot parameters")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Parameter files (*.params)"), qsTr("All files (*)")]
        defaultSuffix: "params"
        onAccepted: {
            if (!dialog.params) return
            dialog.message = dialog.params.saveToFile(selectedFile)
                ? qsTr("Saved %1 parameters").arg(dialog.params.count)
                : dialog.params.lastError
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Load autopilot parameters")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Parameter files (*.params *.param *.parm *.txt)"), qsTr("All files (*)")]
        onAccepted: {
            if (!dialog.params) return
            if (dialog.params.compareFile(selectedFile))
                diffPopup.open()
            else
                dialog.message = dialog.params.lastError
        }
    }

    Popup {
        id: diffPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: Math.round(14 * AppPalette.scale)
        closePolicy: Popup.CloseOnEscape
        width: Math.min(Math.round(760 * AppPalette.scale), parent ? parent.width * 0.9 : 760)
        height: Math.min(Math.round(560 * AppPalette.scale), parent ? parent.height * 0.9 : 560)
        onClosed: if (dialog.params) dialog.params.clearDiff()

        readonly property var rows: dialog.params ? dialog.params.diffRows : []

        background: Rectangle {
            color: AppPalette.bgDeep
            border.color: AppPalette.border
            border.width: 1
            radius: 10
        }

        Item {
            anchors.fill: parent

            Text {
                id: diffTitle
                width: parent.width
                text: qsTr("Parameters from the file")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontXl
                font.bold: true
            }

            Text {
                id: diffSummary
                anchors.top: diffTitle.bottom
                anchors.topMargin: Tokens.spaceXs
                width: parent.width
                wrapMode: Text.WordWrap
                text: {
                    if (!dialog.params) return ""
                    var parts = [qsTr("differ: %1").arg(diffPopup.rows.length),
                                 qsTr("same: %1").arg(dialog.params.diffUnchanged)]
                    if (dialog.params.diffMissing.length)
                        parts.push(qsTr("not on the vehicle: %1").arg(dialog.params.diffMissing.length))
                    if (dialog.params.diffInvalid.length)
                        parts.push(qsTr("invalid value: %1").arg(dialog.params.diffInvalid.join(", ")))
                    return parts.join(" · ")
                }
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
            }

            Rectangle {
                id: diffPane
                anchors.top: diffSummary.bottom
                anchors.topMargin: Tokens.spaceMd
                anchors.bottom: diffButtons.top
                anchors.bottomMargin: Tokens.spaceMd
                width: parent.width
                radius: Tokens.radiusLg
                color: AppPalette.bg
                border.color: AppPalette.border
                border.width: 1

                ListView {
                    id: diffList
                    anchors.fill: parent
                    anchors.margins: Tokens.spaceXs
                    clip: true
                    model: diffPopup.rows
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Item {
                        id: diffRow
                        required property var modelData
                        required property int index
                        width: diffList.width
                        height: Tokens.rowH

                        KCheckBox {
                            id: diffCheck
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: ""
                            checked: diffRow.modelData.selected
                            onClicked: dialog.params.setDiffSelected(diffRow.index, checked)
                        }

                        Text {
                            anchors.left: diffCheck.right
                            anchors.leftMargin: Tokens.spaceSm
                            anchors.right: diffValues.left
                            anchors.rightMargin: Tokens.spaceMd
                            anchors.verticalCenter: parent.verticalCenter
                            text: diffRow.modelData.name
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontBase
                            font.family: "monospace"
                            elide: Text.ElideRight
                        }

                        Text {
                            id: diffValues
                            anchors.right: parent.right
                            anchors.rightMargin: Tokens.spaceMd
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("%1 → %2").arg(diffRow.modelData.current).arg(diffRow.modelData.file)
                            color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontBase
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: diffList.count === 0
                    text: qsTr("The vehicle already has every value from the file")
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontBase
                }

                KScrollBar {
                    flickable: diffList
                    anchors.top: diffList.top
                    anchors.bottom: diffList.bottom
                    anchors.right: diffList.right
                }
            }

            Row {
                id: diffButtons
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                spacing: Tokens.spaceSm

                KButton {
                    height: Tokens.controlHMd
                    width: Math.max(Math.round(110 * AppPalette.scale), implicitWidth)
                    visible: diffPopup.rows.length > 1
                    text: dialog.params && dialog.params.diffSelected === diffPopup.rows.length ? qsTr("Select none") : qsTr("Select all")
                    onClicked: dialog.params.setAllDiffSelected(dialog.params.diffSelected !== diffPopup.rows.length)
                }

                KButton {
                    height: Tokens.controlHMd
                    width: Math.max(Math.round(110 * AppPalette.scale), implicitWidth)
                    text: qsTr("Cancel")
                    onClicked: diffPopup.close()
                }

                KButton {
                    height: Tokens.controlHMd
                    width: Math.max(Math.round(160 * AppPalette.scale), implicitWidth)
                    enabled: dialog.online && !!dialog.params && dialog.params.diffSelected > 0
                    text: qsTr("Write selected (%1)").arg(dialog.params ? dialog.params.diffSelected : 0)
                    onClicked: {
                        var queued = dialog.params.applyDiff()
                        dialog.message = qsTr("Writing %1 parameters").arg(queued)
                        diffPopup.close()
                    }
                }
            }
        }
    }
}

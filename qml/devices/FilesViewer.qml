import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import app 1.0

Column {
    id: filesViewer

    property var store: null

    property string filePath: currentLogPath()
    property var lastLogFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var recentOpenedFiles: []
    property string selectedLogPathSource: ""

    spacing: Tokens.spaceSm

    Settings {
        category: "main/files"
        property alias logFolder:         filesViewer.lastLogFolder
        property alias recentOpenedFiles: filesViewer.recentOpenedFiles
        property alias pathText:          filesViewer.selectedLogPathSource
    }

    Connections {
        target: core
        function onFileOpenFailed(path) { filesViewer.removeRecentFile(path) }
        function onFilePathChanged() { filesViewer.setLogPath(core.filePath) }
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

    function setLogPath(path) {
        selectedLogPathSource = urlSource(path)
        pathText.text = urlDisplay(selectedLogPathSource)
    }

    function currentLogPath() {
        return effectiveSource(pathText.text, selectedLogPathSource)
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
        newFileDialog.currentFolder = filesViewer.lastLogFolder
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

    Component.onCompleted: setLogPath(selectedLogPathSource.length ? selectedLogPathSource : core.filePath)

    component IconBtn: Rectangle {
        id: ib
        property bool checked: false
        property bool checkable: false
        property string iconSource: ""
        property string toolTipText: ""
        signal clicked()
        signal toggled(bool val)

        width: Math.round(28 * AppPalette.scale); height: Math.round(28 * AppPalette.scale); radius: Tokens.radiusSm + 1
        color: checked ? AppPalette.accentBg : (ibMa.pressed ? AppPalette.bgDeep : (ibMa.containsMouse ? AppPalette.cardHover : AppPalette.card))
        border.width: 1
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

    // ── Open file ───────────────────────────────────────────────────────────

    Text {
        text: qsTr("Open file:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

        Rectangle {
            // path + 3 IconBtn-а (open/append/close) = 4 элемента, 3 spacing-а.
            width: parent.width - 3 * Tokens.controlHMd - 3 * Tokens.spaceSm
            height: Tokens.controlHMd; radius: Tokens.radiusMd; color: AppPalette.bg; border.width: 1
            border.color: pathText.activeFocus ? AppPalette.accentBorder : AppPalette.border

            TextInput {
                id: pathText
                activeFocusOnTab: true
                anchors.fill: parent; anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: pathText.selectAll() }
                verticalAlignment: TextInput.AlignVCenter
                color: AppPalette.text; font.pixelSize: Tokens.fontSm; clip: true
                Text {
                    visible: !pathText.text.length; text: qsTr("File path...")
                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter
                }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        const lp = filesViewer.currentLogPath()
                        filesViewer.setLogPath(lp)
                        filesViewer.pushRecentOpenedFile(lp)
                        core.openLogFile(lp, false, false)
                    }
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file.svg"; toolTipText: qsTr("Open file")
            width: Tokens.controlHMd; height: Tokens.controlHMd; anchors.verticalCenter: parent.verticalCenter
            onClicked: filesViewer.openNewFileDialog()

            FileDialog {
                id: newFileDialog
                title: qsTr("Please choose a file")
                currentFolder: filesViewer.lastLogFolder
                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]
                onCurrentFolderChanged: filesViewer.lastLogFolder = currentFolder
                onAccepted: {
                    const file = newFileDialog.selectedFile
                    if (!file) return
                    filesViewer.lastLogFolder = newFileDialog.currentFolder
                    const lp = urlSource(file)
                    filesViewer.setLogPath(lp)
                    filesViewer.pushRecentOpenedFile(lp)
                    core.openLogFile(lp, false, false)
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_plus.svg"; toolTipText: qsTr("Append file")
            width: Tokens.controlHMd; height: Tokens.controlHMd; anchors.verticalCenter: parent.verticalCenter
            onClicked: { appendFileDialog.currentFolder = filesViewer.lastLogFolder; appendFileDialog.open() }

            FileDialog {
                id: appendFileDialog
                title: qsTr("Please choose a file")
                currentFolder: filesViewer.lastLogFolder
                nameFilters: ["Logs (*.klf *.KLF *.ubx *.UBX *.xtf *.XTF)", "Kogger log files (*.klf *.KLF)", "U-blox (*.ubx *.UBX)"]
                onCurrentFolderChanged: filesViewer.lastLogFolder = currentFolder
                onAccepted: {
                    const lp = urlSource(appendFileDialog.selectedFile)
                    filesViewer.setLogPath(lp)
                    filesViewer.lastLogFolder = appendFileDialog.currentFolder
                    filesViewer.pushRecentOpenedFile(lp)
                    core.openLogFile(lp, true, false)
                }
            }
        }

        IconBtn {
            iconSource: "qrc:/icons/ui/file_off.svg"; toolTipText: qsTr("Close file")
            width: Tokens.controlHMd; height: Tokens.controlHMd; anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                if (core.openedFilePath.length > 0) {
                    core.closeLogFile();
                } else {
                    core.onRequestClearing();
                }
            }
        }
    }

    // ── Recent files ────────────────────────────────────────────────────────

    Column {
        visible: recentOpenedFiles.length > 0
        width: parent.width
        spacing: Tokens.spaceXxs + 1

        Text {
            text: qsTr("Recently opened:")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
            leftPadding: Tokens.spaceXxs
        }

        Repeater {
            model: Math.min(recentOpenedFiles.length, 3)

            Row {
                width: parent.width
                spacing: Tokens.spaceXs

                property string filePath: recentOpenedFiles[index] || ""

                Rectangle {
                    id: recentCard
                    width: parent.width - removeBtn.width - parent.spacing
                    height: Tokens.controlHMd - Tokens.spaceXxs; radius: Tokens.radiusMd
                    color: recentMa.containsMouse ? AppPalette.cardHover : AppPalette.card
                    border.width: 1; border.color: AppPalette.border
                    Behavior on color { ColorAnimation { duration: 80 } }

                    activeFocusOnTab: true
                    Keys.onReturnPressed: filesViewer.openRecentFile(parent.filePath)
                    Keys.onEnterPressed:  filesViewer.openRecentFile(parent.filePath)
                    Keys.onSpacePressed:  filesViewer.openRecentFile(parent.filePath)

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                        text: filesViewer.urlDisplay(parent.parent.filePath)
                        color: AppPalette.text; font.pixelSize: Tokens.fontXs
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideLeft
                    }

                    KFocusRing { id: focusRing }

                    MouseArea {
                        id: recentMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: focusRing.suppress()
                        onClicked: { recentCard.forceActiveFocus(); filesViewer.openRecentFile(parent.parent.filePath) }
                    }
                }

                IconBtn {
                    id: removeBtn
                    iconSource: "qrc:/icons/ui/x.svg"
                    width: Tokens.controlHMd - Tokens.spaceXxs
                    height: Tokens.controlHMd - Tokens.spaceXxs
                    toolTipText: qsTr("Remove")
                    onClicked: filesViewer.removeRecentFile(parent.filePath)
                }
            }
        }
    }
}

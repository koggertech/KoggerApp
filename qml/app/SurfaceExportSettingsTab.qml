import QtQuick 2.15
import QtCore
import QtQuick.Dialogs
import kqml_types 1.0
import "ExportPaths.js" as ExportPaths

Column {
    id: page

    required property var store

    readonly property color _bright: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
    readonly property real slotW: Math.round(150 * AppPalette.scale)
    readonly property bool surfaceOn: !!(page.store && page.store.isobathsVisible)
    readonly property bool running: IsobathsViewControlMenuController.surfaceExportRunning
    readonly property bool xyz: formatCombo.currentIndex === 1
    readonly property int bytesPerPoint: xyz ? 33 : 55
    readonly property real largeFileBytes: 500 * 1000 * 1000

    readonly property var estimate: IsobathsViewControlMenuController.surfaceExportEstimate
    readonly property real nodeStep: IsobathsViewControlMenuController.surfaceExportNodeStep(stepCombo.currentIndex)
    readonly property real area: estimate && estimate.ready
                                 ? estimate.triangulationArea + (extrapolationSwitch.checked ? estimate.extrapolationArea : 0)
                                 : -1
    readonly property real points: area >= 0 && nodeStep > 0 ? area / (nodeStep * nodeStep) : -1
    readonly property real bytes: points * bytesPerPoint
    readonly property bool largeFile: bytes > largeFileBytes

    readonly property string folder: page.store && page.store.exportFolderSource.length
                                     ? ExportPaths.localPath(page.store.exportFolderSource)
                                     : core.defaultExportDirectory()
    readonly property string sourceKey: core.openedFilePath + "|" + core.fileTitle
    readonly property string nameSuffix: "_surface_" + page.nodeStep.toFixed(2) + "m"
    readonly property string fileExtension: page.xyz ? "xyz" : "csv"
    readonly property real fieldLabelW: Math.round(72 * AppPalette.scale)
    property bool nameEdited: false

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceSm

    component Spacer: Item {
        property int size: Tokens.spaceMd
        width: page.width
        height: size
    }

    function formatBytes(b) {
        if (b >= 1e9) return qsTr("%1 GB").arg((b / 1e9).toFixed(1))
        if (b < 1e6) return qsTr("< 1 MB")
        return qsTr("%1 MB").arg(Math.round(b / 1e6))
    }
    function estimateText() {
        if (page.points < 0)
            return qsTr("Estimating size…")
        var n = Math.round(page.points).toLocaleString(Qt.locale(), 'f', 0)
        return page.largeFile ? qsTr("≈ %1 points, ~%2 — a large file").arg(n).arg(page.formatBytes(page.bytes))
                              : qsTr("≈ %1 points, ~%2").arg(n).arg(page.formatBytes(page.bytes))
    }
    function autoFileName() {
        var source = core.exportSourceName()
        return source.length ? source + page.nameSuffix : ""
    }
    function refillName() {
        if (!page.nameEdited)
            fileNameText.text = page.autoFileName()
    }
    function startExport() {
        var name = ExportPaths.exportFileName(fileNameText.text, core.exportFileStem(), page.nameSuffix, page.fileExtension)
        if (ExportPaths.hasInvalidFileNameChars(name)) {
            if (typeof notifications !== "undefined" && notifications)
                notifications.warning(qsTr("Invalid characters in file name"))
            return
        }
        var path = core.exportFilePath(page.store ? page.store.exportFolderSource : "", name)
        IsobathsViewControlMenuController.startSurfaceExport(path, stepCombo.currentIndex, extrapolationSwitch.checked,
                                                             depthOffsetSpinBox.value / depthOffsetSpinBox.divisor,
                                                             formatCombo.currentIndex)
    }

    onRunningChanged: if (!running) refillName()
    onNameSuffixChanged: refillName()
    onSourceKeyChanged: {
        nameEdited = false
        refillName()
    }
    Component.onCompleted: refillName()
    Component.onDestruction: IsobathsViewControlMenuController.surfaceExportEstimateActive = false

    Binding {
        target: IsobathsViewControlMenuController
        property: "surfaceExportEstimateActive"
        value: page.visible && page.surfaceOn
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Saves the surface with the chosen parameters.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Spacer {}

    KIsland {
        visible: !page.surfaceOn

        KIslandRow {
            label: qsTr("Show surface")
            caption: qsTr("Without the surface there is nothing to export.")
            labelColor: page._bright
            interactive: true
            onClicked: surfaceSwitch.click()

            KSwitch {
                id: surfaceSwitch
                flat: true
                checked: page.surfaceOn
                onToggled: {
                    if (page.store) page.store.isobathsVisible = checked
                    checked = Qt.binding(function() { return page.surfaceOn })
                }
            }
        }
    }

    Spacer { visible: !page.surfaceOn }

    KIsland {
        title: qsTr("Grid")

        KIslandRow {
            label: qsTr("Point spacing")
            caption: page.surfaceOn ? page.estimateText() : ""
            captionColor: page.largeFile ? AppPalette.dangerText : AppPalette.textSecond
            labelColor: page._bright
            slotWidth: page.slotW

            KCombo {
                id: stepCombo
                width: page.slotW
                model: {
                    var items = []
                    for (var q = 0; q <= 6; ++q)
                        items.push(qsTr("%1 m").arg(IsobathsViewControlMenuController.surfaceExportNodeStep(q).toFixed(2)))
                    return items
                }
                currentIndex: 2
            }
        }

        KIslandRow {
            label: qsTr("Extrapolation")
            toolTipText: qsTr("Also the surface filled around the track")
            labelColor: page._bright
            interactive: true
            onClicked: extrapolationSwitch.click()

            KSwitch {
                id: extrapolationSwitch
                flat: true
                checked: true
            }
        }

        KIslandRow {
            label: qsTr("Depth correction, m")
            toolTipText: qsTr("Added to every depth; positive is deeper")
            labelColor: page._bright
            slotWidth: page.slotW

            KSpinBox {
                id: depthOffsetSpinBox
                width: page.slotW
                height: Tokens.controlHMd
                from: -10000; to: 10000; stepSize: 10; value: 0
                divisor: 100; decimals: 2
            }
        }
    }

    Spacer {}

    KIsland {
        title: qsTr("File")

        KIslandRow {
            label: qsTr("Format")
            labelColor: page._bright
            slotWidth: page.slotW

            KCombo {
                id: formatCombo
                width: page.slotW
                model: ["CSV", "XYZ"]
                currentIndex: 0
            }
        }

        KIslandRow {
            id: folderRow
            label: qsTr("Path")
            labelColor: page._bright
            slotWidth: folderRow.innerWidth - page.fieldLabelW - Tokens.spaceMd

            Row {
                width: parent.width
                height: Tokens.controlHMd
                spacing: Tokens.spaceSm

                Rectangle {
                    width: parent.width - Tokens.controlHMd - Tokens.spaceSm
                    height: Tokens.controlHMd
                    radius: Tokens.radiusMd
                    color: AppPalette.bg
                    border.width: folderText.activeFocus ? 1 : Tokens.cardBorderWidth
                    border.color: folderText.activeFocus ? AppPalette.accentBorder : AppPalette.border

                    TextInput {
                        id: folderText
                        activeFocusOnTab: true
                        anchors.fill: parent
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.rightMargin: Tokens.spaceMd
                        TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: folderText.selectAll() }
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontBase
                        clip: true
                        autoScroll: activeFocus
                        text: page.folder
                        onTextEdited: if (page.store) page.store.exportFolderSource = ExportPaths.localPath(text)

                        Connections {
                            target: page
                            function onFolderChanged() {
                                if (folderText.text !== page.folder)
                                    folderText.text = page.folder
                            }
                        }

                        Text {
                            visible: !folderText.text.length
                            text: qsTr("Export folder")
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontBase
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                KButton {
                    width: Tokens.controlHMd
                    height: Tokens.controlHMd
                    text: "..."
                    horizontalPadding: 0
                    verticalPadding: 0
                    normalBg: AppPalette.bg
                    normalBorder: AppPalette.border
                    toolTipText: qsTr("Choose export folder")
                    onClicked: {
                        if (page.store)
                            folderDialog.currentFolder = page.store.exportFolderUrl
                        folderDialog.open()
                    }
                }
            }
        }

        KIslandRow {
            id: nameRow
            label: qsTr("Name")
            labelColor: page._bright
            slotWidth: nameRow.innerWidth - page.fieldLabelW - Tokens.spaceMd

            Rectangle {
                width: parent.width
                height: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: fileNameText.activeFocus ? 1 : Tokens.cardBorderWidth
                border.color: fileNameText.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: fileNameText
                    activeFocusOnTab: true
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: fileNameText.selectAll() }
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    clip: true
                    autoScroll: activeFocus
                    onTextEdited: page.nameEdited = true
                    onEditingFinished: {
                        if (text.trim().length)
                            return
                        page.nameEdited = false
                        page.refillName()
                    }

                    Text {
                        visible: !fileNameText.text.length
                        text: qsTr("File name")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontBase
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Export folder")
        onAccepted: {
            if (!page.store) return
            page.store.exportFolderUrl = folderDialog.currentFolder
            page.store.exportFolderSource = ExportPaths.localPath(folderDialog.selectedFolder)
        }
    }

    Spacer { size: Tokens.spaceLg }

    KButton {
        width: parent.width
        height: Tokens.controlHLg
        fontPixelSize: Tokens.fontLg
        text: page.running ? qsTr("Cancel export") : page.xyz ? qsTr("Export to XYZ") : qsTr("Export to CSV")
        enabled: page.running || page.surfaceOn
        onClicked: page.running ? IsobathsViewControlMenuController.cancelSurfaceExport() : page.startExport()
    }

    Settings { category: "scene3d/isobaths"; property alias exportSurfaceQuality:       stepCombo.currentIndex }
    Settings { category: "scene3d/isobaths"; property alias exportSurfaceExtrapolation: extrapolationSwitch.checked }
    Settings { category: "scene3d/isobaths"; property alias exportSurfaceDepthOffset:   depthOffsetSpinBox.value }
    Settings { category: "scene3d/isobaths"; property alias exportSurfaceFormat:        formatCombo.currentIndex }
}

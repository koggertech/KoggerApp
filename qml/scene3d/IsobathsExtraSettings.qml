import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtQuick.Window 2.15
import QtCore
import kqml_types 1.0
import app 1.0
import "../controls"
import "../menus"


MenuFrame {
    id: isobathsSettings

    property CheckButton isobathsCheckButton
    property var exportSurfaceFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property string exportSurfacePathSource: ""

    readonly property Item _overlayItem: Window.window ? Window.window.contentItem : null
    parent: _overlayItem
    z: 1000

    function _updatePosition() {
        if (!parent || !isobathsCheckButton) return
        var p = isobathsCheckButton.mapToItem(parent, 0, 0)
        var nx = p.x + isobathsCheckButton.width / 2 - width / 2
        var ny = p.y - height
        if (nx < 0) nx = 0
        if (ny < 0) ny = 0
        if (parent.width  > 0 && nx + width  > parent.width)  nx = parent.width  - width
        if (parent.height > 0 && ny + height > parent.height) ny = parent.height - height
        x = nx
        y = ny
    }
    onWidthChanged:  _updatePosition()
    onHeightChanged: _updatePosition()
    Connections {
        target: isobathsSettings.isobathsCheckButton
        ignoreUnknownSignals: true
        function onXChanged()      { isobathsSettings._updatePosition() }
        function onYChanged()      { isobathsSettings._updatePosition() }
        function onWidthChanged()  { isobathsSettings._updatePosition() }
        function onHeightChanged() { isobathsSettings._updatePosition() }
    }

    readonly property bool anyComboPopupOpen:
        isobathsTheme.popup && isobathsTheme.popup.visible

    readonly property bool anyHoverSource:
        isobathsCheckButton.hovered ||
        isHovered                   ||
        anyComboPopupOpen           ||
        isobathsTheme.activeFocus

    visible: Qt.platform.os === "android"
             ? (isobathsCheckButton.isobathsLongPressTriggered || anyHoverSource)
             : anyHoverSource

    backgroundColor: AppPalette.bg
    horizontalMargins: Tokens.spaceLg
    verticalMargins: Tokens.spaceLg
    spacing: Tokens.spaceMd

    onIsHoveredChanged: {
        if (Qt.platform.os === "android") {
            if (isHovered) {
                isHovered = false
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            focus = true;
            _updatePosition()
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            Qt.callLater(function() {
                if (!isobathsSettings.focus) {
                    isobathsCheckButton.isobathsLongPressTriggered = false
                }
            })
        }
    }

    function sourceUrl(value) {
        if (!value) {
            return ""
        }

        if (typeof value === "string") {
            if (value.startsWith("file:///")) {
                return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
            }
            if (value.startsWith("file://")) {
                return value.slice(7)
            }
            return value
        }

        var localPath = value.toLocalFile ? value.toLocalFile() : ""
        return localPath && localPath.length ? localPath : value.toString()
    }

    function displayUrl(value) {
        var source = sourceUrl(value)
        if (!source.length) {
            return ""
        }

        try {
            return decodeURIComponent(source)
        } catch (error) {
            return source
        }
    }

    function prevTheme() {
        isobathsTheme.currentIndex = Math.max(0, isobathsTheme.currentIndex - 1)
    }

    function nextTheme() {
        isobathsTheme.currentIndex = Math.min(isobathsTheme.model.length - 1, isobathsTheme.currentIndex + 1)
    }

    function stepDown(step) {
        const delta = step === undefined ? 1 : step
        for (let i = 0; i < delta; ++i) {
            isobathsSurfaceLineStepSizeSpinBox.decrement()
        }
    }

    function stepUp(step) {
        const delta = step === undefined ? 1 : step
        for (let i = 0; i < delta; ++i) {
            isobathsSurfaceLineStepSizeSpinBox.increment()
        }
    }

    function effectiveSource(displayText, storedSource) {
        if (!displayText || !displayText.length) {
            return ""
        }

        if (storedSource && displayText === displayUrl(storedSource)) {
            return storedSource
        }

        return displayText
    }

    Component.onCompleted: {
        exportSurfacePathText.text = displayUrl(exportSurfacePathSource)
    }

    function currentExportSurfacePath() {
        return effectiveSource(exportSurfacePathText.text, exportSurfacePathSource)
    }

    readonly property int labelW: Math.round(120 * AppPalette.scale)
    readonly property int ctrlW:  Math.round(200 * AppPalette.scale)

    ColumnLayout {
        spacing: Tokens.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
            Text {
                text: qsTr("Isobaths settings")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
        }

        RowLayout {
            spacing: Tokens.spaceMd
            Layout.fillWidth: true

            Text {
                text: qsTr("Theme:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.preferredWidth: isobathsSettings.labelW
            }
            Item { Layout.fillWidth: true }
            KCombo  {
                id: isobathsTheme
                Layout.preferredWidth: isobathsSettings.ctrlW
                model: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("Standard"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green")]
                currentIndex: 0
                onCurrentIndexChanged: {
                    IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                }

                onActiveFocusChanged: {
                    if (Qt.platform.os === 'android') {
                        isobathsSettings.focus = true
                    }
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                }

                Settings {
                    property alias isobathsTheme: isobathsTheme.currentIndex
                }
            }
        }

        RowLayout {
            spacing: Tokens.spaceMd
            Layout.fillWidth: true

            Text {
                text: qsTr("Edge limit, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsEdgeLimitSpinBox
                Layout.preferredWidth: isobathsSettings.ctrlW
                from: 10
                to: 1000
                stepSize: 5
                value: 100
                editable: false

                onValueModified: function(v) {
                    IsobathsViewControlMenuController.onEdgeLimitChanged(v)
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onEdgeLimitChanged(value)
                }

                Settings {
                    property alias isobathsEdgeLimitSpinBox: isobathsEdgeLimitSpinBox.value
                }
            }
        }

        RowLayout {
            spacing: Tokens.spaceMd
            Layout.fillWidth: true

            Text {
                text: qsTr("Step, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsSurfaceLineStepSizeSpinBox
                Layout.preferredWidth: isobathsSettings.ctrlW
                from: 1
                to: 200
                stepSize: 1
                value: 10
                divisor: 10
                decimals: 1
                editable: false

                readonly property real realValue: value / 10

                onValueModified: function(v) {
                    IsobathsViewControlMenuController.onSetSurfaceLineStepSize(v / 10)
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onSetSurfaceLineStepSize(realValue)
                }

                Settings {
                    property alias isobathsSurfaceLineStepSizeSpinBox: isobathsSurfaceLineStepSizeSpinBox.value
                }
            }
        }

        RowLayout {
            spacing: Tokens.spaceMd
            Layout.fillWidth: true

            Text {
                text: qsTr("Extra width, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
            }
            Item { Layout.fillWidth: true }
            KSpinBox {
                id: extraWidthSpinBox
                Layout.preferredWidth: isobathsSettings.ctrlW
                from: 5
                to: 100
                stepSize: 5
                value: 10
                editable: false

                onValueModified: function(v) {
                    IsobathsViewControlMenuController.onSetExtraWidth(v)
                }
                Component.onCompleted: {
                    IsobathsViewControlMenuController.onSetExtraWidth(value)
                }

                Settings {
                    property alias extraWidthSpinBox: extraWidthSpinBox.value
                }
            }
        }

        RowLayout {
            spacing: Tokens.spaceMd
            Layout.fillWidth: true

            onFocusChanged: {
                isobathsSettings.focus = true
            }

            CTextField {
                id: exportSurfacePathText
                hoverEnabled: true
                Layout.maximumWidth: isobathsSettings.ctrlW
                Layout.fillWidth: true
                placeholderText: qsTr("Enter path")

                onFocusChanged: {
                    isobathsSettings.focus = true
                }
            }

            KButton {
                text: "..."
                Layout.fillWidth: false
                implicitWidth: Math.round(40 * AppPalette.scale)

                onClicked: {
                    exportSurfaceFileDialog.currentFolder = isobathsSettings.exportSurfaceFolder
                    exportSurfaceFileDialog.open()
                }

                onFocusChanged: {
                    isobathsSettings.focus = true
                }
            }

            FileDialog  {
                id: exportSurfaceFileDialog
                title: qsTr("Select folder and set .csv file name")

                currentFolder: isobathsSettings.exportSurfaceFolder

                fileMode: FileDialog.SaveFile

                nameFilters: ["CSV Files (*.csv)", "All Files (*)"]
                defaultSuffix: "csv"

                onCurrentFolderChanged: {
                    isobathsSettings.exportSurfaceFolder = currentFolder
                }

                onAccepted: {
                    isobathsSettings.exportSurfaceFolder = exportSurfaceFileDialog.currentFolder
                    isobathsSettings.exportSurfacePathSource = sourceUrl(selectedFile)
                    if (!isobathsSettings.exportSurfacePathSource.toLowerCase().endsWith(".csv")) {
                        isobathsSettings.exportSurfacePathSource += ".csv"
                    }
                    exportSurfacePathText.text = displayUrl(isobathsSettings.exportSurfacePathSource)
                }
            }

            KButton {
                text: qsTr("Export to CSV")
                Layout.fillWidth: true
                onClicked: Scene3DControlMenuController.onExportToCSVButtonClicked(isobathsSettings.currentExportSurfacePath())

                onFocusChanged: {
                    isobathsSettings.focus = true
                }
            }


            Settings {
                property alias exportSurfaceFolder: isobathsSettings.exportSurfaceFolder
            }

            Settings {
                property alias exportSurfaceFolderText: isobathsSettings.exportSurfacePathSource
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import Qt5Compat.GraphicalEffects
import QtCore
import kqml_types 1.0
import controls
import scene2d

Column {
    id: root

    required property var store
    property var targetPlot: null
    property var echograms: []

    readonly property int instruments: theme ? theme.instrumentsGrade : 0
    readonly property real groupWidth: Math.max(0, width)
    readonly property color _bright: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
    readonly property color _shownFill:    Qt.rgba(0.98, 0.80, 0.08, AppPalette.isDark ? 0.14 : 0.20)
    readonly property color _shownHover:   Qt.rgba(0.98, 0.80, 0.08, AppPalette.isDark ? 0.24 : 0.30)
    readonly property color _shownPressed: Qt.rgba(0.98, 0.80, 0.08, AppPalette.isDark ? 0.32 : 0.38)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    property var _flick: null
    Component.onCompleted: _flick = _settingsFlickable()
    function _settingsFlickable() {
        var it = root.parent
        while (it) {
            if (it.contentY !== undefined && it.contentHeight !== undefined && it.flickableDirection !== undefined)
                return it
            it = it.parent
        }
        return null
    }
    function _expandedGroupInFlick() {
        if (!store || !store._settingsGroupInstances)
            return null
        var f = _flick || _settingsFlickable()
        var arr = store._settingsGroupInstances
        for (var i = 0; i < arr.length; ++i) {
            var g = arr[i]
            if (g && g.expanded && g.collapsible && g._flick === f)
                return g
        }
        return null
    }
    function _scrollToTop() {
        var f = _flick || _settingsFlickable()
        if (!f)
            return
        var g = _expandedGroupInFlick()
        var target = g ? g.mapToItem(f.contentItem, 0, 0).y : 0
        target = Math.max(0, Math.min(target, Math.max(0, f.contentHeight - f.height)))
        if (Math.abs(target - f.contentY) < 0.5)
            return
        scrollTopAnim.target = f
        scrollTopAnim.from = f.contentY
        scrollTopAnim.to = target
        scrollTopAnim.restart()
    }
    NumberAnimation {
        id: scrollTopAnim
        property: "contentY"
        duration: 220
        easing.type: Easing.OutCubic
    }

    component ShowIn3DAction: KCircleIconButton {
        property bool active: false
        readonly property color _fg: active ? AppPalette.accentText : AppPalette.text
        readonly property int _sz: Math.round(36 * AppPalette.scale)   // = SettingsGroup._headerH
        width: _sz
        height: _sz
        cornerRadius: Tokens.radiusLg   // uniform rounded chip, full header height
        borderWidth: 0
        scaleOnHover: false
        iconSource: active ? "qrc:/icons/ui/eye.svg" : "qrc:/icons/ui/eye-off.svg"
        iconPixelSize: Math.round(width * 0.70)
        iconTintColor: _fg
        toolTipText: qsTr("Show in 3D")
        fillColor:      active ? AppPalette.accentBgStrong : AppPalette.chipRaised
        fillHoverColor: active ? AppPalette.accentBgStrong : AppPalette.chipRaisedHover

        Text {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: Math.round(1 * AppPalette.scale)
            text: "3D"
            color: parent._fg
            opacity: 0.9
            font.pixelSize: Math.round(parent.width * 0.26)
            font.bold: true
            style: Text.Outline
            styleColor: parent.fillColor
        }
    }

    // Parameter row card — matches KSwitch's full-width pattern.
    //
    //   [ label                  (interactive area)              [TOGGLE] ]
    //                                  [ optional spinbox slot ]
    //
    // Click anywhere on the card (except the spinbox area) flips the toggle.
    // Hover highlights the whole card. The default property is a content slot
    // sized by `slotWidth` — drop a KSpinBox (or any control) inside it.

    // Inline toggle switch for parameter rows — same visual size as KSwitch's
    // indicator so all toggles in the app look identical and are easy to tap.
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
            color: sc.checked ? AppPalette.toggleOn : AppPalette.trackOff
            border.width: 1
            border.color: sc.checked ? AppPalette.toggleOnBorder : AppPalette.trackOffBorder

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

                Behavior on x {
                    NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                }
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


    // URL helpers (for export/import paths)
    function localPath(value) {
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

    function displayPath(value) {
        var s = localPath(value)
        if (!s.length) return ""
        try { return decodeURIComponent(s) } catch(e) { return s }
    }

    onTargetPlotChanged: {
        if (root.targetPlot) btGroup.refreshParams()
    }

    // ── Connections ──────────────────────────────────────────────────────────

    ConnectionsSettingsPage {
        width: root.groupWidth
        store: root.store
    }

    // ── Files ──────────────────────────────────────────────────────────────

    FilesSettingsPage {
        width: root.groupWidth
        store: root.store
    }

    // ── Factory (visible only in factory mode) ──────────

    FactorySettingsGroup {
        width: root.groupWidth
        store: root.store
        visible: (typeof core !== "undefined" && core) ? core.isFactoryMode : false
    }

    // ── Экспорт ───────────────────────────────────────────────────────────────

    SettingsGroup {
        id: exportGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Export")
        description: qsTr("Export plot data as XTF, CSV (regular or complex) or USBL.")
        stateStore: root.store
        stateKey: "app.export"
        collapsedByDefault: true
        contentSpacing: Tokens.spaceSm

        function currentExportPath() {
            return root.store ? root.store.exportFolderSource : ""
        }

        // Path row
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

            Rectangle {
                width: parent.width - Tokens.controlHMd - Tokens.spaceSm
                height: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: exportPathField.activeFocus ? 1 : Tokens.cardBorderWidth
                border.color: exportPathField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: exportPathField
                    activeFocusOnTab: true
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: exportPathField.selectAll() }
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    clip: true
                    text: root.store ? (root.store.exportFolderSource.length ? root.localPath(root.store.exportFolderSource) : core.defaultExportDirectory()) : ""
                    onTextEdited: if (root.store) root.store.exportFolderSource = root.localPath(text)

                    Connections {
                        target: root.store
                        ignoreUnknownSignals: true
                        function onExportFolderSourceChanged() {
                            var clean = root.store.exportFolderSource.length ? root.localPath(root.store.exportFolderSource) : core.defaultExportDirectory()
                            if (exportPathField.text !== clean)
                                exportPathField.text = clean
                        }
                    }

                    Text {
                        visible: !exportPathField.text.length
                        text: qsTr("Export path...")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontBase
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            KButton {
                width: Tokens.controlHMd; height: Tokens.controlHMd; text: "..."
                horizontalPadding: 0; verticalPadding: 0
                toolTipText: qsTr("Choose export folder")
                onClicked: {
                    if (root.store)
                        exportFolderDialog.currentFolder = root.store.exportFolderUrl
                    exportFolderDialog.open()
                }
            }

            FolderDialog {
                id: exportFolderDialog
                title: qsTr("Export folder")
                onAccepted: {
                    if (!root.store) return
                    root.store.exportFolderUrl = exportFolderDialog.currentFolder
                    root.store.exportFolderSource = root.localPath(exportFolderDialog.selectedFolder)
                }
            }
        }

        KIsland {
            KIslandRow {
                label: qsTr("Export to CSV")
                chevron: true
                interactive: true
                toolTipText: qsTr("Open the CSV export tab")
                onClicked: if (root.store) root.store.openCsvExportSettings()
            }

            KIslandRow {
                label: qsTr("Export to XTF")
                interactive: true
                onClicked: core.exportPlotAsXTF(exportGroup.currentExportPath())
            }

            KIslandRow {
                label: qsTr("Complex signal to CSV")
                interactive: true
                onClicked: core.exportComplexToCSV(exportGroup.currentExportPath())
            }

            KIslandRow {
                label: qsTr("USBL to CSV")
                interactive: true
                onClicked: core.exportUSBLToCSV(exportGroup.currentExportPath())
            }
        }
    }

    // ── Интерфейс ─────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        id: interfaceGroup
        title: qsTr("Interface")
        description: qsTr("Language, theme, UI scale and panel visibility.")
        stateStore: root.store
        stateKey: "app.preference"
        collapsedByDefault: false

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Language:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            KTabBar {
                id: langTabBar
                width: parent.width
                fontPixelSize: Tokens.fontLg
                options: [
                    { label: "English", value: 0 },
                    { label: "Русский", value: 1 },
                    { label: "Polski",  value: 2 }
                ]
                currentValue: langController ? langController.currentIndex : 0
                onValueSelected: function(v) { if (langController) langController.apply(v) }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Theme:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: appThemeHolder
                width: parent.width
                property int selectedIndex: 0

                readonly property var names: ["Dark","S.Dark","Light","S.Light","OneDark","Monokai","Kimbie","Solarized","Desert","Olive","Dracula","Nord"]
                readonly property int gap: Tokens.spaceXs
                readonly property int cellMinW: Math.round(80 * AppPalette.scale)
                readonly property int cols: Tokens.gridColumns(width, cellMinW, gap, 4)
                readonly property int itemH: Tokens.controlHMd
                readonly property real itemW: (width - (cols - 1) * gap) / cols
                readonly property int rows: Math.ceil(names.length / cols)
                height: rows * itemH + (rows - 1) * gap

                onSelectedIndexChanged: if (theme) theme.themeID = selectedIndex
                Component.onCompleted: if (theme) theme.themeID = selectedIndex

                Repeater {
                    model: appThemeHolder.names.length
                    delegate: Rectangle {
                        id: themeCell
                        required property int index
                        readonly property bool sel: index === appThemeHolder.selectedIndex
                        x: (index % appThemeHolder.cols) * (appThemeHolder.itemW + appThemeHolder.gap)
                        y: Math.floor(index / appThemeHolder.cols) * (appThemeHolder.itemH + appThemeHolder.gap)
                        width: appThemeHolder.itemW
                        height: appThemeHolder.itemH
                        radius: Tokens.radiusMd
                        color: sel ? AppPalette.accentBg
                               : (themeMa.pressed ? AppPalette.bgDeep : (themeMa.containsMouse ? AppPalette.cardHover : AppPalette.bg))
                        border.width: Tokens.cardBorderWidth
                        border.color: sel ? AppPalette.accentBorder : AppPalette.border
                        scale: themeMa.pressed ? 0.97 : (themeMa.containsMouse ? 1.03 : 1.0)
                        Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
                        Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

                        activeFocusOnTab: true
                        Keys.onReturnPressed: appThemeHolder.selectedIndex = index
                        Keys.onEnterPressed:  appThemeHolder.selectedIndex = index
                        Keys.onSpacePressed:  appThemeHolder.selectedIndex = index

                        Text {
                            anchors.centerIn: parent
                            text: appThemeHolder.names[index]
                            color: themeCell.sel ? AppPalette.accentText : AppPalette.textStrong
                            font.pixelSize: Tokens.fontBase; font.bold: true
                            elide: Text.ElideRight
                        }

                        KFocusRing { id: focusRing }

                        MouseArea {
                            id: themeMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onPressed: focusRing.suppress()
                            onClicked: { themeCell.forceActiveFocus(); appThemeHolder.selectedIndex = index }
                        }
                    }
                }

                Settings { category: "main/ui"; property alias appTheme: appThemeHolder.selectedIndex }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Toolset:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: instrumentsGradeHolder
                width: parent.width
                height: gradeTabBar.implicitHeight

                KTabBar {
                    id: gradeTabBar
                    width: parent.width
                    fontPixelSize: Tokens.fontLg
                    options: [
                        { label: qsTr("Fish Finders"),  value: 0 },
                        { label: qsTr("Bottom Track"),  value: 1 },
                        { label: qsTr("Maximum"),       value: 2 }
                    ]
                    currentValue: theme ? theme.instrumentsGrade : 0
                    onValueSelected: function(v) { if (theme) theme.instrumentsGrade = v }
                }
            }
        }

        // UI scale — DPI auto-detect + user override. Persisted via theme.manualScale.
        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("UI scale:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
            }

            KTabBar {
                id: uiScaleTabBar
                width: parent.width
                fontPixelSize: Tokens.fontLg
                options: [
                    { label: "75%",  value: 0.75 },
                    { label: "100%", value: 1.00 },
                    { label: "125%", value: 1.25 },
                    { label: "150%", value: 1.50 },
                    { label: "200%", value: 2.00 }
                ]
                currentValue: theme ? theme.manualScale : 1.0
                onValueSelected: function(v) { if (theme) theme.manualScale = v }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Sidebar position:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            KTabBar {
                width: parent.width
                fontPixelSize: Tokens.fontLg
                options: [
                    { label: qsTr("Left"),  value: "left"  },
                    { label: qsTr("Right"), value: "right" }
                ]
                currentValue: root.store.settingsSide
                onValueSelected: function(value) { root.store.settingsSide = value }
            }
        }

        // ── Merged from former "Interface" group ──────────────────────────

        KIsland {
            KIslandRow {
                label: qsTr("Hide important notifications")
                toolTipText: qsTr("Auto-hide warning notifications like info ones")
                interactive: true
                onClicked: hideNotificationsSwitch.click()
                KSwitch {
                    id: hideNotificationsSwitch
                    flat: true
                    checked: root.store ? root.store.hideImportantNotifications : false
                    onToggled: if (root.store) root.store.hideImportantNotifications = checked
                }
            }

            KIslandRow {
                label: qsTr("Hide UI elements for missing data")
                toolTipText: qsTr("Hide echogram controls when there is no matching data; off shows everything")
                interactive: true
                onClicked: hideEmptyControlsSwitch.click()
                KSwitch {
                    id: hideEmptyControlsSwitch
                    flat: true
                    checked: root.store ? root.store.hideEmptyEchogramControls : true
                    onToggled: if (root.store) root.store.hideEmptyEchogramControls = checked
                }
            }

            KIslandRow {
                label: qsTr("Workspace shift")
                toolTipText: qsTr("Shift the workspace aside when the settings panel opens, instead of overlaying on top")
                interactive: true
                onClicked: workspaceShiftSwitch.click()
                KSwitch {
                    id: workspaceShiftSwitch
                    flat: true
                    checked: root.store.settingsPushContent
                    onToggled: root.store.settingsPushContent = checked
                }
            }

            KIslandRow {
                visible: Qt.platform.os === "windows" && appUtils.instanceIndex === 1
                label: qsTr("Bring window to front")
                toolTipText: qsTr("Raise and focus the app window on key events")
                interactive: true
                onClicked: bringToFrontSwitch.click()
                KSwitch {
                    id: bringToFrontSwitch
                    flat: true
                    checked: core.bringWindowToFrontEnabled
                    onToggled: core.bringWindowToFrontEnabled = checked
                }
            }

            KIslandRow {
                visible: Qt.platform.os === "android" || Qt.platform.os === "ios"
                label: qsTr("Rotate layout with device")
                interactive: true
                onClicked: rotateLayoutSwitch.click()
                KSwitch {
                    id: rotateLayoutSwitch
                    flat: true
                    checked: root.store ? root.store.rotateLayoutEnabled : true
                    onToggled: if (root.store) root.store.rotateLayoutEnabled = checked
                }
            }

            KIslandRow {
                visible: Qt.platform.os !== "android"
                label: qsTr("Key bindings")
                chevron: true
                interactive: true
                onClicked: { hotkeysLoader.active = true; hotkeysLoader.item.open() }
            }

            KIslandRow {
                label: qsTr("Quick action menu")
                chevron: true
                interactive: true
                onClicked: if (root.store) root.store.openQuickActionsSettings()
            }

            KIslandRow {
                label: qsTr("UI Saving")
                chevron: true
                interactive: true
                onClicked: if (root.store) root.store.openUiSavingSettings()
            }

            KIslandRow {
                label: qsTr("Console")
                chevron: true
                interactive: true
                toolTipText: qsTr("Colour marking and log buffer size")
                onClicked: if (root.store) root.store.openConsoleSettings()
            }

            KIslandRow {
                label: qsTr("About")
                chevron: true
                interactive: true
                onClicked: if (root.store) root.store.openAboutSettings()
            }

            KIslandRow {
                visible: !!(root.store && root.store.developerMode)
                label: qsTr("Developer mode")
                labelColor: root._bright
                toolTipText: qsTr("Unlocks panels and controls meant for development. Turning it off hides this row again.")
                interactive: true
                onClicked: developerModeSwitch.click()

                KSwitch {
                    id: developerModeSwitch
                    flat: true
                    checked: !!(root.store && root.store.developerMode)
                    onToggled: if (root.store) root.store.developerMode = checked
                }
            }
        }

        Loader {
            id: hotkeysLoader
            active: false
            source: "qrc:/qml/settings/HotkeysDialog.qml"
            onLoaded: { if (item) item.store = root.store }
        }
    }

    // ── Workspace Layout ──────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Workspace")
        description: qsTr("Workspace layouts and pane editing.")
        stateStore: root.store
        stateKey: "app.layoutPlacement"

        KIsland {
            KIslandRow {
                label: qsTr("Edit")
                toolTipText: qsTr("Edit workspace panes")
                interactive: true
                onClicked: editableModeSwitch.click()
                KSwitch {
                    id: editableModeSwitch
                    flat: true
                    checked: root.store.editableMode
                    onToggled: { root.store.editableMode = checked }
                }
            }

            KIslandRow {
                label: qsTr("Global pop-up")
                toolTipText: qsTr("Floating window over the workspace, independent of the layout")
                interactive: true
                onClicked: globalPopupSwitch.click()
                KSwitch {
                    id: globalPopupSwitch
                    flat: true
                    checked: root.store.globalPopupEnabled
                    onToggled: { root.store.globalPopupEnabled = checked }
                }
            }
        }

        KIsland {
            Repeater {
                model: root.store.layouts.length
                delegate: KIslandRow {
                    id: layoutRow
                    required property int index

                    readonly property int layoutIndex: index
                    readonly property var layoutEntry: (layoutIndex >= 0 && layoutIndex < root.store.layouts.length) ? root.store.layouts[layoutIndex] : null
                    readonly property bool selected: layoutIndex === root.store.activeLayoutIndex

                    label: qsTr("Layout %1").arg(layoutIndex + 1)
                    labelColor: selected ? "#FDE68A" : AppPalette.textStrong
                    caption: selected ? qsTr("Active") : ""
                    fillColor: selected ? Qt.rgba(0.98, 0.80, 0.08, AppPalette.isDark ? 0.14 : 0.20) : "transparent"
                    verticalPadding: Tokens.spaceSm
                    interactive: true
                    onClicked: root.store.applyLayout(layoutIndex)

                    leading: LayoutSnapshotPreview {
                        width: Math.round(84 * AppPalette.scale)
                        height: Math.round(64 * AppPalette.scale)
                        layoutSnapshot: layoutRow.layoutEntry && layoutRow.layoutEntry.layout ? layoutRow.layoutEntry.layout : layoutRow.layoutEntry
                        popupLinks: layoutRow.layoutEntry && layoutRow.layoutEntry.popupLinks ? layoutRow.layoutEntry.popupLinks : []
                        redrawDebounceMs: 48
                    }

                    KCircleIconButton {
                        visible: root.store.layouts.length > 1
                        width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd; iconSource: ""; glyph: "×"
                        glyphPixelSize: Tokens.iconSm; glyphColor: AppPalette.textSecond; fillColor: AppPalette.controlRaised
                        fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                        borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover; showGlyphWithIcon: true
                        toolTipText: qsTr("Delete layout")
                        onClicked: root.store.deleteLayoutAt(layoutRow.layoutIndex)
                    }
                }
            }
        }

        KButton {
            width: parent.width
            text: qsTr("Create layout")
            onClicked: root.store.openCreateLayoutSettings()
        }
    }

    // ── Widgets ───────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Widget panels")
        description: qsTr("On-scene data panels: grid size and drag-in widgets.")
        stateStore: root.store
        stateKey: "app.widgets"

        KIsland {
            KIslandRow {
                id: usblPanelRow
                readonly property bool shown: !!(root.store && root.store.usblPanelShown)

                visible: !!(root.store && root.store.usblPanelListed)
                label: qsTr("USBL")
                labelColor: shown ? "#FDE68A" : AppPalette.textStrong
                fillColor:    shown ? root._shownFill    : "transparent"
                hoverColor:   shown ? root._shownHover   : AppPalette.cardHover
                pressedColor: shown ? root._shownPressed : AppPalette.bgHover
                verticalPadding: Tokens.spaceSm
                interactive: true
                onClicked: if (root.store) root.store.setUsblPanelShown(!usblPanelRow.shown)

                leading: WidgetGridPreview {
                    width: Math.round(84 * AppPalette.scale)
                    height: Math.round(64 * AppPalette.scale)
                    def: root.store ? root.store.usblPanelDef : null
                }

                KCircleIconButton {
                    width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd
                    iconSource: "qrc:/icons/ui/pencil.svg"; iconTintColor: AppPalette.textSecond
                    fillColor: AppPalette.controlRaised
                    fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover
                    toolTipText: qsTr("Edit panel")
                    onClicked: root.store.openUsblPanelSettings()
                }
            }

            KIslandRow {
                id: standPanelRow
                readonly property bool shown: !!(root.store && root.store.standPanelShown)

                visible: !!(root.store && root.store.standPanelListed)
                label: qsTr("Stand")
                labelColor: shown ? "#FDE68A" : AppPalette.textStrong
                fillColor:    shown ? root._shownFill    : "transparent"
                hoverColor:   shown ? root._shownHover   : AppPalette.cardHover
                pressedColor: shown ? root._shownPressed : AppPalette.bgHover
                verticalPadding: Tokens.spaceSm
                interactive: true
                onClicked: if (root.store) root.store.setStandPanelShown(!standPanelRow.shown)

                leading: WidgetGridPreview {
                    width: Math.round(84 * AppPalette.scale)
                    height: Math.round(64 * AppPalette.scale)
                    def: root.store ? root.store.standPanelDef : null
                }

                KCircleIconButton {
                    width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd
                    iconSource: "qrc:/icons/ui/pencil.svg"; iconTintColor: AppPalette.textSecond
                    fillColor: AppPalette.controlRaised
                    fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover
                    toolTipText: qsTr("Edit panel")
                    onClicked: root.store.openStandPanelSettings()
                }
            }

            KIslandRow {
                id: servoPanelRow
                readonly property bool shown: !!(root.store && root.store.servoPanelShown)

                visible: !!(root.store && root.store.servoPanelListed)
                label: qsTr("Servo")
                labelColor: shown ? "#FDE68A" : AppPalette.textStrong
                fillColor:    shown ? root._shownFill    : "transparent"
                hoverColor:   shown ? root._shownHover   : AppPalette.cardHover
                pressedColor: shown ? root._shownPressed : AppPalette.bgHover
                verticalPadding: Tokens.spaceSm
                interactive: true
                onClicked: if (root.store) root.store.setServoPanelShown(!servoPanelRow.shown)

                leading: WidgetGridPreview {
                    width: Math.round(84 * AppPalette.scale)
                    height: Math.round(64 * AppPalette.scale)
                    def: root.store ? root.store.servoPanelDef : null
                }

                KCircleIconButton {
                    width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd
                    iconSource: "qrc:/icons/ui/pencil.svg"; iconTintColor: AppPalette.textSecond
                    fillColor: AppPalette.controlRaised
                    fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover
                    toolTipText: qsTr("Edit panel")
                    onClicked: root.store.openServoPanelSettings()
                }
            }

            Repeater {
                model: root.store.widgets.length
                delegate: KIslandRow {
                    id: widgetRow
                    required property int index

                    readonly property int widgetIndex: index
                    readonly property var def: (widgetIndex >= 0 && widgetIndex < root.store.widgets.length) ? root.store.widgets[widgetIndex] : null
                    readonly property bool shown: !!(root.store && widgetRow.def && root.store.widgetShown(widgetRow.def.id))

                    visible: !!(root.store && root.store.widgetListed(widgetRow.def))
                    label: qsTr("Panel %1").arg(widgetRow.widgetIndex + 1)
                    labelColor: shown ? "#FDE68A" : AppPalette.textStrong
                    caption: widgetRow.def ? (widgetRow.def.cols + "×" + widgetRow.def.rows) : ""
                    fillColor:    shown ? root._shownFill    : "transparent"
                    hoverColor:   shown ? root._shownHover   : AppPalette.cardHover
                    pressedColor: shown ? root._shownPressed : AppPalette.bgHover
                    verticalPadding: Tokens.spaceSm
                    interactive: true
                    onClicked: {
                        if (root.store && widgetRow.def)
                            root.store.setWidgetShown(widgetRow.def.id, !widgetRow.shown)
                    }

                    leading: WidgetGridPreview {
                        width: Math.round(84 * AppPalette.scale)
                        height: Math.round(64 * AppPalette.scale)
                        def: widgetRow.def
                    }

                    Row {
                        spacing: Tokens.spaceSm

                        KCircleIconButton {
                            width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd
                            iconSource: "qrc:/icons/ui/pencil.svg"; iconTintColor: AppPalette.textSecond
                            fillColor: AppPalette.controlRaised
                            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                            borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover
                            toolTipText: qsTr("Edit panel")
                            onClicked: root.store.openWidgetEditSettings(widgetRow.widgetIndex)
                        }

                        KCircleIconButton {
                            width: Tokens.controlHMd; height: Tokens.controlHMd; rounded: false; cornerRadius: Tokens.radiusMd; iconSource: ""; glyph: "×"
                            glyphPixelSize: Tokens.iconSm; glyphColor: AppPalette.textSecond; fillColor: AppPalette.controlRaised
                            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2); fillPressedColor: AppPalette.bgDeep
                            borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover; showGlyphWithIcon: true
                            toolTipText: qsTr("Delete panel")
                            onClicked: root.store.deleteWidgetAt(widgetRow.widgetIndex)
                        }
                    }
                }
            }
        }

        KButton {
            width: parent.width
            text: qsTr("Create panel")
            enabled: !!root.store && root.store.canCreateWidget
            onClicked: root.store.openWidgetCreateSettings()
        }
    }


    // ── Датасет ───────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Dataset")
        description: qsTr("Black-stripe filtering and sonar mount-point offset.")
        stateStore: root.store
        stateKey: "app.dataset"
        collapsedByDefault: true

        Component.onCompleted: {
            core.setFixBlackStripesState(fixBlackStripesCheckButton.checked)
            core.setFixBlackStripesForwardSteps(fixBlackStripesForwardStepsSpinBox.value)
            core.setFixBlackStripesBackwardSteps(fixBlackStripesBackwardStepsSpinBox.value)
            core.setIsAttitudeExpected(sonarOffsetCheckButton.checked)
            if (sonarOffsetCheckButton.checked)
                dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, sonarOffsetValueY.value * 0.001, 0)
            core.setPosZeroing(zeroingPosButton.checked)
            core.setBottomTrackZeroing(zeroingBottomTrackButton.checked)
        }

        KIsland {
            KIslandRow {
                label: qsTr("FBS forward / backward:")
                labelColor: root._bright
                toolTipText: qsTr("Fills black stripes in the echogram by interpolating the given number of steps forward / backward.")
                interactive: true
                onClicked: fixBlackStripesCheckButton.click()

                Item {
                    width: fbsSlotRow.width
                    height: fbsSlotRow.height

                    // Eats clicks landing in the gaps around the spinboxes so they
                    // cannot bubble up to the row and flip the toggle.
                    MouseArea { anchors.fill: parent }

                    Row {
                        id: fbsSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: fixBlackStripesForwardStepsSpinBox
                            fontPixelSize: Tokens.fontLg
                            textColor: root._bright
                            width: Math.round(93 * AppPalette.scale)
                            height: Tokens.controlHMd
                            from: 0; to: 100; stepSize: 1; value: 5
                            onValueModified: function(v) { core.setFixBlackStripesForwardSteps(v) }
                        }

                        KSpinBox {
                            id: fixBlackStripesBackwardStepsSpinBox
                            fontPixelSize: Tokens.fontLg
                            textColor: root._bright
                            width: Math.round(93 * AppPalette.scale)
                            height: Tokens.controlHMd
                            from: 0; to: 100; stepSize: 1; value: 5
                            onValueModified: function(v) { core.setFixBlackStripesBackwardSteps(v) }
                        }

                        KSwitch {
                            id: fixBlackStripesCheckButton
                            flat: true
                            onToggled: core.setFixBlackStripesState(checked)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("S.offset XY, mm:")
                labelColor: root._bright
                toolTipText: qsTr("Sonar mount-point offset along the X / Y axes, in millimeters.")
                interactive: true
                onClicked: sonarOffsetCheckButton.click()

                Item {
                    width: sonarOffsetSlotRow.width
                    height: sonarOffsetSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: sonarOffsetSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: sonarOffsetValueX
                            fontPixelSize: Tokens.fontLg
                            textColor: root._bright
                            width: Math.round(93 * AppPalette.scale)
                            height: Tokens.controlHMd
                            from: -9999; to: 9999; stepSize: 50; value: 0
                            onValueModified: function(v) {
                                if (sonarOffsetCheckButton.checked)
                                    dataset.setSonarOffset(v * 0.001, sonarOffsetValueY.value * 0.001, 0)
                            }
                        }

                        KSpinBox {
                            id: sonarOffsetValueY
                            fontPixelSize: Tokens.fontLg
                            textColor: root._bright
                            width: Math.round(93 * AppPalette.scale)
                            height: Tokens.controlHMd
                            from: -9999; to: 9999; stepSize: 50; value: 0
                            onValueModified: function(v) {
                                if (sonarOffsetCheckButton.checked)
                                    dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, v * 0.001, 0)
                            }
                        }

                        KSwitch {
                            id: sonarOffsetCheckButton
                            flat: true
                            onToggled: {
                                if (checked) dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, sonarOffsetValueY.value * 0.001, 0)
                                else         dataset.setSonarOffset(0, 0, 0)
                                core.setIsAttitudeExpected(checked)
                            }
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Pos zeroing")
                labelColor: root._bright
                toolTipText: qsTr("Zeroes position coordinates relative to the start point.")
                interactive: true
                onClicked: zeroingPosButton.click()

                KSwitch {
                    id: zeroingPosButton
                    flat: true
                    onToggled: core.setPosZeroing(checked)
                }
            }

            KIslandRow {
                label: qsTr("Bottom track zeroing")
                labelColor: root._bright
                toolTipText: qsTr("Zeroes the bottom-track depth reference.")
                interactive: true
                onClicked: zeroingBottomTrackButton.click()

                KSwitch {
                    id: zeroingBottomTrackButton
                    flat: true
                    onToggled: core.setBottomTrackZeroing(checked)
                }
            }

            KIslandRow {
                visible: instruments >= 1
                label: qsTr("TGC")
                toolTipText: qsTr("Open TGC settings")
                chevron: true
                interactive: true
                onClicked: if (root.store) root.store.openTgcSettings()
            }
        }

        Settings { category: "main/blackStripes"; property alias fixBlackStripesCheckButton: fixBlackStripesCheckButton.checked }
        Settings { category: "main/blackStripes"; property alias fixBlackStripesForwardStepsSpinBox: fixBlackStripesForwardStepsSpinBox.value }
        Settings { category: "main/blackStripes"; property alias fixBlackStripesBackwardStepsSpinBox: fixBlackStripesBackwardStepsSpinBox.value }

        Settings { category: "main/sonarOffset"; property alias sonarOffsetCheckButton: sonarOffsetCheckButton.checked }
        Settings { category: "main/sonarOffset"; property alias sonarOffsetValueX: sonarOffsetValueX.value }
        Settings { category: "main/sonarOffset"; property alias sonarOffsetValueY: sonarOffsetValueY.value }

        Settings { category: "main/dataset"; property alias zeroingPosButtonCheched: zeroingPosButton.checked }
        Settings { category: "main/dataset"; property alias zeroingBottomTrackButtonChecked: zeroingBottomTrackButton.checked }
    }

    // Boat Track
    SettingsGroup {
        id: boatTrackGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Boat track")
        description: qsTr("Vessel track displayed in the 3D scene.")
        stateStore: root.store
        stateKey: "app.boattrack"
        headerActions: ShowIn3DAction {
            active: root.store ? root.store.boatTrackVisible : false
            onClicked: if (root.store) root.store.boatTrackVisible = !root.store.boatTrackVisible
        }
        expandable: false   // no body controls — header + description only
        collapsedByDefault: true

    }

    // Bottom Track
    SettingsGroup {
        id: btGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Bottom Track")
        description: qsTr("Bottom detection presets, thresholds and search window.")
        stateStore: root.store
        stateKey: "app.bottomtrack"
        headerActions: ShowIn3DAction {
            active: root.store ? root.store.bottomTrackVisible : false
            onClicked: if (root.store) root.store.bottomTrackVisible = !root.store.bottomTrackVisible
        }
        collapsedByDefault: false

        readonly property int spinW: Math.round(115 * AppPalette.scale)

        function refreshParams() {
            if (!root.targetPlot) return
            root.targetPlot.refreshDistParams(
                btPresetHolder.selectedIndex,
                bottomTrackWindow.checked    ? bottomTrackWindowValue.value : 1,
                bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value * 0.01 : 0,
                bottomTrackMinRange.checked  ? bottomTrackMinRangeValue.value / 1000 : 0,
                bottomTrackMaxRange.checked  ? bottomTrackMaxRangeValue.value / 1000 : 1000,
                bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.value / 100  : 1,
                bottomTrackThreshold.checked ? bottomTrackThresholdValue.value / 100  : 0
            )
        }

        function doDistProcessing() {
            if (!root.targetPlot) return
            root.targetPlot.doDistProcessing(
                btPresetHolder.selectedIndex,
                bottomTrackWindow.checked    ? bottomTrackWindowValue.value : 1,
                bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value * 0.01 : 0,
                bottomTrackMinRange.checked  ? bottomTrackMinRangeValue.value / 1000 : 0,
                bottomTrackMaxRange.checked  ? bottomTrackMaxRangeValue.value / 1000 : 1000,
                bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.value / 100  : 1,
                bottomTrackThreshold.checked ? bottomTrackThresholdValue.value / 100  : 0,
                false
            )
        }

        Component.onCompleted: refreshParams()

        Item {
            id: btPresetHolder
            width: parent.width
            height: presetTabBar.implicitHeight
            property int selectedIndex: 0

            onSelectedIndexChanged: if (root.targetPlot) root.targetPlot.setPreset(selectedIndex)

            KTabBar {
                id: presetTabBar
                width: parent.width
                options: [
                    { label: qsTr("Normal 2D"), value: 0,
                      tip: qsTr("Bottom search settings for an ordinary sounder with one beam pointing down.") },
                    { label: qsTr("Narrow 2D"), value: 1,
                      tip: qsTr("For a sounder with a narrow beam: the bottom echo is shorter, so the search is finer.") },
                    { label: qsTr("Side-Scan"), value: 2,
                      tip: qsTr("For a side-scan sonar: the beam looks sideways, not down.") }
                ]
                currentValue: btPresetHolder.selectedIndex
                onValueSelected: function(v) { btPresetHolder.selectedIndex = v }
            }

            Settings { category: "scene2d/bottomTrack"; property alias bottomTrackList: btPresetHolder.selectedIndex }
        }

        KIsland {
            KIslandRow {
                label: qsTr("Gain slope")
                toolTipText: qsTr("Lifts the weak echo at long range so the bottom is found far out too.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackGainSlope.click()

                Item {
                    width: gainSlopeSlotRow.width
                    height: gainSlopeSlotRow.height

                    // Eats clicks landing in the gaps around the spinbox so they
                    // cannot bubble up to the row and flip the toggle.
                    MouseArea { anchors.fill: parent }

                    Row {
                        id: gainSlopeSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackGainSlopeValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 0; to: 300; stepSize: 10; value: 100; divisor: 100; decimals: 2
                            onValueModified: function(v) { if (bottomTrackGainSlope.checked && root.targetPlot) root.targetPlot.setGainSlope(v / 100) }
                        }

                        KSwitch {
                            id: bottomTrackGainSlope
                            flat: true
                            checked: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setGainSlope(bottomTrackGainSlopeValue.value / 100)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Threshold")
                toolTipText: qsTr("How clear the echo must be to count as bottom. Where it is weaker, the bottom line breaks.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackThreshold.click()

                Item {
                    width: thresholdSlotRow.width
                    height: thresholdSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: thresholdSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackThresholdValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 0; to: 200; stepSize: 5; value: 0; divisor: 100; decimals: 2
                            onValueModified: function(v) { if (bottomTrackThreshold.checked && root.targetPlot) root.targetPlot.setThreshold(v / 100) }
                        }

                        KSwitch {
                            id: bottomTrackThreshold
                            flat: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setThreshold(bottomTrackThresholdValue.value / 100)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Horizontal window")
                toolTipText: qsTr("Smooths the bottom line over neighbouring pings. Wider is smoother but lags more.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackWindow.click()

                Item {
                    width: windowSlotRow.width
                    height: windowSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: windowSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackWindowValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 1; to: 100; stepSize: 2; value: 1
                            onValueModified: function(v) { if (bottomTrackWindow.checked && root.targetPlot) root.targetPlot.setWindowSize(v) }
                        }

                        KSwitch {
                            id: bottomTrackWindow
                            flat: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setWindowSize(bottomTrackWindowValue.value)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Vertical gap, %")
                toolTipText: qsTr("Refines the depth near the one already found and rejects false bottoms from re-reflections. 0 is off.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackVerticalGap.click()

                Item {
                    width: verticalGapSlotRow.width
                    height: verticalGapSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: verticalGapSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackVerticalGapValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 0; to: 100; stepSize: 2; value: 10
                            onValueModified: function(v) { if (bottomTrackVerticalGap.checked && root.targetPlot) root.targetPlot.setVerticalGap(v * 0.01) }
                        }

                        KSwitch {
                            id: bottomTrackVerticalGap
                            flat: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Min range, m")
                toolTipText: qsTr("The bottom is not searched for closer than this.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackMinRange.click()

                Item {
                    width: minRangeSlotRow.width
                    height: minRangeSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: minRangeSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackMinRangeValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 0; to: 200000; stepSize: 10; value: 0; divisor: 1000; decimals: 2
                            onValueModified: function(v) { if (bottomTrackMinRange.checked && root.targetPlot) root.targetPlot.setRangeMin(v / 1000) }
                        }

                        KSwitch {
                            id: bottomTrackMinRange
                            flat: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setRangeMin(bottomTrackMinRangeValue.value / 1000)
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Max range, m")
                toolTipText: qsTr("The bottom is not searched for further than this.")
                labelColor: root._bright
                interactive: true
                onClicked: bottomTrackMaxRange.click()

                Item {
                    width: maxRangeSlotRow.width
                    height: maxRangeSlotRow.height

                    MouseArea { anchors.fill: parent }

                    Row {
                        id: maxRangeSlotRow
                        spacing: Tokens.spaceSm

                        KSpinBox {
                            id: bottomTrackMaxRangeValue
                            width: btGroup.spinW
                            height: Tokens.controlHMd
                            from: 0; to: 200000; stepSize: 1000; value: 100000; divisor: 1000; decimals: 2
                            onValueModified: function(v) { if (bottomTrackMaxRange.checked && root.targetPlot) root.targetPlot.setRangeMax(v / 1000) }
                        }

                        KSwitch {
                            id: bottomTrackMaxRange
                            flat: true
                            onToggled: if (checked && root.targetPlot) root.targetPlot.setRangeMax(bottomTrackMaxRangeValue.value / 1000)
                        }
                    }
                }
            }
        }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackGainSlope: bottomTrackGainSlope.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackGainSlopeValue: bottomTrackGainSlopeValue.value }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackThreshold: bottomTrackThreshold.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackThresholdValue: bottomTrackThresholdValue.value }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackWindow: bottomTrackWindow.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackWindowValue: bottomTrackWindowValue.value }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackVerticalGap: bottomTrackVerticalGap.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackMinRange: bottomTrackMinRange.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value }

        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackMaxRange: bottomTrackMaxRange.checked }
        Settings { category: "scene2d/bottomTrack"; property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value }

        // Action buttons
        KButton {
            width: parent.width
            fontPixelSize: Tokens.fontLg
            text: qsTr("Processing")
            toolTipText: qsTr("Recompute the bottom over the whole open file. Manual edits are left alone.")
            onClicked: btGroup.doDistProcessing()
        }
    }

    // Isobaths
    SettingsGroup {
        id: isobathsGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Isobaths")
        description: qsTr("Equal-depth contour lines on the surface.")
        stateStore: root.store
        stateKey: "app.isobaths"
        headerActions: ShowIn3DAction {
            active: root.store ? root.store.isobathsVisible : false
            onClicked: if (root.store) root.store.isobathsVisible = !root.store.isobathsVisible
        }
        collapsedByDefault: true

        readonly property int ctrlW: Math.round(170 * AppPalette.scale)
        property var exportSurfaceFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/KoggerApp/exports"
        property string exportSurfacePathSource: core.defaultExportDirectory() + "/surface.csv"

        // Hotkey API — invoked from WorkspaceStore.applyIsobathsHotkey().
        function prevTheme() {
            if (root.store) root.store.isobathsThemeIndex = Math.max(0, root.store.isobathsThemeIndex - 1)
        }
        function nextTheme() {
            if (root.store) root.store.isobathsThemeIndex = Math.min(isobathsTheme.model.length - 1, root.store.isobathsThemeIndex + 1)
        }
        function stepDown(step) {
            var d = step === undefined ? 1 : step
            for (var i = 0; i < d; ++i) isobathsSurfaceLineStepSizeSpinBox.decrement()
        }
        function stepUp(step) {
            var d = step === undefined ? 1 : step
            for (var i = 0; i < d; ++i) isobathsSurfaceLineStepSizeSpinBox.increment()
        }

        function isoSourceUrl(value) {
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
        function isoDisplayUrl(value) {
            var s = isoSourceUrl(value)
            if (!s.length) return ""
            try { return decodeURIComponent(s) } catch (e) { return s }
        }
        function isoEffectiveSource(displayText, storedSource) {
            if (!displayText || !displayText.length) return ""
            if (storedSource && displayText === isoDisplayUrl(storedSource)) return storedSource
            return displayText
        }
        function currentExportSurfacePath() {
            var p = isoSourceUrl(isoEffectiveSource(exportSurfacePathText.text, exportSurfacePathSource))
            if (!p || p.length === 0)
                p = core.defaultExportDirectory() + "/surface.csv"
            return p
        }
        function hasInvalidFileName(p) {
            var name = p.replace(/^.*[\\/]/, "")
            if (name.length === 0) return true
            return /[<>:"|?*\x00-\x1F]/.test(name)
        }

        Component.onCompleted: {
            if (!exportSurfacePathSource || exportSurfacePathSource.length === 0)
                exportSurfacePathSource = core.defaultExportDirectory() + "/surface.csv"
            exportSurfacePathText.text = isoDisplayUrl(exportSurfacePathSource)
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Theme:")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                Layout.fillWidth: true
            }
            KCombo {
                id: isobathsTheme
                toolTipText: qsTr("Colour theme for isobaths (palette by depth)")
                Layout.preferredWidth: isobathsGroup.ctrlW
                model: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("Standard"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green")]
                swatchFor: function(i) { return IsobathsViewControlMenuController.themeStops(i) }
                currentIndex: root.store ? root.store.isobathsThemeIndex : 0
                onCurrentIndexChanged: if (root.store && root.store.isobathsThemeIndex !== currentIndex) root.store.isobathsThemeIndex = currentIndex
                Connections {
                    target: root.store
                    function onIsobathsThemeIndexChanged() {
                        if (isobathsTheme.currentIndex !== root.store.isobathsThemeIndex)
                            isobathsTheme.currentIndex = root.store.isobathsThemeIndex
                    }
                }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Edge limit, m:")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsEdgeLimitSpinBox
                toolTipText: qsTr("Max triangulation edge length")
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 10; to: 1000; stepSize: 5; value: 100
                editable: false
                onValueModified: function(v) { IsobathsViewControlMenuController.onEdgeLimitChanged(v) }
                Component.onCompleted: IsobathsViewControlMenuController.onEdgeLimitChanged(value)
                Settings { category: "scene3d/isobaths"; property alias isobathsEdgeLimitSpinBox: isobathsEdgeLimitSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Step, m:")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsSurfaceLineStepSizeSpinBox
                toolTipText: qsTr("Isobath interval — spacing between depth lines")
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 1; to: 200; stepSize: 1; value: 10
                divisor: 10; decimals: 1
                editable: false
                readonly property real realValue: value / 10
                onValueModified: function(v) { IsobathsViewControlMenuController.onSetSurfaceLineStepSize(v / 10) }
                Component.onCompleted: IsobathsViewControlMenuController.onSetSurfaceLineStepSize(realValue)
                Settings { category: "scene3d/isobaths"; property alias isobathsSurfaceLineStepSizeSpinBox: isobathsSurfaceLineStepSizeSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Extra width, m:")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                Layout.fillWidth: true
            }
            KSpinBox {
                id: extraWidthSpinBox
                toolTipText: qsTr("Surface extrapolation radius around the track")
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 5; to: 100; stepSize: 5; value: 10
                editable: false
                onValueModified: function(v) { IsobathsViewControlMenuController.onSetExtraWidth(v) }
                Component.onCompleted: IsobathsViewControlMenuController.onSetExtraWidth(value)
                Settings { category: "scene3d/isobaths"; property alias extraWidthSpinBox: extraWidthSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: exportSurfacePathText.activeFocus ? 1 : Tokens.cardBorderWidth
                border.color: exportSurfacePathText.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: exportSurfacePathText
                    activeFocusOnTab: true
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: exportSurfacePathText.selectAll() }
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    clip: true

                    Text {
                        visible: !exportSurfacePathText.text.length
                        text: qsTr("Enter path")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontBase
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            KButton {
                text: "..."
                toolTipText: qsTr("Choose the surface .csv file")
                Layout.fillWidth: false
                Layout.preferredWidth: Tokens.controlHMd
                Layout.maximumWidth: Tokens.controlHMd
                Layout.preferredHeight: Tokens.controlHMd
                horizontalPadding: 0
                verticalPadding: 0
                onClicked: {
                    exportSurfaceFileDialog.currentFolder = isobathsGroup.exportSurfaceFolder
                    exportSurfaceFileDialog.open()
                }
            }

            FileDialog {
                id: exportSurfaceFileDialog
                title: qsTr("Select folder and set .csv file name")
                currentFolder: isobathsGroup.exportSurfaceFolder
                fileMode: FileDialog.SaveFile
                nameFilters: ["CSV Files (*.csv)", "All Files (*)"]
                defaultSuffix: "csv"
                onCurrentFolderChanged: { isobathsGroup.exportSurfaceFolder = currentFolder }
                onAccepted: {
                    isobathsGroup.exportSurfaceFolder = exportSurfaceFileDialog.currentFolder
                    isobathsGroup.exportSurfacePathSource = isobathsGroup.isoSourceUrl(selectedFile)
                    if (!isobathsGroup.exportSurfacePathSource.toLowerCase().endsWith(".csv")) {
                        isobathsGroup.exportSurfacePathSource += ".csv"
                    }
                    exportSurfacePathText.text = isobathsGroup.isoDisplayUrl(isobathsGroup.exportSurfacePathSource)
                }
            }

            KButton {
                text: qsTr("Export to CSV")
                toolTipText: qsTr("Export the surface to CSV")
                fontPixelSize: Tokens.fontLg
                Layout.fillWidth: false
                Layout.preferredWidth: isobathsGroup.ctrlW
                Layout.maximumWidth: isobathsGroup.ctrlW
                Layout.preferredHeight: Tokens.controlHMd
                onClicked: {
                    var p = isobathsGroup.currentExportSurfacePath()
                    var hasN = typeof notifications !== "undefined" && notifications
                    if (isobathsGroup.hasInvalidFileName(p)) {
                        if (hasN) notifications.warning(qsTr("Invalid characters in file name"))
                        return
                    }
                    var ok = Scene3DControlMenuController.onExportToCSVButtonClicked(p)
                    if (hasN) {
                        if (ok) notifications.info(qsTr("Surface exported to %1").arg(p), p)
                        else    notifications.warning(qsTr("Surface export failed"))
                    }
                }
            }

            Settings { category: "main/export"; property alias exportSurfaceFolder:     isobathsGroup.exportSurfaceFolder }
            Settings { category: "main/export"; property alias exportSurfaceFolderText: isobathsGroup.exportSurfacePathSource }
        }
    }

    // Mpsaic
    SettingsGroup {
        id: mosaicGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Mosaic")
        description: qsTr("Side-scan mosaic visualisation.")
        stateStore: root.store
        stateKey: "app.mosaic"
        headerActions: ShowIn3DAction {
            active: root.store ? root.store.mosaicVisible : false
            onClicked: if (root.store) root.store.mosaicVisible = !root.store.mosaicVisible
        }
        collapsedByDefault: true

        readonly property int labelW: Math.round(140 * AppPalette.scale)
        readonly property int ctrlW:  Math.round(170 * AppPalette.scale)

        function setChannelNamesToBackend() {
            core.setMosaicChannels(channel1Combo.currentText, channel2Combo.currentText)
        }

        // Hotkey API — invoked from WorkspaceStore.applyMosaicHotkey().
        function prevTheme() {
            if (root.store) root.store.mosaicThemeIndex = Math.max(0, root.store.mosaicThemeIndex - 1)
        }
        function nextTheme() {
            if (root.store) root.store.mosaicThemeIndex = Math.min(mosaicTheme.model.length - 1, root.store.mosaicThemeIndex + 1)
        }
        function lowLevelUp(step) {
            var d = step === undefined ? 1 : step
            var v = Math.min(mosaicLevelsSlider.to, mosaicLevelsSlider.startValue + d)
            mosaicLevelsSlider.startValue = v
            if (mosaicLevelsSlider.startValue > mosaicLevelsSlider.stopValue)
                mosaicLevelsSlider.stopValue = mosaicLevelsSlider.startValue
        }
        function lowLevelDown(step) {
            var d = step === undefined ? 1 : step
            mosaicLevelsSlider.startValue = Math.max(mosaicLevelsSlider.from, mosaicLevelsSlider.startValue - d)
        }
        function highLevelUp(step) {
            var d = step === undefined ? 1 : step
            mosaicLevelsSlider.stopValue = Math.min(mosaicLevelsSlider.to, mosaicLevelsSlider.stopValue + d)
        }
        function highLevelDown(step) {
            var d = step === undefined ? 1 : step
            var v = Math.max(mosaicLevelsSlider.from, mosaicLevelsSlider.stopValue - d)
            mosaicLevelsSlider.stopValue = v
            if (mosaicLevelsSlider.stopValue < mosaicLevelsSlider.startValue)
                mosaicLevelsSlider.startValue = mosaicLevelsSlider.stopValue
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            KChartLevelCapsule {
                id: mosaicLevelsSlider
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                Layout.maximumHeight: Math.max(implicitHeight, mosaicControlsColumn.implicitHeight)
                cornerRadius: Tokens.radiusLg   // settings: rounded rectangle (2D overlay stays a pill)
                onStartValueChanged: MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                onStopValueChanged:  MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                Component.onCompleted: MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                Settings {
                    category: "scene3d/mosaic"
                    property alias mosaicLevelsStart: mosaicLevelsSlider.startValue
                    property alias mosaicLevelsStop:  mosaicLevelsSlider.stopValue
                }
            }

            ColumnLayout {
                id: mosaicControlsColumn
                Layout.fillWidth: true
                spacing: Tokens.spaceMd

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Theme:")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        Layout.fillWidth: true
                    }
                    KCombo {
                        id: mosaicTheme
                        Layout.preferredWidth: mosaicGroup.ctrlW
                        model: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
                        swatchFor: function(i) { return MosaicViewControlMenuController.themeStops(i) }
                        currentIndex: root.store ? root.store.mosaicThemeIndex : 0
                        onCurrentIndexChanged: if (root.store && root.store.mosaicThemeIndex !== currentIndex) root.store.mosaicThemeIndex = currentIndex
                        Connections {
                            target: root.store
                            function onMosaicThemeIndexChanged() {
                                if (mosaicTheme.currentIndex !== root.store.mosaicThemeIndex)
                                    mosaicTheme.currentIndex = root.store.mosaicThemeIndex
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Channels:")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        spacing: Tokens.spaceMd
                        Layout.preferredWidth: mosaicGroup.ctrlW

                        KCombo {
                            id: channel1Combo
                            Layout.preferredWidth: mosaicGroup.ctrlW
                            property bool suppressTextSignal: false

                            onCurrentTextChanged: {
                                if (suppressTextSignal) return
                                mosaicGroup.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()
                                let index = model.indexOf(core.ch1Name)
                                // Auto-fill consistent with WorkspaceStore.pushMosaicChannelsFromCore:
                                // list[0] is "None", first real channel is list[1].
                                if (index >= 0) channel1Combo.currentIndex = index
                                else if (model.length > 1) channel1Combo.currentIndex = 1
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel1Combo.suppressTextSignal = true

                                    channel1Combo.model = []
                                    channel1Combo.model = list

                                    let newIndex = list.indexOf(core.ch1Name)
                                    if (newIndex < 0) newIndex = list.length > 1 ? 1 : 0

                                    // Force re-sync: model reset puts the inner
                                    // ComboBox.currentIndex at 0. If root.currentIndex
                                    // already equals newIndex, onCurrentIndexChanged
                                    // won't fire and the inner combo stays at 0.
                                    // Bouncing through -1 guarantees the signal fires.
                                    channel1Combo.currentIndex = -1
                                    channel1Combo.currentIndex = newIndex

                                    mosaicGroup.setChannelNamesToBackend()

                                    channel1Combo.suppressTextSignal = false
                                }
                            }
                        }

                        KCombo {
                            id: channel2Combo
                            Layout.preferredWidth: mosaicGroup.ctrlW
                            property bool suppressTextSignal: false

                            onCurrentTextChanged: {
                                if (suppressTextSignal) return
                                mosaicGroup.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()
                                let index = model.indexOf(core.ch2Name)
                                // Only second real channel (index 2) auto-fills.
                                // Single-channel datasets leave ch2 on "None" (index 0)
                                // so the mosaic processor doesn't receive ch1 twice.
                                if (index >= 0)             channel2Combo.currentIndex = index
                                else if (model.length > 2)  channel2Combo.currentIndex = 2
                                else                        channel2Combo.currentIndex = 0
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel2Combo.suppressTextSignal = true

                                    channel2Combo.model = []
                                    channel2Combo.model = list

                                    let newIndex = list.indexOf(core.ch2Name)
                                    if (newIndex < 0) {
                                        // Same rule as Component.onCompleted: only auto-fill
                                        // when a second real channel exists. Otherwise keep
                                        // ch2 on "None" (index 0) to avoid duplicating ch1.
                                        newIndex = list.length > 2 ? 2 : 0
                                    }

                                    channel2Combo.currentIndex = -1
                                    channel2Combo.currentIndex = newIndex

                                    mosaicGroup.setChannelNamesToBackend()

                                    channel2Combo.suppressTextSignal = false
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Angle, °:")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        spacing: Tokens.spaceMd
                        Layout.preferredWidth: mosaicGroup.ctrlW

                        KSpinBox {
                            id: mosaicLAngleOffset
                            toolTipText: qsTr("Left-side beam angle offset for the mosaic, °")
                            Layout.preferredWidth: mosaicGroup.ctrlW
                            from: -90; to: 90; stepSize: 1; value: 0
                            onValueModified: function(v) {
                                MosaicViewControlMenuController.onSetLAngleOffset(v)
                                dataset.onSetLAngleOffset(v)
                            }
                            Component.onCompleted: {
                                MosaicViewControlMenuController.onSetLAngleOffset(value)
                                dataset.onSetLAngleOffset(value)
                            }
                            Settings { category: "scene3d/mosaic"; property alias mosaicLAngleOffset: mosaicLAngleOffset.value }
                        }

                        KSpinBox {
                            id: mosaicRAngleOffset
                            toolTipText: qsTr("Right-side beam angle offset for the mosaic, °")
                            Layout.preferredWidth: mosaicGroup.ctrlW
                            from: -90; to: 90; stepSize: 1; value: 0
                            onValueModified: function(v) {
                                MosaicViewControlMenuController.onSetRAngleOffset(v)
                                dataset.onSetRAngleOffset(v)
                            }
                            Component.onCompleted: {
                                MosaicViewControlMenuController.onSetRAngleOffset(value)
                                dataset.onSetRAngleOffset(value)
                            }
                            Settings { category: "scene3d/mosaic"; property alias mosaicRAngleOffset: mosaicRAngleOffset.value }
                        }
                    }
                }

                KSwitch {
                    id: mosaicTraceLine
                    text: qsTr("Trace line")
                    toolTipText: qsTr("Show the current mosaic trace line")
                    checked: true
                    Layout.fillWidth: true
                    onToggled: MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    Component.onCompleted: MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    Settings { category: "scene3d/mosaic"; property alias mosaicTraceLine: mosaicTraceLine.checked }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Data source:")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    KCircleIconButton {
                        visible: mosaicSource.currentIndex === 2
                        Layout.preferredWidth: Tokens.controlHMd
                        Layout.preferredHeight: Tokens.controlHMd
                        iconSource: "qrc:/icons/ui/settings.svg"
                        iconTintColor: AppPalette.accentBar
                        cornerRadius: Tokens.radiusSm   // square, not a circle
                        fillColor: AppPalette.controlRaised
                        fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
                        borderWidth: 0
                        borderColor: "transparent"
                        toolTipText: qsTr("Open TGC settings")
                        onClicked: if (root.store) root.store.openTgcSettings()
                    }
                    KCombo {
                        id: mosaicSource
                        Layout.preferredWidth: mosaicGroup.ctrlW
                        model: [qsTr("Raw"), qsTr("Side-Scan"), qsTr("TGC")]
                        currentIndex: 1
                        onCurrentIndexChanged: core.setMosaicSource(currentIndex)
                        Component.onCompleted: core.setMosaicSource(currentIndex)
                        Settings { category: "scene3d/mosaic"; property alias mosaicSource: mosaicSource.currentIndex }
                    }
                }
            }
        }

        Rectangle {
            id: fakeCoordsGroup
            visible: core.posZeroing
            width: parent.width
            height: fakeCoordsGroupContent.implicitHeight + 2 * Tokens.spaceLg
            color: "transparent"
            radius: Tokens.radiusLg
            border.width: Math.max(1, Math.round(1 * AppPalette.scale))
            border.color: AppPalette.borderHover

            ColumnLayout {
                id: fakeCoordsGroupContent
                anchors.fill: parent
                anchors.margins: Tokens.spaceLg
                spacing: Tokens.spaceMd

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceSm

                    Item {
                        Layout.preferredWidth: Tokens.iconMd
                        Layout.preferredHeight: Tokens.iconMd

                        Image {
                            id: fakeCoordsIcon
                            anchors.fill: parent
                            source: "qrc:/icons/ui/route_crossed_out.svg"
                            sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
                            sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            visible: false
                        }
                        ColorOverlay {
                            anchors.fill: fakeCoordsIcon
                            source: fakeCoordsIcon
                            color: AppPalette.textStrong
                            smooth: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Dataset: with position zeroing on")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: fakeCoordsClearOldDataCheck.switchHorizontalPadding
                    Layout.rightMargin: fakeCoordsClearOldDataCheck.switchHorizontalPadding
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.maximumWidth: mosaicGroup.labelW
                        text: qsTr("Calc last N epochs:")
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        elide: Text.ElideRight
                    }
                    KSlider {
                        id: fakeCoordsLastNSlider
                        Layout.fillWidth: true
                        from: 10; to: 3000; stepSize: 10; value: 500
                        showValueTip: false
                        readonly property int effectiveN: (core.posZeroing && value < to) ? value : 0
                        onEffectiveNChanged: core.setMosaicFakeCoordsLastN(effectiveN)
                        Component.onCompleted: core.setMosaicFakeCoordsLastN(effectiveN)
                        Settings { category: "main/dataset"; property alias fakeCoordsLastNSlider: fakeCoordsLastNSlider.value }
                    }
                    Text {
                        id: fakeCoordsValue
                        Layout.preferredWidth: Math.round(44 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        text: fakeCoordsLastNSlider.value >= fakeCoordsLastNSlider.to
                              ? qsTr("All") : fakeCoordsLastNSlider.value
                    }
                }

                KSwitch {
                    id: fakeCoordsClearOldDataCheck
                    text: qsTr("Clear old data (*)")
                    checked: true
                    Layout.fillWidth: true
                    readonly property bool effectiveClearOldData: checked && core.posZeroing
                    onEffectiveClearOldDataChanged: core.setMosaicFakeCoordsClearOldData(effectiveClearOldData)
                    Component.onCompleted: core.setMosaicFakeCoordsClearOldData(effectiveClearOldData)
                    Settings { category: "main/dataset"; property alias fakeCoordsClearOldDataCheck: fakeCoordsClearOldDataCheck.checked }
                }
            }
        }
    }


    // ── 3D scene (map provider) ──────────────────────────────────────────────

    SettingsGroup {
        id: videoGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Video")
        description: videoGroup.surfaces.length > 0 ? qsTr("Per-window video settings.") : ""
        stateStore: root.store
        stateKey: "app.video"
        collapsedByDefault: true

        readonly property var surfaces: root.store ? root.store.visibleVideoSurfaces : []

        Column {
            id: videoWindowList
            width: parent.width
            spacing: Tokens.spaceMd

            HoverHandler {
                onHoveredChanged: if (!hovered && root.store) root.store.highlightedLeafId = -1
            }

            Text {
                width: parent.width
                visible: videoGroup.surfaces.length === 0
                text: qsTr("No video windows displayed. You can add one while editing the workspace.")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
            }

            KIsland {
                visible: videoGroup.surfaces.length > 0

                Repeater {
                    model: videoGroup.surfaces

                    delegate: KIslandRow {
                        required property var modelData

                        label: modelData.label
                        labelColor: root._bright
                        chevron: true
                        interactive: true
                        onHoveredChanged: if (hovered && root.store && modelData.leafId >= 0)
                                              root.store.highlightedLeafId = modelData.leafId
                        onClicked: root.store.openVideoPaneSettings(modelData.contentId)
                    }
                }
            }
        }
    }

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Echograms")
        description: qsTr("Per-echogram display settings.")
        stateStore: root.store
        stateKey: "app.echograms"
        collapsedByDefault: true

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            HoverHandler {
                onHoveredChanged: if (!hovered) root.store.highlightedLeafId = -1
            }

            Text {
                width: parent.width
                visible: root.echograms.length === 0
                text: qsTr("No echograms displayed")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontMd
                wrapMode: Text.WordWrap
            }

            KIsland {
                visible: root.echograms.length > 0

                Repeater {
                    model: root.echograms

                    delegate: KIslandRow {
                        required property var modelData

                        label: modelData.label
                        labelColor: root._bright
                        chevron: true
                        interactive: true
                        onHoveredChanged: if (hovered && !root.store.echogramSettingsActive)
                                              root.store.highlightedLeafId = modelData.key
                        onClicked: root.store.openEchogramSettings(modelData.plot, modelData.label, modelData.key)
                    }
                }
            }
        }

        KIsland {
            KIslandRow {
                label: qsTr("Loupe")
                labelColor: root._bright
                interactive: true
                onClicked: echogramLoupeSwitch.click()

                KSwitch {
                    id: echogramLoupeSwitch
                    flat: true
                    checked: root.store ? root.store.echogramLoupeVisible : false
                    onToggled: if (root.store) root.store.echogramLoupeVisible = checked
                }
            }

            KIslandRow {
                label: qsTr("Size")
                labelColor: root._bright
                verticalPadding: Tokens.spaceSm
                showSeparator: false
                open: root.store ? root.store.echogramLoupeVisible : false

                KSpinBox {
                    width: Math.round(120 * AppPalette.scale)
                    height: Tokens.controlHMd
                    from: 1; to: 3; stepSize: 1
                    value: root.store ? root.store.echogramLoupeSize : 1
                    onValueModified: function(val) { if (root.store) root.store.echogramLoupeSize = val }
                }
            }

            KIslandRow {
                id: echogramLoupeZoomRow
                label: qsTr("Zoom")
                labelColor: root._bright
                verticalPadding: Tokens.spaceSm
                showSeparator: false
                open: root.store ? root.store.echogramLoupeVisible : false
                slotWidth: Math.round(240 * AppPalette.scale)

                Item {
                    width: echogramLoupeZoomRow.slotWidth
                    height: echogramLoupeZoomLayout.implicitHeight

                    RowLayout {
                        id: echogramLoupeZoomLayout
                        anchors.fill: parent
                        spacing: Tokens.spaceSm

                        KSlider {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            from: 0; to: 300; stepSize: 1
                            value: root.store ? root.store.echogramLoupeZoom : 100
                            onValueModified: function(val) {
                                if (!root.store) return
                                root.store.echogramLoupeZoom = Math.round(val)
                                root.store.echogramLoupePreview("update")
                            }
                            onPressedChanged: {
                                if (root.store) root.store.echogramLoupePreview(pressed ? "begin" : "end")
                            }
                        }
                        Text {
                            text: (root.store ? Math.round(root.store.echogramLoupeZoom) : 0) + "%"
                            color: root._bright
                            font.pixelSize: Tokens.fontLg
                            Layout.preferredWidth: Math.round(52 * AppPalette.scale)
                            Layout.alignment: Qt.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            KIslandRow {
                label: qsTr("Sync echograms")
                labelColor: root._bright
                toolTipText: qsTr("Sync the cursor position across all echograms")
                interactive: true
                onClicked: echogramSyncCursorSwitch.click()

                KSwitch {
                    id: echogramSyncCursorSwitch
                    flat: true
                    checked: root.store ? root.store.echogramSyncCursor : false
                    onToggled: if (root.store) root.store.echogramSyncCursor = checked
                }
            }

            KIslandRow {
                label: qsTr("Sync view")
                labelColor: root._bright
                verticalPadding: Tokens.spaceSm
                showSeparator: false
                toolTipText: qsTr("Sync scroll and zoom across echograms")
                open: root.store ? root.store.echogramSyncCursor : false
                interactive: true
                onClicked: echogramSyncViewSwitch.click()

                KSwitch {
                    id: echogramSyncViewSwitch
                    flat: true
                    checked: root.store ? root.store.echogramSyncView : false
                    onToggled: if (root.store) root.store.echogramSyncView = checked
                }
            }

            KIslandRow {
                label: qsTr("Information panel")
                labelColor: root._bright
                toolTipText: qsTr("Configure the information panel")
                chevron: true
                interactive: true
                onClicked: if (root.store) root.store.openAimPanelSettings()
            }
        }
    }

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("3D scene")
        description: qsTr("3D scene settings, map provider switching.")
        stateStore: root.store
        stateKey: "app.scene3d"
        collapsedByDefault: true

        Column {
            id: render3dList
            width: parent.width
            spacing: Tokens.spaceMd

            readonly property int spinW: Math.round(120 * AppPalette.scale)
            readonly property bool circleGridOpen: render3dSettings.gridCheckButton && render3dSettings.gridTypeCheckButton

            Text {
                width: parent.width
                text: qsTr("Rendering") + ":"
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                topPadding: Tokens.spaceXs
            }

            KIsland {
                KIslandRow {
                    label: qsTr("Show surface quality")
                    labelColor: root._bright
                    toolTipText: qsTr("Show the surface quality label in the 3D scene")
                    interactive: true
                    onClicked: showSurfaceQualitySwitch.click()

                    KSwitch {
                        id: showSurfaceQualitySwitch
                        flat: true
                        checked: root.store ? root.store.showSurfaceQuality : false
                        onToggled: if (root.store) root.store.showSurfaceQuality = checked
                    }
                }

                KIslandRow {
                    visible: core.needForceZooming
                    label: qsTr("Force zoom")
                    labelColor: root._bright
                    interactive: true
                    onClicked: forceSingleZoomSwitch.click()

                    KSwitch {
                        id: forceSingleZoomSwitch
                        flat: true
                        checked: render3dSettings.forceSingleZoomCheckButton
                        onToggled: {
                            render3dSettings.forceSingleZoomCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Loupe")
                    labelColor: root._bright
                    interactive: true
                    onClicked: syncLoupeSwitch.click()

                    KSwitch {
                        id: syncLoupeSwitch
                        flat: true
                        checked: render3dSettings.syncLoupeCheckButton
                        onToggled: {
                            render3dSettings.syncLoupeCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onSyncLoupeVisibleChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.syncLoupeCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Size")
                    labelColor: root._bright

                    KSpinBox {
                        id: syncLoupeSizeSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 3; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onSyncLoupeSizeChanged(v)
                        }
                    }
                }

                KIslandRow {
                    id: syncLoupeZoomRow
                    open: render3dSettings.syncLoupeCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Zoom, %:")
                    labelColor: root._bright
                    slotWidth: Math.round(240 * AppPalette.scale)

                    Item {
                        width: syncLoupeZoomRow.slotWidth
                        height: syncLoupeZoomLayout.implicitHeight

                        RowLayout {
                            id: syncLoupeZoomLayout
                            anchors.fill: parent
                            spacing: Tokens.spaceSm

                            KSlider {
                                id: syncLoupeZoomSlider
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                from: 0; to: 300; stepSize: 1; value: 100
                                onValueModified: function(val) {
                                    if (typeof Scene3dToolBarController !== "undefined") {
                                        Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(val))
                                        Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(true)
                                    }
                                }
                                onPressedChanged: {
                                    if (typeof Scene3dToolBarController !== "undefined")
                                        Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(pressed)
                                }
                            }
                            Text {
                                text: Math.round(syncLoupeZoomSlider.value) + "%"
                                color: root._bright
                                font.pixelSize: Tokens.fontLg
                                Layout.preferredWidth: Math.round(52 * AppPalette.scale)
                                Layout.alignment: Qt.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("North mode")
                    labelColor: root._bright
                    toolTipText: qsTr("Orient the 3D view to north (north stays up)")
                    interactive: true
                    onClicked: northViewSwitch.click()

                    KSwitch {
                        id: northViewSwitch
                        flat: true
                        checked: render3dSettings.isNorthViewButton
                        onToggled: {
                            render3dSettings.isNorthViewButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Sync echogram")
                    labelColor: root._bright
                    toolTipText: qsTr("Sync the cursor between the 2D echogram and the 3D scene")
                    interactive: true
                    onClicked: selectionToolSwitch.click()

                    KSwitch {
                        id: selectionToolSwitch
                        flat: true
                        checked: render3dSettings.selectionToolButton
                        onToggled: {
                            render3dSettings.selectionToolButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Grid")
                    labelColor: root._bright
                    interactive: true
                    onClicked: grid3dSwitch.click()

                    KSwitch {
                        id: grid3dSwitch
                        flat: true
                        checked: render3dSettings.gridCheckButton
                        onToggled: {
                            render3dSettings.gridCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onGridVisibilityCheckedChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.gridCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Circle")
                    labelColor: root._bright
                    interactive: true
                    onClicked: gridTypeSwitch.click()

                    KSwitch {
                        id: gridTypeSwitch
                        flat: true
                        checked: render3dSettings.gridTypeCheckButton
                        onToggled: {
                            render3dSettings.gridTypeCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dList.circleGridOpen
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Labels")
                    labelColor: root._bright
                    interactive: true
                    onClicked: gridLabelsSwitch.click()

                    KSwitch {
                        id: gridLabelsSwitch
                        flat: true
                        checked: render3dSettings.gridLabelsCheckButton
                        onToggled: {
                            render3dSettings.gridLabelsCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dList.circleGridOpen
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Size")
                    labelColor: root._bright

                    KSpinBox {
                        id: circleGridSizeSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 3; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(v)
                        }
                    }
                }

                KIslandRow {
                    open: render3dList.circleGridOpen
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Step")
                    labelColor: root._bright

                    KSpinBox {
                        id: circleGridStepSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 20; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(v)
                        }
                    }
                }

                KIslandRow {
                    open: render3dList.circleGridOpen
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Angle")
                    labelColor: root._bright

                    KSpinBox {
                        id: circleGridAngleSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 5; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(v)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Shadows")
                    labelColor: root._bright
                    interactive: true
                    onClicked: shadowEnabledSwitch.click()

                    KSwitch {
                        id: shadowEnabledSwitch
                        flat: true
                        checked: render3dSettings.shadowEnabledCheckButton
                        onToggled: {
                            render3dSettings.shadowEnabledCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onShadowsEnabledChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Navigation arrow")
                    labelColor: root._bright
                    interactive: true
                    onClicked: navigationArrowSwitch.click()

                    KSwitch {
                        id: navigationArrowSwitch
                        flat: true
                        checked: render3dSettings.navigationArrowCheckButton
                        onToggled: {
                            render3dSettings.navigationArrowCheckButton = checked
                            if (typeof NavigationArrowControlMenuController !== "undefined")
                                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.navigationArrowCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Shape")
                    labelColor: root._bright

                    KCombo {
                        id: navigationArrowShapeCombo
                        width: render3dList.spinW
                        model: [qsTr("Arrow"), qsTr("Boat")]
                        currentIndex: 0
                        onActivated: function(idx) {
                            if (typeof NavigationArrowControlMenuController !== "undefined")
                                NavigationArrowControlMenuController.onRepresentationChanged(idx)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.navigationArrowCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Size")
                    labelColor: root._bright

                    KSpinBox {
                        id: navigationArrowSizeSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 5; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof NavigationArrowControlMenuController !== "undefined")
                                NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(v)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Compass")
                    labelColor: root._bright
                    interactive: true
                    onClicked: compassSwitch.click()

                    KSwitch {
                        id: compassSwitch
                        flat: true
                        checked: render3dSettings.compassCheckButton
                        onToggled: {
                            render3dSettings.compassCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onCompassButtonChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.compassCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Position")
                    labelColor: root._bright

                    KSpinBox {
                        id: compassPosSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 3; stepSize: 1; value: 2
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onCompassPosChanged(v)
                        }
                    }
                }

                KIslandRow {
                    open: render3dSettings.compassCheckButton
                    showSeparator: false
                    verticalPadding: Tokens.spaceSm
                    label: qsTr("Size")
                    labelColor: root._bright

                    KSpinBox {
                        id: compassSizeSpinBox
                        width: render3dList.spinW
                        height: Tokens.controlHMd
                        from: 1; to: 5; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onCompassSizeChanged(v)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Scale bar")
                    labelColor: root._bright
                    toolTipText: qsTr("Show the scale bar in the 3D scene")
                    interactive: true
                    onClicked: scaleBarSwitch.click()

                    KSwitch {
                        id: scaleBarSwitch
                        flat: true
                        checked: render3dSettings.scaleBarCheckButton
                        onToggled: {
                            render3dSettings.scaleBarCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onScaleBarButtonChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("USBL beacons")
                    labelColor: root._bright
                    toolTipText: qsTr("Show the acoustic nodes and their tracks in the 3D scene")
                    interactive: true
                    onClicked: usblLayerSwitch.click()

                    KSwitch {
                        id: usblLayerSwitch
                        flat: true
                        checked: render3dSettings.usblLayerCheckButton
                        onToggled: {
                            render3dSettings.usblLayerCheckButton = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onUsblLayerVisibilityChanged(checked)
                        }
                    }
                }
            }

            Settings {
                id: render3dSettings
                category: "scene3d/view"
                property bool usblLayerCheckButton: true
                property bool forceSingleZoomCheckButton: false
                property bool syncLoupeCheckButton: false
                property bool isNorthViewButton: false
                property bool selectionToolButton: true
                property bool gridCheckButton: false
                property bool gridTypeCheckButton: false
                property bool gridLabelsCheckButton: true
                property bool shadowEnabledCheckButton: true
                property bool navigationArrowCheckButton: true
                property bool compassCheckButton: true
                property bool scaleBarCheckButton: true
            }
            Settings { category: "scene2d/echogramLoupe"; property alias syncLoupeSize:        syncLoupeSizeSpinBox.value }
            Settings { category: "scene2d/echogramLoupe"; property alias syncLoupeZoom:        syncLoupeZoomSlider.value }
            Settings { category: "scene3d/grid";            property alias circleGridSize:       circleGridSizeSpinBox.value }
            Settings { category: "scene3d/grid";            property alias circleGridStep:       circleGridStepSpinBox.value }
            Settings { category: "scene3d/grid";            property alias circleGridAngle:      circleGridAngleSpinBox.value }
            Settings { category: "scene3d/navigationArrow"; property alias navigationArrowSize:  navigationArrowSizeSpinBox.value }
            Settings { category: "scene3d/navigationArrow"; property alias navigationArrowShape: navigationArrowShapeCombo.currentIndex }
            Settings { category: "scene3d/compass";         property alias compassPos:           compassPosSpinBox.value }
            Settings { category: "scene3d/compass";         property alias compassSize:          compassSizeSpinBox.value }

            Text {
                width: parent.width
                text: qsTr("Map") + ":"
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                topPadding: Tokens.spaceXs
            }

            KIsland {
                KIslandRow {
                    label: qsTr("Show map tiles")
                    labelColor: root._bright
                    interactive: true
                    onClicked: mapTilesSwitch.click()

                    KSwitch {
                        id: mapTilesSwitch
                        flat: true
                        checked: mapVisibilitySettings.mapViewCheckButton
                        onToggled: {
                            mapVisibilitySettings.mapViewCheckButton = checked
                            if (typeof MapViewControlMenuController !== "undefined")
                                MapViewControlMenuController.onVisibilityChanged(checked)
                            core.setMapTileLoadingEnabled(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Limit downloads on metered networks")
                    labelColor: root._bright
                    interactive: true
                    onClicked: meteredLimitSwitch.click()

                    KSwitch {
                        id: meteredLimitSwitch
                        flat: true
                        checked: meteredSettings.deferTilesOnMetered
                        onToggled: {
                            meteredSettings.deferTilesOnMetered = checked
                            core.setDeferTilesOnMetered(checked)
                        }
                    }
                }
            }

            // Persisted under the legacy QSettings key so existing user
            // preferences carry over from the old in-3D-toolbar UI.
            Settings {
                id: mapVisibilitySettings
                category: "scene3d/map"
                property bool mapViewCheckButton: true
            }

            Settings { id: meteredSettings; category: "scene3d/map"; property bool deferTilesOnMetered: true }

            Row {
                width: parent.width
                spacing: Tokens.spaceMd
                height: Tokens.controlHSm
                visible: core.metered

                Rectangle {
                    width: Math.round(10 * AppPalette.scale)
                    height: width
                    radius: width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: AppPalette.linkIdleBorder
                    border.width: 1
                    border.color: AppPalette.border
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Metered network (limited)")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontMd
                }
            }

            // ── Internet status row ──────────────────────────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd
                height: Tokens.controlHSm

                Rectangle {
                    width: Math.round(10 * AppPalette.scale)
                    height: width
                    radius: width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: core.internetAvailable ? "#35c759" : "#ff3b30"
                    border.width: 1
                    border.color: AppPalette.border
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: core.internetAvailable ? qsTr("Internet available")
                                                 : qsTr("Internet unavailable")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontMd
                }
            }

            Text {
                width: parent.width
                text: qsTr("Providers") + ":"
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                topPadding: Tokens.spaceXs
            }

            // ── Provider selector (single-pick rows) ─────────────────────
            KIsland {
                Repeater {
                    model: core.mapTileProviders

                    delegate: KIslandRow {
                        id: providerRow
                        required property var modelData

                        readonly property bool isSelected: modelData.id === core.mapTileProviderId
                        // Cached once per delegate (providers list is CONSTANT).
                        // Refreshed on click — see below.
                        property var dbInfo: core.getMapTileDbInfo(modelData.id)

                        label: modelData.name
                        labelPixelSize: Tokens.fontMd
                        labelColor: isSelected ? AppPalette.accentText : AppPalette.textStrong
                        caption: {
                            if (!providerRow.dbInfo || !providerRow.dbInfo.exists)
                                return qsTr("Cache: empty")
                            var mb = (providerRow.dbInfo.sizeBytes / (1024 * 1024)).toFixed(1)
                            var iso = providerRow.dbInfo.created || providerRow.dbInfo.modified || ""
                            var d = new Date(iso)
                            var dateStr = isNaN(d.getTime())
                                          ? iso
                                          : d.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
                            return qsTr("Cache since %1  •  %2 MB").arg(dateStr).arg(mb)
                        }
                        captionColor: isSelected
                                      ? Qt.rgba(AppPalette.accentText.r, AppPalette.accentText.g, AppPalette.accentText.b, 0.82)
                                      : AppPalette.textMuted
                        fillColor: isSelected ? AppPalette.accentBg : "transparent"
                        hoverColor: isSelected ? Qt.lighter(AppPalette.accentBg, 1.12) : AppPalette.cardHover
                        pressedColor: isSelected ? Qt.darker(AppPalette.accentBg, 1.08) : AppPalette.bgHover
                        labelHoverColor: isSelected ? AppPalette.accentText : Qt.lighter(AppPalette.textStrong, Anim.hoverLighten)
                        verticalPadding: Tokens.spaceSm
                        interactive: true
                        onClicked: {
                            core.setMapTileProvider(modelData.id)
                            providerRow.dbInfo = core.getMapTileDbInfo(modelData.id)
                        }

                        Text {
                            text: providerRow.modelData.layer_type
                            color: providerRow.isSelected
                                   ? Qt.rgba(AppPalette.accentText.r, AppPalette.accentText.g, AppPalette.accentText.b, 0.82)
                                   : AppPalette.textSecond
                            font.pixelSize: Tokens.fontSm
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: qsTr("Navigator") + ":"
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                topPadding: Tokens.spaceXs
            }

            KIsland {
                KIslandRow {
                    label: qsTr("Use angle")
                    labelColor: root._bright
                    interactive: true
                    onClicked: useAngleSwitch.click()

                    KSwitch {
                        id: useAngleSwitch
                        flat: true
                        checked: root.store.useAngleEnabled
                        onToggled: {
                            root.store.useAngleEnabled = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onUseAngleLocationButtonChanged(checked)
                        }
                    }
                }

                KIslandRow {
                    label: qsTr("Navigator view")
                    labelColor: root._bright
                    interactive: true
                    onClicked: navigatorViewSwitch.click()

                    KSwitch {
                        id: navigatorViewSwitch
                        flat: true
                        checked: root.store.navigationViewEnabled
                        onToggled: {
                            root.store.navigationViewEnabled = checked
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onNavigatorLocationButtonChanged(checked)
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: qsTr("Data") + ":"
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                topPadding: Tokens.spaceXs
            }

            Item {
                id: resetSurfaceRow
                width: parent.width
                implicitHeight: Tokens.controlHMd
                height: implicitHeight
                property bool confirming: false

                KButton {
                    anchors.fill: parent
                    visible: !resetSurfaceRow.confirming
                    text: qsTr("Reset surface")
                    onClicked: {
                        resetSurfaceRow.confirming = true
                        resetSurfaceConfirmTimer.restart()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    visible: resetSurfaceRow.confirming
                    spacing: Tokens.spaceMd

                    KButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        danger: true
                        text: qsTr("Clear?")
                        onClicked: {
                            resetSurfaceConfirmTimer.stop()
                            resetSurfaceRow.confirming = false
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onResetProcessingButtonClicked()
                        }
                    }
                    KButton {
                        Layout.fillHeight: true
                        Layout.preferredWidth: Math.round(110 * AppPalette.scale)
                        text: qsTr("Cancel")
                        onClicked: {
                            resetSurfaceConfirmTimer.stop()
                            resetSurfaceRow.confirming = false
                        }
                    }
                }

                Timer {
                    id: resetSurfaceConfirmTimer
                    interval: 3000
                    onTriggered: resetSurfaceRow.confirming = false
                }
            }

            Component.onCompleted: {
                if (typeof MapViewControlMenuController !== "undefined")
                    MapViewControlMenuController.onVisibilityChanged(mapVisibilitySettings.mapViewCheckButton)
                core.setMapTileLoadingEnabled(mapVisibilitySettings.mapViewCheckButton)
                core.setDeferTilesOnMetered(meteredSettings.deferTilesOnMetered)
                if (typeof Scene3dToolBarController !== "undefined") {
                    Scene3dToolBarController.onUseAngleLocationButtonChanged(root.store.useAngleEnabled)
                    Scene3dToolBarController.onNavigatorLocationButtonChanged(root.store.navigationViewEnabled)
                    Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(root.store.trackLastDataEnabled)
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(render3dSettings.forceSingleZoomCheckButton)
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(render3dSettings.syncLoupeCheckButton)
                    Scene3dToolBarController.onSyncLoupeSizeChanged(syncLoupeSizeSpinBox.value)
                    Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(syncLoupeZoomSlider.value))
                    Scene3dToolBarController.onIsNorthLocationButtonChanged(render3dSettings.isNorthViewButton)
                    Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(render3dSettings.selectionToolButton)
                    Scene3dToolBarController.onGridVisibilityCheckedChanged(render3dSettings.gridCheckButton)
                    Scene3dToolBarController.onPlaneGridTypeChanged(!render3dSettings.gridTypeCheckButton)
                    Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(render3dSettings.gridLabelsCheckButton)
                    Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(circleGridSizeSpinBox.value)
                    Scene3dToolBarController.onPlaneGridCircleGridStepChanged(circleGridStepSpinBox.value)
                    Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(circleGridAngleSpinBox.value)
                    Scene3dToolBarController.onShadowsEnabledChanged(render3dSettings.shadowEnabledCheckButton)
                    Scene3dToolBarController.onCompassButtonChanged(render3dSettings.compassCheckButton)
                    Scene3dToolBarController.onCompassPosChanged(compassPosSpinBox.value)
                    Scene3dToolBarController.onCompassSizeChanged(compassSizeSpinBox.value)
                    Scene3dToolBarController.onScaleBarButtonChanged(render3dSettings.scaleBarCheckButton)
                    Scene3dToolBarController.onUsblLayerVisibilityChanged(render3dSettings.usblLayerCheckButton)
                }
                if (typeof NavigationArrowControlMenuController !== "undefined") {
                    NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(render3dSettings.navigationArrowCheckButton)
                    NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(navigationArrowSizeSpinBox.value)
                    NavigationArrowControlMenuController.onRepresentationChanged(navigationArrowShapeCombo.currentIndex)
                }
            }
        }
    }

    // ── Test (developer-only — compiled with MANUAL_TESTING) ─────────────────

    SettingsGroup {
        visible: typeof manualTesting !== "undefined" && manualTesting === true
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Test")
        description: qsTr("Developer knobs — visible only in MANUAL_TESTING builds.")
        stateStore: root.store
        stateKey: "app.test"
        collapsedByDefault: false

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Double-tap tolerance, px:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.doubleTapDistancePx) + " px"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: tapTolerSlider
                width: parent.width
                from: 1; to: 500; stepSize: 1
                value: AppPalette.doubleTapDistancePx
                onValueModified: function(v) { AppPalette.doubleTapDistancePx = v }
            }

            // Persists the chosen value across launches.
            Settings { category: "main/ui"; property alias appDoubleTapDistancePx: tapTolerSlider.value }

            // ── Pane split grab thickness ────────────────────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Split grab thickness, px:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.splitHitSizePx) + " px"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: splitHitSlider
                width: parent.width
                from: 4; to: 200; stepSize: 1
                value: AppPalette.splitHitSizePx
                onValueModified: function(v) { AppPalette.splitHitSizePx = v }
            }

            Settings { category: "main/ui"; property alias appSplitHitSizePx: splitHitSlider.value }

            // ── Sidebar slide animation duration ─────────────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Sidebar slide, ms:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.sidebarAnimMs) + " ms"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: sidebarAnimSlider
                width: parent.width
                from: 0; to: 5000; stepSize: 10
                value: AppPalette.sidebarAnimMs
                onValueModified: function(v) { AppPalette.sidebarAnimMs = v }
            }

            Settings { category: "main/ui"; property alias appSidebarAnimMs: sidebarAnimSlider.value }

            // ── Workspace rubber-band adjustment duration ────────────────
            Row {
                width: parent.width
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Workspace adjust, ms:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: Math.round(AppPalette.workspaceAnimMs) + " ms"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KSlider {
                id: workspaceAnimSlider
                width: parent.width
                from: 0; to: 5000; stepSize: 10
                value: AppPalette.workspaceAnimMs
                onValueModified: function(v) { AppPalette.workspaceAnimMs = v }
            }

            Settings { category: "main/ui"; property alias appWorkspaceAnimMs: workspaceAnimSlider.value }

            Component.onCompleted: {
                AppPalette.doubleTapDistancePx = tapTolerSlider.value
                AppPalette.splitHitSizePx = splitHitSlider.value
                AppPalette.sidebarAnimMs = sidebarAnimSlider.value
                AppPalette.workspaceAnimMs = workspaceAnimSlider.value
            }
        }
    }

    Item {
        width: parent.width
        height: footerCol.implicitHeight + Tokens.spaceXl

        AppIdentityCard {
            id: footerCol
            anchors.top: parent.top
            anchors.topMargin: Tokens.spaceMd
            anchors.horizontalCenter: parent.horizontalCenter
            store: root.store
        }

        KCircleIconButton {
            visible: !!root._flick && root._flick.contentHeight > root._flick.height + 0.5
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceLg
            anchors.verticalCenter: footerCol.verticalCenter
            width: Tokens.controlHLg
            height: width
            rounded: false
            cornerRadius: Tokens.radiusMd
            borderWidth: 0
            fillColor: AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            toolTipText: qsTr("Scroll to top")
            onClicked: root._scrollToTop()

            DisclosureIndicator {
                anchors.centerIn: parent
                width: Math.round(12 * AppPalette.scale)
                height: width
                expanded: true
                rotation: 180
                indicatorColor: AppPalette.text
            }
        }
    }
}

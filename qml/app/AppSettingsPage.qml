import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import "../controls"
import "../scene2d"

Column {
    id: root

    required property var store
    property var targetPlot: null
    property var echograms: []

    readonly property int instruments: theme ? theme.instrumentsGrade : 0
    readonly property real groupWidth: Math.max(0, width)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

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

                Behavior on x {
                    NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { sc.checked = !sc.checked; sc.toggled(sc.checked) }
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

    // ── Интерфейс ─────────────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
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
                options: [
                    { label: qsTr("English"), value: 0 },
                    { label: qsTr("Russian"), value: 1 },
                    { label: qsTr("Polish"),  value: 2 }
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

                readonly property var names: ["Dark","S.Dark","Light","S.Light","OneDark","Monokai","Kimbie","Solar","Desert","Steam 2003"]
                readonly property int gap: Tokens.spaceXs
                readonly property int cellMinW: Math.round(80 * AppPalette.scale)
                readonly property int cols: Tokens.gridColumns(width, cellMinW, gap, 5)
                readonly property int itemH: Tokens.controlHMd
                readonly property real itemW: (width - (cols - 1) * gap) / cols
                readonly property int rows: Math.ceil(10 / cols)
                height: rows * itemH + (rows - 1) * gap

                onSelectedIndexChanged: if (theme) theme.themeID = selectedIndex
                Component.onCompleted: if (theme) theme.themeID = selectedIndex

                Repeater {
                    model: 10
                    delegate: Rectangle {
                        required property int index
                        readonly property bool sel: index === appThemeHolder.selectedIndex
                        x: (index % appThemeHolder.cols) * (appThemeHolder.itemW + appThemeHolder.gap)
                        y: Math.floor(index / appThemeHolder.cols) * (appThemeHolder.itemH + appThemeHolder.gap)
                        width: appThemeHolder.itemW
                        height: appThemeHolder.itemH
                        radius: Tokens.radiusMd
                        color: sel ? AppPalette.accentBg : AppPalette.bg
                        border.width: 1
                        border.color: sel ? AppPalette.accentBorder : AppPalette.border

                        Text {
                            anchors.centerIn: parent
                            text: appThemeHolder.names[index]
                            color: AppPalette.text
                            font.pixelSize: Tokens.fontXs
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appThemeHolder.selectedIndex = index
                        }
                    }
                }

                Settings { property alias appTheme: appThemeHolder.selectedIndex }
            }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Feature level:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            Item {
                id: instrumentsGradeHolder
                width: parent.width
                height: gradeTabBar.implicitHeight
                property int selectedIndex: 0

                onSelectedIndexChanged: if (theme) theme.instrumentsGrade = selectedIndex
                Component.onCompleted: if (theme) theme.instrumentsGrade = selectedIndex

                KTabBar {
                    id: gradeTabBar
                    width: parent.width
                    options: [
                        { label: qsTr("Fish Finders"),  value: 0 },
                        { label: qsTr("Bottom Track"),  value: 1 },
                        { label: qsTr("Maximum"),       value: 2 }
                    ]
                    currentValue: instrumentsGradeHolder.selectedIndex
                    onValueSelected: function(v) { instrumentsGradeHolder.selectedIndex = v }
                }

                Settings { property alias instrumentsGradeList: instrumentsGradeHolder.selectedIndex }
            }
        }

        // UI scale — DPI auto-detect + user override. Persisted via theme.manualScale.
        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Row {
                width: parent.width
                spacing: Tokens.spaceMd
                Text {
                    text: qsTr("UI scale:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: theme ? Math.round(theme.manualScale * 100) + "%" : "100%"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            KTabBar {
                id: uiScaleTabBar
                width: parent.width
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

        // ── Merged from former "Interface" group ──────────────────────────

        KSwitch {
            id: consoleVisible
            visible: instruments >= 2
            width: parent.width
            text: qsTr("Console")
            checked: theme ? theme.consoleVisible : false
            onToggled: if (theme) theme.consoleVisible = checked

            Connections {
                target: theme
                ignoreUnknownSignals: true
                function onInterfaceChanged() {
                    if (consoleVisible.checked !== theme.consoleVisible)
                        consoleVisible.checked = theme.consoleVisible
                }
            }
        }

        Settings { property alias consoleVisible: consoleVisible.checked }

        KSwitch {
            width: parent.width
            text: qsTr("Hide controls without data")
            checked: root.store ? root.store.hideEmptyEchogramControls : true
            onToggled: if (root.store) root.store.hideEmptyEchogramControls = checked
        }

        KSwitch {
            width: parent.width
            text: qsTr("Autopilot panel")
            checked: root.store ? root.store.autopilotEnabled : true
            onToggled: if (root.store) root.store.autopilotEnabled = checked
        }

        KSwitch {
            width: parent.width
            text: qsTr("Workspace shift")
            checked: root.store.settingsPushContent
            onToggled: { root.store.settingsPushContent = checked }
        }

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Sidebar position:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

            KTabBar {
                width: parent.width
                options: [
                    { label: qsTr("Left"),  value: "left"  },
                    { label: qsTr("Right"), value: "right" }
                ]
                currentValue: root.store.settingsSide
                onValueSelected: function(value) { root.store.settingsSide = value }
            }
        }

        KButton {
            visible: Qt.platform.os !== "android"
            width: parent.width
            text: qsTr("Hotkeys")
            onClicked: { hotkeysLoader.active = true; hotkeysLoader.item.open() }
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
        title: qsTr("Workspace Layout")
        description: qsTr("Pane editing, favorites and ready-made layout presets.")
        stateStore: root.store
        stateKey: "app.layoutPlacement"

        KSwitch {
            width: parent.width
            text: qsTr("Edit")
            checked: root.store.editableMode
            onToggled: { root.store.editableMode = checked }
        }

        KSwitch {
            width: parent.width
            text: qsTr("Global pop-up")
            checked: root.store.globalPopupEnabled
            onToggled: { root.store.globalPopupEnabled = checked }
        }

        KButton {
            width: parent.width
            text: qsTr("Reset workspace")
            onClicked: root.store.resetWindowConfiguration()
        }

        Row {
            spacing: Tokens.spaceLg

            KButton {
                width: Tokens.controlHLg; height: Tokens.controlHLg
                text: root.store.currentLayoutIsFavorite ? "★" : "☆"
                checkable: true
                checked: root.store.currentLayoutIsFavorite
                fontPixelSize: Tokens.fontXl
                onClicked: root.store.toggleCurrentLayoutFavorite()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.store.currentLayoutIsFavorite ? qsTr("Current layout is in favorites") : qsTr("Add current layout to favorites")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase
            }
        }

        Text { text: qsTr("Favorite layouts"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

        Text {
            visible: root.store.favoriteLayouts.length === 0
            text: qsTr("No favorite layouts yet")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
        }

        Repeater {
            model: root.store.favoriteLayouts.length
            delegate: Item {
                id: favoriteCard
                required property int index
                readonly property int favoriteIndex: index
                readonly property var favoriteEntry: (favoriteIndex >= 0 && favoriteIndex < root.store.favoriteLayouts.length) ? root.store.favoriteLayouts[favoriteIndex] : null
                readonly property var snapshot: favoriteEntry && favoriteEntry.layout ? favoriteEntry.layout : favoriteEntry
                readonly property var popupLinks: favoriteEntry && favoriteEntry.popupLinks ? favoriteEntry.popupLinks : []
                readonly property bool selected: root.store.favoriteLayoutIsCurrent(favoriteIndex)
                width: parent.width; height: favoriteCardView.implicitHeight

                FavoriteLayoutCard {
                    id: favoriteCardView
                    anchors.fill: parent
                    snapshot: favoriteCard.snapshot; popupLinks: favoriteCard.popupLinks
                    favoriteIndex: favoriteCard.favoriteIndex; selected: favoriteCard.selected; showText: true
                    onClicked: root.store.applyFavoriteLayout(favoriteCard.favoriteIndex)
                }

                KCircleIconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: Tokens.spaceSm
                    width: Tokens.iconLg; height: Tokens.iconLg; iconSource: ""; glyph: "×"
                    glyphPixelSize: Tokens.iconSm; glyphColor: AppPalette.textSecond; fillColor: AppPalette.card
                    fillHoverColor: AppPalette.cardHover; fillPressedColor: AppPalette.bgDeep
                    borderColor: AppPalette.border; borderHoverColor: AppPalette.borderHover; showGlyphWithIcon: true
                    toolTipText: qsTr("Remove favorite"); z: 6
                    onClicked: root.store.removeFavoriteLayoutAt(favoriteIndex)
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: AppPalette.border }

        Text { text: qsTr("Layout presets"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

        Repeater {
            model: [
                { presetId: 1, title: qsTr("Preset 1"), subtitle: qsTr("2 top panes, 1 bottom pane") },
                { presetId: 2, title: qsTr("Preset 2"), subtitle: qsTr("2 × 2 grid") },
                { presetId: 3, title: qsTr("Preset 3"), subtitle: qsTr("1 top pane, 2 bottom panes") }
            ]
            delegate: Rectangle {
                required property var modelData
                readonly property var preset: modelData
                readonly property bool hovered: cardMouse.containsMouse
                width: parent.width; height: Math.round(88 * AppPalette.scale); radius: Tokens.radiusLg
                color: hovered ? AppPalette.bg : AppPalette.card; border.width: 1
                border.color: hovered ? AppPalette.borderHover : AppPalette.border

                Row {
                    anchors.fill: parent; anchors.margins: Tokens.spaceMd; spacing: Tokens.spaceLg
                    Rectangle {
                        width: Math.round(84 * AppPalette.scale); height: Math.round(64 * AppPalette.scale); radius: Tokens.radiusMd; color: AppPalette.bgDeep
                        border.width: 1; border.color: AppPalette.border
                        Item {
                            id: previewArea
                            anchors.fill: parent; anchors.margins: Tokens.spaceXs
                            readonly property real gap: 4
                            Repeater {
                                model: {
                                    var iw = previewArea.width, ih = previewArea.height, g = previewArea.gap
                                    if (iw <= 0 || ih <= 0) return []
                                    if (preset.presetId === 1) {
                                        var tH = ih*.5-g/2, bH = ih-tH-g, lW = iw*.5-g/2, rW = iw-lW-g
                                        return [ {x:0,y:0,w:lW,h:tH}, {x:lW+g,y:0,w:rW,h:tH}, {x:0,y:tH+g,w:iw,h:bH} ]
                                    } else if (preset.presetId === 2) {
                                        var lW2 = iw*.5-g/2, rW2 = iw-lW2-g, tH2 = ih*.5-g/2, bH2 = ih-tH2-g
                                        return [ {x:0,y:0,w:lW2,h:tH2}, {x:lW2+g,y:0,w:rW2,h:tH2}, {x:0,y:tH2+g,w:lW2,h:bH2}, {x:lW2+g,y:tH2+g,w:rW2,h:bH2} ]
                                    }
                                    var tH3 = ih*.5-g/2, bH3 = ih-tH3-g, lW3 = iw*.5-g/2, rW3 = iw-lW3-g
                                    return [ {x:0,y:0,w:iw,h:tH3}, {x:0,y:tH3+g,w:lW3,h:bH3}, {x:lW3+g,y:tH3+g,w:rW3,h:bH3} ]
                                }
                                delegate: Rectangle {
                                    required property var modelData
                                    x: modelData.x; y: modelData.y
                                    width: modelData.w; height: modelData.h
                                    color: "transparent"
                                    border.width: 1; border.color: "#64748B"
                                }
                            }
                        }
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, parent.width - Math.round(84 * AppPalette.scale) - Tokens.spaceLg); spacing: Tokens.spaceXs
                        Text { text: preset.title; color: AppPalette.text; font.pixelSize: Tokens.fontBase; font.bold: true }
                        Text { text: preset.subtitle; color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm }
                    }
                }
                MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.store.applyLayoutPreset(preset.presetId) }
            }
        }

        Text {
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("After applying a preset, choose 2D or 3D mode for each pane.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
        }
    }

    // ── Extra info panel ──────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Extra info panel")
        stateStore: root.store
        stateKey: "app.extraInfo"
        collapsedByDefault: true

        KSwitch {
            width: parent.width
            text: qsTr("Show extra info panel")
            checked: root.store ? root.store.extraInfoVisible : false
            onToggled: if (root.store) root.store.extraInfoVisible = checked
        }

        Text {
            text: qsTr("Fields:")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
            leftPadding: Tokens.spaceXxs
        }

        KSwitch {
            width: parent.width; text: qsTr("Depth")
            checked: root.store ? root.store.extraInfoDepth : true
            onToggled: if (root.store) root.store.extraInfoDepth = checked
        }
        KSwitch {
            width: parent.width; text: qsTr("Speed")
            checked: root.store ? root.store.extraInfoSpeed : true
            onToggled: if (root.store) root.store.extraInfoSpeed = checked
        }
        KSwitch {
            width: parent.width; text: qsTr("Coordinates")
            checked: root.store ? root.store.extraInfoCoordinates : true
            onToggled: if (root.store) root.store.extraInfoCoordinates = checked
        }
        KSwitch {
            width: parent.width; text: qsTr("Active point")
            checked: root.store ? root.store.extraInfoActivePoint : true
            onToggled: if (root.store) root.store.extraInfoActivePoint = checked
        }
        KSwitch {
            width: parent.width; text: qsTr("Navigation info")
            checked: root.store ? root.store.extraInfoNav : false
            onToggled: if (root.store) root.store.extraInfoNav = checked
        }
        KSwitch {
            width: parent.width; text: qsTr("Boat status")
            checked: root.store ? root.store.extraInfoBoatStatus : false
            onToggled: if (root.store) root.store.extraInfoBoatStatus = checked
        }
    }

    // ── Датасет ───────────────────────────────────────────────────────────────

    SettingsGroup {
        visible: instruments >= 2
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Dataset")
        description: qsTr("Black-stripe smoothing and sonar mount-point offset.")
        stateStore: root.store
        stateKey: "app.dataset"
        collapsedByDefault: true

        Component.onCompleted: {
            core.setFixBlackStripesState(fixBlackStripesCheckButton.checked)
            core.setFixBlackStripesForwardSteps(fixBlackStripesForwardStepsSpinBox.value)
            core.setFixBlackStripesBackwardSteps(fixBlackStripesBackwardStepsSpinBox.value)
            core.setIsAttitudeExpected(sonarOffsetCheckButton.checked)
            core.setPosZeroing(zeroingPosButton.checked)
            core.setBottomTrackZeroing(zeroingBottomTrackButton.checked)
        }

        // FBS row
        ParamCard {
            id: fixBlackStripesCheckButton
            label: qsTr("FBS forward / backward:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs
            onToggled: function(v) { core.setFixBlackStripesState(v) }

            KSpinBox {
                id: fixBlackStripesForwardStepsSpinBox
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 5
                onValueModified: function(v) { core.setFixBlackStripesForwardSteps(v) }
            }

            KSpinBox {
                id: fixBlackStripesBackwardStepsSpinBox
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: fixBlackStripesForwardStepsSpinBox.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 5
                onValueModified: function(v) { core.setFixBlackStripesBackwardSteps(v) }
            }
        }

        Settings { property alias fixBlackStripesCheckButton: fixBlackStripesCheckButton.checked }
        Settings { property alias fixBlackStripesForwardStepsSpinBox: fixBlackStripesForwardStepsSpinBox.value }
        Settings { property alias fixBlackStripesBackwardStepsSpinBox: fixBlackStripesBackwardStepsSpinBox.value }

        // Sonar offset row
        ParamCard {
            id: sonarOffsetCheckButton
            label: qsTr("S.offset XY, mm:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs
            onToggled: function(v) {
                if (v) dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, sonarOffsetValueY.value * 0.001, 0)
                else   dataset.setSonarOffset(0, 0, 0)
                core.setIsAttitudeExpected(v)
            }

            KSpinBox {
                id: sonarOffsetValueX
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) {
                    if (sonarOffsetCheckButton.checked)
                        dataset.setSonarOffset(v * 0.001, sonarOffsetValueY.value * 0.001, 0)
                }
            }

            KSpinBox {
                id: sonarOffsetValueY
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: sonarOffsetValueX.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) {
                    if (sonarOffsetCheckButton.checked)
                        dataset.setSonarOffset(sonarOffsetValueX.value * 0.001, v * 0.001, 0)
                }
            }
        }

        Settings { property alias sonarOffsetCheckButton: sonarOffsetCheckButton.checked }
        Settings { property alias sonarOffsetValueX: sonarOffsetValueX.value }
        Settings { property alias sonarOffsetValueY: sonarOffsetValueY.value }

        ParamCard {
            id: zeroingPosButton
            label: qsTr("Pos zeroing")
            onToggled: function(v) { core.setPosZeroing(v) }
        }
        Settings { property alias zeroingPosButtonCheched: zeroingPosButton.checked }

        ParamCard {
            id: zeroingBottomTrackButton
            label: qsTr("Bottom track zeroing")
            onToggled: function(v) { core.setBottomTrackZeroing(v) }
        }
        Settings { property alias zeroingBottomTrackButtonChecked: zeroingBottomTrackButton.checked }
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
        collapsedByDefault: true

        ParamCard {
            id: boatTrackVisible3d
            label: qsTr("Show in 3D")
            checked: root.store ? root.store.boatTrackVisible : true
            onToggled: function(v) { if (root.store) root.store.boatTrackVisible = v }
        }
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
                bottomTrackThreshold.checked ? bottomTrackThresholdValue.value / 100  : 0,
                bottomTrackSensorOffset.checked ? btOffX.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffY.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffZ.value * -0.001 : 0
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
                bottomTrackSensorOffset.checked ? btOffX.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffY.value *  0.001 : 0,
                bottomTrackSensorOffset.checked ? btOffZ.value * -0.001 : 0,
                false
            )
        }

        Component.onCompleted: refreshParams()

        ParamCard {
            id: bottomTrackVisible3d
            label: qsTr("Show in 3D")
            checked: root.store ? root.store.bottomTrackVisible : false
            onToggled: function(v) { if (root.store) root.store.bottomTrackVisible = v }
        }

        // Preset
        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text { text: qsTr("Preset:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

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
                        { label: qsTr("Normal 2D"), value: 0 },
                        { label: qsTr("Narrow 2D"), value: 1 },
                        { label: qsTr("Side-Scan"), value: 2 }
                    ]
                    currentValue: btPresetHolder.selectedIndex
                    onValueSelected: function(v) { btPresetHolder.selectedIndex = v }
                }

                Settings { property alias bottomTrackList: btPresetHolder.selectedIndex }
            }
        }

        // Gain slope
        ParamCard {
            id: bottomTrackGainSlope
            label: qsTr("Gain slope:")
            checked: true
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setGainSlope(bottomTrackGainSlopeValue.value / 100) }

            KSpinBox {
                id: bottomTrackGainSlopeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 300; stepSize: 10; value: 100; divisor: 100; decimals: 2
                onValueModified: function(v) { if (bottomTrackGainSlope.checked && root.targetPlot) root.targetPlot.setGainSlope(v / 100) }
            }
        }
        Settings { property alias bottomTrackGainSlope: bottomTrackGainSlope.checked }
        Settings { property alias bottomTrackGainSlopeValue: bottomTrackGainSlopeValue.value }

        // Threshold
        ParamCard {
            id: bottomTrackThreshold
            label: qsTr("Threshold:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setThreshold(bottomTrackThresholdValue.value / 100) }

            KSpinBox {
                id: bottomTrackThresholdValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200; stepSize: 5; value: 0; divisor: 100; decimals: 2
                onValueModified: function(v) { if (bottomTrackThreshold.checked && root.targetPlot) root.targetPlot.setThreshold(v / 100) }
            }
        }
        Settings { property alias bottomTrackThreshold: bottomTrackThreshold.checked }
        Settings { property alias bottomTrackThresholdValue: bottomTrackThresholdValue.value }

        // Horizontal window
        ParamCard {
            id: bottomTrackWindow
            label: qsTr("Horizontal window:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setWindowSize(bottomTrackWindowValue.value) }

            KSpinBox {
                id: bottomTrackWindowValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 1; to: 100; stepSize: 2; value: 1
                onValueModified: function(v) { if (bottomTrackWindow.checked && root.targetPlot) root.targetPlot.setWindowSize(v) }
            }
        }
        Settings { property alias bottomTrackWindow: bottomTrackWindow.checked }
        Settings { property alias bottomTrackWindowValue: bottomTrackWindowValue.value }

        // Vertical gap
        ParamCard {
            id: bottomTrackVerticalGap
            label: qsTr("Vertical gap, %:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01) }

            KSpinBox {
                id: bottomTrackVerticalGapValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 100; stepSize: 2; value: 10
                onValueModified: function(v) { if (bottomTrackVerticalGap.checked && root.targetPlot) root.targetPlot.setVerticalGap(v * 0.01) }
            }
        }
        Settings { property alias bottomTrackVerticalGap: bottomTrackVerticalGap.checked }
        Settings { property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value }

        // Min range
        ParamCard {
            id: bottomTrackMinRange
            label: qsTr("Min range, m:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMin(bottomTrackMinRangeValue.value / 1000) }

            KSpinBox {
                id: bottomTrackMinRangeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200000; stepSize: 10; value: 0; divisor: 1000; decimals: 2
                onValueModified: function(v) { if (bottomTrackMinRange.checked && root.targetPlot) root.targetPlot.setRangeMin(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMinRange: bottomTrackMinRange.checked }
        Settings { property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value }

        // Max range
        ParamCard {
            id: bottomTrackMaxRange
            label: qsTr("Max range, m:")
            slotWidth: btGroup.spinW
            onToggled: function(v) { if (v && root.targetPlot) root.targetPlot.setRangeMax(bottomTrackMaxRangeValue.value / 1000) }

            KSpinBox {
                id: bottomTrackMaxRangeValue
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: Tokens.controlHMd
                from: 0; to: 200000; stepSize: 1000; value: 100000; divisor: 1000; decimals: 2
                onValueModified: function(v) { if (bottomTrackMaxRange.checked && root.targetPlot) root.targetPlot.setRangeMax(v / 1000) }
            }
        }
        Settings { property alias bottomTrackMaxRange: bottomTrackMaxRange.checked }
        Settings { property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value }

        // Sensor offset (label row + values row)
        ParamCard {
            id: bottomTrackSensorOffset
            label: qsTr("Sonar offset XYZ, mm:")
            onToggled: function(v) {
                if (v && root.targetPlot) {
                    root.targetPlot.setOffsetX(btOffX.value *  0.001)
                    root.targetPlot.setOffsetY(btOffY.value *  0.001)
                    root.targetPlot.setOffsetZ(btOffZ.value *  0.001)
                }
            }
        }
        Row {
            visible: bottomTrackSensorOffset.checked
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceXs
            readonly property real sw: (width - 2 * Tokens.spaceXs) / 3

            KSpinBox {
                id: btOffX
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetX(v * 0.001) }
            }
            KSpinBox {
                id: btOffY
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetY(v * 0.001) }
            }
            KSpinBox {
                id: btOffZ
                width: parent.sw; from: -9999; to: 9999; stepSize: 50; value: 0
                onValueModified: function(v) { if (bottomTrackSensorOffset.checked && root.targetPlot) root.targetPlot.setOffsetZ(v * 0.001) }
            }
        }
        Settings { property alias bottomTrackSensorOffset: bottomTrackSensorOffset.checked }
        Settings { property alias bottomTrackSensorOffsetValueX: btOffX.value }
        Settings { property alias bottomTrackSensorOffsetValueY: btOffY.value }
        Settings { property alias bottomTrackSensorOffsetValueZ: btOffZ.value }

        // Action buttons
        Row {
            width: parent.width; spacing: Tokens.spaceMd
            readonly property real bw: (width - Tokens.spaceMd) / 2

            KButton {
                width: parent.bw
                text: qsTr("Processing")
                onClicked: btGroup.doDistProcessing()
            }

            KButton {
                id: btRealtimeBtn
                width: parent.bw
                text: qsTr("Realtime")
                checkable: true
                checked: false
                onToggled: core.setBottomTrackRealtimeFromSettings(checked)
                Component.onCompleted: core.setBottomTrackRealtimeFromSettings(false)
            }
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
        collapsedByDefault: true

        readonly property int ctrlW: Math.round(200 * AppPalette.scale)
        property var exportSurfaceFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property string exportSurfacePathSource: ""

        // Hotkey API — invoked from WorkspaceStore.applyIsobathsHotkey().
        function prevTheme() {
            isobathsTheme.currentIndex = Math.max(0, isobathsTheme.currentIndex - 1)
        }
        function nextTheme() {
            isobathsTheme.currentIndex = Math.min(isobathsTheme.model.length - 1, isobathsTheme.currentIndex + 1)
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
            return isoEffectiveSource(exportSurfacePathText.text, exportSurfacePathSource)
        }

        Component.onCompleted: {
            exportSurfacePathText.text = isoDisplayUrl(exportSurfacePathSource)
        }

        ParamCard {
            id: isobathsVisible3d
            label: qsTr("Show in 3D")
            checked: root.store ? root.store.isobathsVisible : false
            onToggled: function(v) { if (root.store) root.store.isobathsVisible = v }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Theme:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KCombo {
                id: isobathsTheme
                Layout.preferredWidth: isobathsGroup.ctrlW
                model: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("Standard"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green")]
                currentIndex: 0
                onCurrentIndexChanged: IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                Component.onCompleted: IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                Settings { property alias isobathsTheme: isobathsTheme.currentIndex }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Edge limit, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsEdgeLimitSpinBox
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 10; to: 1000; stepSize: 5; value: 100
                editable: false
                onValueModified: function(v) { IsobathsViewControlMenuController.onEdgeLimitChanged(v) }
                Component.onCompleted: IsobathsViewControlMenuController.onEdgeLimitChanged(value)
                Settings { property alias isobathsEdgeLimitSpinBox: isobathsEdgeLimitSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Step, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KSpinBox {
                id: isobathsSurfaceLineStepSizeSpinBox
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 1; to: 200; stepSize: 1; value: 10
                divisor: 10; decimals: 1
                editable: false
                readonly property real realValue: value / 10
                onValueModified: function(v) { IsobathsViewControlMenuController.onSetSurfaceLineStepSize(v / 10) }
                Component.onCompleted: IsobathsViewControlMenuController.onSetSurfaceLineStepSize(realValue)
                Settings { property alias isobathsSurfaceLineStepSizeSpinBox: isobathsSurfaceLineStepSizeSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Extra width, m:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                Layout.fillWidth: true
            }
            KSpinBox {
                id: extraWidthSpinBox
                Layout.preferredWidth: isobathsGroup.ctrlW
                from: 5; to: 100; stepSize: 5; value: 10
                editable: false
                onValueModified: function(v) { IsobathsViewControlMenuController.onSetExtraWidth(v) }
                Component.onCompleted: IsobathsViewControlMenuController.onSetExtraWidth(value)
                Settings { property alias extraWidthSpinBox: extraWidthSpinBox.value }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            CTextField {
                id: exportSurfacePathText
                hoverEnabled: true
                Layout.maximumWidth: isobathsGroup.ctrlW
                Layout.fillWidth: true
                placeholderText: qsTr("Enter path")
            }

            KButton {
                text: "..."
                Layout.fillWidth: false
                implicitWidth: Math.round(40 * AppPalette.scale)
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
                Layout.fillWidth: true
                onClicked: Scene3DControlMenuController.onExportToCSVButtonClicked(isobathsGroup.currentExportSurfacePath())
            }

            Settings { property alias exportSurfaceFolder:     isobathsGroup.exportSurfaceFolder }
            Settings { property alias exportSurfaceFolderText: isobathsGroup.exportSurfacePathSource }
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
        collapsedByDefault: true

        readonly property int labelW: Math.round(140 * AppPalette.scale)
        readonly property int ctrlW:  Math.round(220 * AppPalette.scale)

        function setChannelNamesToBackend() {
            core.setMosaicChannels(channel1Combo.currentText, channel2Combo.currentText)
        }

        // Hotkey API — invoked from WorkspaceStore.applyMosaicHotkey().
        function prevTheme() {
            mosaicTheme.currentIndex = Math.max(0, mosaicTheme.currentIndex - 1)
        }
        function nextTheme() {
            mosaicTheme.currentIndex = Math.min(mosaicTheme.model.length - 1, mosaicTheme.currentIndex + 1)
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

        ParamCard {
            id: mosaicVisible3d
            label: qsTr("Show in 3D")
            checked: root.store ? root.store.mosaicVisible : false
            onToggled: function(v) { if (root.store) root.store.mosaicVisible = v }
        }

        RowLayout {
            width: parent.width
            spacing: Tokens.spaceMd

            KChartLevelCapsule {
                id: mosaicLevelsSlider
                Layout.fillHeight: true
                onStartValueChanged: MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                onStopValueChanged:  MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                Component.onCompleted: MosaicViewControlMenuController.onLevelChanged(startValue, stopValue)
                Settings {
                    property alias mosaicLevelsStart: mosaicLevelsSlider.startValue
                    property alias mosaicLevelsStop:  mosaicLevelsSlider.stopValue
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Tokens.spaceMd

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Theme:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.fillWidth: true
                    }
                    KCombo {
                        id: mosaicTheme
                        Layout.preferredWidth: mosaicGroup.ctrlW
                        model: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
                        currentIndex: 0
                        onCurrentIndexChanged: MosaicViewControlMenuController.onThemeChanged(currentIndex)
                        Component.onCompleted: MosaicViewControlMenuController.onThemeChanged(currentIndex)
                        Settings { property alias mosaicTheme: mosaicTheme.currentIndex }
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Channels:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        spacing: Tokens.spaceXs
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
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        spacing: Tokens.spaceXs
                        Layout.preferredWidth: mosaicGroup.ctrlW

                        KSpinBox {
                            id: mosaicLAngleOffset
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
                            Settings { property alias mosaicLAngleOffset: mosaicLAngleOffset.value }
                        }

                        KSpinBox {
                            id: mosaicRAngleOffset
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
                            Settings { property alias mosaicRAngleOffset: mosaicRAngleOffset.value }
                        }
                    }
                }

                KSwitch {
                    id: mosaicTraceLine
                    text: qsTr("Trace line")
                    checked: true
                    Layout.fillWidth: true
                    onToggled: MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    Component.onCompleted: MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    Settings { property alias mosaicTraceLine: mosaicTraceLine.checked }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Source:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.fillWidth: true
                    }
                    KCombo {
                        id: mosaicSource
                        Layout.preferredWidth: mosaicGroup.ctrlW
                        model: [qsTr("Raw"), qsTr("Side-Scan"), qsTr("TGC")]
                        currentIndex: 1
                        onCurrentIndexChanged: core.setMosaicSource(currentIndex)
                        Component.onCompleted: core.setMosaicSource(currentIndex)
                        Settings { property alias mosaicSource: mosaicSource.currentIndex }
                    }
                }

                Rectangle {
                    id: fakeCoordsGroup
                    visible: core.posZeroing
                    Layout.fillWidth: true
                    Layout.topMargin: Tokens.spaceMd
                    implicitHeight: fakeCoordsGroupContent.implicitHeight + 2 * Tokens.spaceLg
                    color: "transparent"
                    border.color: AppPalette.border
                    border.width: 1
                    radius: Tokens.radiusMd

                    ColumnLayout {
                        id: fakeCoordsGroupContent
                        anchors.fill: parent
                        anchors.margins: Tokens.spaceLg
                        spacing: Tokens.spaceMd

                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            flat: true
                            enabled: false
                            padding: 0
                            background: null
                            icon.source: "qrc:/icons/ui/route_crossed_out.svg"
                            icon.color: AppPalette.text
                            icon.width: Tokens.controlHMd * 1.1
                            icon.height: Tokens.controlHMd * 1.1
                            implicitWidth: Tokens.controlHMd * 1.1
                            implicitHeight: Tokens.controlHMd * 1.1
                        }

                        RowLayout {
                            spacing: Tokens.spaceMd
                            Text {
                                text: qsTr("Calc last N epochs:")
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontMd
                                Layout.fillWidth: true
                            }
                            KSlider {
                                id: fakeCoordsLastNSlider
                                Layout.preferredWidth: mosaicGroup.ctrlW - Math.round(70 * AppPalette.scale)
                                from: 10; to: 3000; stepSize: 10; value: 500
                                readonly property int effectiveN: (core.posZeroing && value < to) ? value : 0
                                onEffectiveNChanged: core.setMosaicFakeCoordsLastN(effectiveN)
                                Component.onCompleted: core.setMosaicFakeCoordsLastN(effectiveN)
                                Settings { property alias fakeCoordsLastNSlider: fakeCoordsLastNSlider.value }
                            }
                            Text {
                                Layout.preferredWidth: Math.round(50 * AppPalette.scale)
                                horizontalAlignment: Text.AlignRight
                                color: AppPalette.text
                                font.pixelSize: Tokens.fontMd
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
                            Settings { property alias fakeCoordsClearOldDataCheck: fakeCoordsClearOldDataCheck.checked }
                        }
                    }
                }
            }
        }
    }

    // ── TGC ──────────────────────────────────────────────────────────────────

    SettingsGroup {
        id: tgcGroup
        visible: instruments >= 1
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("TGC")
        description: qsTr("Time-varying gain and depth-based amplification curve.")
        stateStore: root.store
        stateKey: "app.tgc"
        collapsedByDefault: true

        readonly property int valueLabelW: Math.round(60 * AppPalette.scale)
        readonly property int labelW: Math.round(92 * AppPalette.scale)

        Component.onCompleted: {
            core.setTgcGainNear(tgcGainNearSlider.value * 0.01)
            core.setTgcGainFar(tgcGainFarSlider.value * 0.01)
            core.setTgcCompensate(tgcCompensateSwitch.checked)
        }

        // Near gain
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Text {
                text: qsTr("Near gain:")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
                width: tgcGroup.labelW
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        tgcGainNearSlider.value = 100
                        core.setTgcGainNear(1.0)
                    }
                }
            }

            KSlider {
                id: tgcGainNearSlider
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 2 * Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 500; stepSize: 1; value: 50
                valueSuffix: "%"
                onValueModified: function(v) { core.setTgcGainNear(v * 0.01) }
            }

            Text {
                width: tgcGroup.valueLabelW
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                text: tgcGainNearSlider.value + "%"
                color: AppPalette.text; font.pixelSize: Tokens.fontMd
            }
        }

        Settings { property alias appTgcGainNear: tgcGainNearSlider.value }

        // Far gain
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Text {
                text: qsTr("Far gain:")
                color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
                width: tgcGroup.labelW
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        tgcGainFarSlider.value = 100
                        core.setTgcGainFar(1.0)
                    }
                }
            }

            KSlider {
                id: tgcGainFarSlider
                width: parent.width - tgcGroup.labelW - tgcGroup.valueLabelW - 2 * Tokens.spaceMd
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 1000; stepSize: 1; value: 250
                valueSuffix: "%"
                onValueModified: function(v) { core.setTgcGainFar(v * 0.01) }
            }

            Text {
                width: tgcGroup.valueLabelW
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                text: tgcGainFarSlider.value + "%"
                color: AppPalette.text; font.pixelSize: Tokens.fontMd
            }
        }

        Settings { property alias appTgcGainFar: tgcGainFarSlider.value }

        // Curve preview
        Canvas {
            id: tgcCurveCanvas
            width: parent.width
            height: Math.round(100 * AppPalette.scale)

            Connections {
                target: tgcGainNearSlider
                function onValueChanged() { tgcCurveCanvas.requestPaint() }
            }
            Connections {
                target: tgcGainFarSlider
                function onValueChanged() { tgcCurveCanvas.requestPaint() }
            }

            onPaint: {
                var ctx = getContext("2d")
                if (!ctx)
                    return
                var w = width
                var h = height

                ctx.fillStyle = AppPalette.bg
                ctx.fillRect(0, 0, w, h)

                var gNear = tgcGainNearSlider.value / 100.0
                var gFar  = tgcGainFarSlider.value / 100.0

                var yMax = Math.max(gNear, gFar, 1.0) * 1.15
                if (yMax < 0.5) yMax = 0.5

                function yFor(g) { return h - (g / yMax) * h }

                ctx.strokeStyle = AppPalette.border
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(0, h - 0.5)
                ctx.lineTo(w, h - 0.5)
                ctx.stroke()

                var y100 = yFor(1.0)
                ctx.strokeStyle = AppPalette.text
                ctx.globalAlpha = 0.35
                if (ctx.setLineDash) ctx.setLineDash([3, 3])
                ctx.beginPath()
                ctx.moveTo(0, y100)
                ctx.lineTo(w, y100)
                ctx.stroke()
                if (ctx.setLineDash) ctx.setLineDash([])
                ctx.globalAlpha = 1.0

                ctx.strokeStyle = "#F07000"
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.moveTo(0, yFor(gNear))
                ctx.lineTo(w, yFor(gFar))
                ctx.stroke()

                ctx.fillStyle = AppPalette.text
                ctx.globalAlpha = 0.6
                ctx.font = "10px sans-serif"
                ctx.fillText("100%", 4, Math.max(y100 - 2, 10))
                ctx.globalAlpha = 1.0
            }
        }

        // Compensate
        KSwitch {
            id: tgcCompensateSwitch
            width: parent.width
            text: qsTr("Compensate")
            checked: false
            onToggled: core.setTgcCompensate(checked)
        }

        Settings { property alias appTgcCompensate: tgcCompensateSwitch.checked }
    }

    // ── 3D scene (map provider) ──────────────────────────────────────────────

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
            spacing: Tokens.spaceXs

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

            Repeater {
                model: root.echograms
                delegate: Rectangle {
                    required property var modelData
                    width: parent.width
                    height: Math.round(38 * AppPalette.scale)
                    radius: Tokens.radiusLg
                    color: navMouse.containsMouse ? AppPalette.bgHover : AppPalette.bg
                    border.width: 1
                    border.color: navMouse.containsMouse ? AppPalette.borderHover : AppPalette.border
                    Behavior on color       { ColorAnimation { duration: 110 } }
                    Behavior on border.color { ColorAnimation { duration: 110 } }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: navChevron.left
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    DisclosureIndicator {
                        id: navChevron
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(10 * AppPalette.scale)
                        height: width
                        expanded: false
                        indicatorColor: AppPalette.textSecond
                    }

                    MouseArea {
                        id: navMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onContainsMouseChanged: if (containsMouse && !root.store.echogramSettingsActive)
                                                    root.store.highlightedLeafId = modelData.key
                        onClicked: root.store.openEchogramSettings(modelData.plot, modelData.label)
                    }
                }
            }
        }
    }

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("3D scene")
        description: qsTr("Map tile provider for the 3D scene background.")
        stateStore: root.store
        stateKey: "app.scene3d"
        collapsedByDefault: true

        Column {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                width: parent.width
                text: qsTr("Rendering")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                topPadding: Tokens.spaceXs
            }

            Row {
                width: parent.width
                spacing: Tokens.spaceMd
                KButton {
                    text: qsTr("Reset depth zoom")
                    width: (parent.width - Tokens.spaceMd) / 2
                    visible: Qt.platform.os !== "android"
                    onClicked: {
                        if (typeof Scene3dToolBarController !== "undefined")
                            Scene3dToolBarController.onCancelZoomButtonClicked()
                    }
                }
                KButton {
                    text: qsTr("Reset surface")
                    width: (parent.width - Tokens.spaceMd) / 2
                    onClicked: {
                        if (typeof Scene3dToolBarController !== "undefined")
                            Scene3dToolBarController.onResetProcessingButtonClicked()
                    }
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("Show surface quality")
                checked: render3dSettings.showQualityLabelCheck
                onToggled: function(v) {
                    render3dSettings.showQualityLabelCheck = v
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("Force zoom")
                visible: core.needForceZooming
                checked: render3dSettings.forceSingleZoomCheckButton
                onToggled: function(v) {
                    render3dSettings.forceSingleZoomCheckButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onForceSingleZoomCheckedChanged(v)
                }
            }

            // Loupe — toggle + Size/Zoom row in animated card body.
            ParamCardGroup {
                id: loupeCard
                label: qsTr("Loupe")
                checked: render3dSettings.syncLoupeCheckButton
                onToggled: function(v) {
                    render3dSettings.syncLoupeCheckButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onSyncLoupeVisibleChanged(v)
                }

                Row {
                    width: parent.width
                    height: Tokens.controlHMd
                    spacing: Tokens.spaceXs
                    readonly property real sw: (width - Tokens.spaceXs) / 2

                    Row {
                        width: parent.sw
                        height: parent.height
                        spacing: Tokens.spaceSm
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Size:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        KSpinBox {
                            id: syncLoupeSizeSpinBox
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 56
                            from: 1; to: 3; stepSize: 1; value: 1
                            onValueModified: function(v) {
                                if (typeof Scene3dToolBarController !== "undefined")
                                    Scene3dToolBarController.onSyncLoupeSizeChanged(v)
                            }
                        }
                    }
                    Row {
                        width: parent.sw
                        height: parent.height
                        spacing: Tokens.spaceSm
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Zoom, %:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        KSpinBox {
                            id: syncLoupeZoomSpinBox
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 80
                            from: 0; to: 300; stepSize: 10; value: 100
                            onValueModified: function(v) {
                                if (typeof Scene3dToolBarController !== "undefined") {
                                    Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(v))
                                    Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(true)
                                }
                            }
                        }
                    }
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("North mode")
                checked: render3dSettings.isNorthViewButton
                onToggled: function(v) {
                    render3dSettings.isNorthViewButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onIsNorthLocationButtonChanged(v)
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("Sync echogram")
                checked: render3dSettings.selectionToolButton
                onToggled: function(v) {
                    render3dSettings.selectionToolButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(v)
                }
            }

            // Grid — toggle with nested Circle sub-group (Labels + Size/Step/Angle).
            ParamCardGroup {
                id: gridCard
                label: qsTr("Grid")
                checked: render3dSettings.gridCheckButton
                onToggled: function(v) {
                    render3dSettings.gridCheckButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onGridVisibilityCheckedChanged(v)
                }

                // Nested: Circle sub-group with its own animated body.
                ParamCardGroup {
                    id: gridTypeCard
                    label: qsTr("Circle")
                    checked: render3dSettings.gridTypeCheckButton
                    onToggled: function(v) {
                        render3dSettings.gridTypeCheckButton = v
                        if (typeof Scene3dToolBarController !== "undefined")
                            Scene3dToolBarController.onPlaneGridTypeChanged(!v)
                    }

                    ParamCard {
                        width: parent.width
                        label: qsTr("Labels")
                        checked: render3dSettings.gridLabelsCheckButton
                        onToggled: function(v) {
                            render3dSettings.gridLabelsCheckButton = v
                            if (typeof Scene3dToolBarController !== "undefined")
                                Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(v)
                        }
                    }
                    Row {
                        width: parent.width
                        height: Tokens.controlHMd
                        spacing: Tokens.spaceXs
                        readonly property real sw: (width - 2 * Tokens.spaceXs) / 3

                        Row {
                            width: parent.sw
                            height: parent.height
                            spacing: Tokens.spaceSm
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Size:")
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontMd
                            }
                            KSpinBox {
                                id: circleGridSizeSpinBox
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 56
                                from: 1; to: 3; stepSize: 1; value: 1
                                onValueModified: function(v) {
                                    if (typeof Scene3dToolBarController !== "undefined")
                                        Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(v)
                                }
                            }
                        }
                        Row {
                            width: parent.sw
                            height: parent.height
                            spacing: Tokens.spaceSm
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Step:")
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontMd
                            }
                            KSpinBox {
                                id: circleGridStepSpinBox
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 56
                                from: 1; to: 20; stepSize: 1; value: 1
                                onValueModified: function(v) {
                                    if (typeof Scene3dToolBarController !== "undefined")
                                        Scene3dToolBarController.onPlaneGridCircleGridStepChanged(v)
                                }
                            }
                        }
                        Row {
                            width: parent.sw
                            height: parent.height
                            spacing: Tokens.spaceSm
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Angle:")
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontMd
                            }
                            KSpinBox {
                                id: circleGridAngleSpinBox
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 64
                                from: 1; to: 5; stepSize: 1; value: 1
                                onValueModified: function(v) {
                                    if (typeof Scene3dToolBarController !== "undefined")
                                        Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(v)
                                }
                            }
                        }
                    }
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("Shadows")
                checked: render3dSettings.shadowEnabledCheckButton
                onToggled: function(v) {
                    render3dSettings.shadowEnabledCheckButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onShadowsEnabledChanged(v)
                }
            }

            // Boat (NavigationArrow) — toggle + animated Size row in same card.
            ParamCardGroup {
                id: boatCard
                label: qsTr("Boat")
                checked: render3dSettings.navigationArrowCheckButton
                onToggled: function(v) {
                    render3dSettings.navigationArrowCheckButton = v
                    if (typeof NavigationArrowControlMenuController !== "undefined")
                        NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(v)
                }

                Row {
                    width: parent.width
                    height: Tokens.controlHMd
                    spacing: Tokens.spaceSm
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Size:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }
                    KSpinBox {
                        id: navigationArrowSizeSpinBox
                        anchors.verticalCenter: parent.verticalCenter
                        // Fixed reasonable width so single-cell rows don't
                        // stretch the spinbox across the entire body; value
                        // (1-5) fits comfortably with -/+ buttons + padding.
                        width: Math.round(140 * AppPalette.scale)
                        from: 1; to: 5; stepSize: 1; value: 1
                        onValueModified: function(v) {
                            if (typeof NavigationArrowControlMenuController !== "undefined")
                                NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(v)
                        }
                    }
                }
            }

            // Compass — toggle + animated Pos/Size row in same card.
            ParamCardGroup {
                id: compassCard
                label: qsTr("Compass")
                checked: render3dSettings.compassCheckButton
                onToggled: function(v) {
                    render3dSettings.compassCheckButton = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onCompassButtonChanged(v)
                }

                Row {
                    width: parent.width
                    height: Tokens.controlHMd
                    spacing: Tokens.spaceXs
                    readonly property real sw: (width - Tokens.spaceXs) / 2

                    Row {
                        width: parent.sw
                        height: parent.height
                        spacing: Tokens.spaceSm
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Pos:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        KSpinBox {
                            id: compassPosSpinBox
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 56
                            from: 1; to: 3; stepSize: 1; value: 2
                            onValueModified: function(v) {
                                if (typeof Scene3dToolBarController !== "undefined")
                                    Scene3dToolBarController.onCompassPosChanged(v)
                            }
                        }
                    }
                    Row {
                        width: parent.sw
                        height: parent.height
                        spacing: Tokens.spaceSm
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Size:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        KSpinBox {
                            id: compassSizeSpinBox
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 56
                            from: 1; to: 5; stepSize: 1; value: 1
                            onValueModified: function(v) {
                                if (typeof Scene3dToolBarController !== "undefined")
                                    Scene3dToolBarController.onCompassSizeChanged(v)
                            }
                        }
                    }
                }
            }

            Settings {
                id: render3dSettings
                property bool showQualityLabelCheck: false
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
            }
            Settings { property alias syncLoupeSize:        syncLoupeSizeSpinBox.value }
            Settings { property alias syncLoupeZoom:        syncLoupeZoomSpinBox.value }
            Settings { property alias circleGridSize:       circleGridSizeSpinBox.value }
            Settings { property alias circleGridStep:       circleGridStepSpinBox.value }
            Settings { property alias circleGridAngle:      circleGridAngleSpinBox.value }
            Settings { property alias navigationArrowSize:  navigationArrowSizeSpinBox.value }
            Settings { property alias compassPos:           compassPosSpinBox.value }
            Settings { property alias compassSize:          compassSizeSpinBox.value }

            Item { width: parent.width; height: Tokens.spaceMd }

            Text {
                width: parent.width
                text: qsTr("Map")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                topPadding: Tokens.spaceXs
            }

            // ── Visibility toggle ────────────────────────────────────────
            ParamCard {
                width: parent.width
                label: qsTr("Show map tiles")
                checked: mapVisibilitySettings.mapViewCheckButton
                onToggled: function(v) {
                    mapVisibilitySettings.mapViewCheckButton = v
                    if (typeof MapViewControlMenuController !== "undefined")
                        MapViewControlMenuController.onVisibilityChanged(v)
                    core.setMapTileLoadingEnabled(v)
                }
            }

            // Persisted under the legacy QSettings key so existing user
            // preferences carry over from the old in-3D-toolbar UI.
            Settings {
                id: mapVisibilitySettings
                property bool mapViewCheckButton: true
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
                text: qsTr("Providers")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                topPadding: Tokens.spaceXs
            }

            // ── Provider selector (single-pick rows) ─────────────────────
            Repeater {
                model: core.mapTileProviders
                delegate: Rectangle {
                    width: parent.width
                    implicitHeight: rowCol.implicitHeight + 2 * Tokens.spaceSm
                    height: implicitHeight
                    radius: Tokens.radiusMd

                    readonly property bool isSelected: modelData.id === core.mapTileProviderId
                    // Cached once per delegate (providers list is CONSTANT).
                    // Refreshed on click — see below.
                    property var dbInfo: core.getMapTileDbInfo(modelData.id)

                    color: isSelected
                           ? AppPalette.accentBg
                           : (providerMouse.containsMouse ? AppPalette.bgHover : AppPalette.bg)
                    border.width: 1
                    border.color: isSelected
                                  ? AppPalette.accentBorder
                                  : (providerMouse.containsMouse ? AppPalette.borderHover : AppPalette.border)
                    Behavior on color       { ColorAnimation { duration: 110 } }
                    Behavior on border.color { ColorAnimation { duration: 110 } }

                    Column {
                        id: rowCol
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        // ── Top line: name + layer type ────────────────
                        Item {
                            width: parent.width
                            height: nameLabel.implicitHeight

                            Text {
                                id: nameLabel
                                anchors.left: parent.left
                                anchors.right: typeLabel.left
                                anchors.rightMargin: Tokens.spaceMd
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.name
                                color: AppPalette.text
                                font.pixelSize: Tokens.fontMd
                                elide: Text.ElideRight
                            }
                            Text {
                                id: typeLabel
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.layer_type
                                color: AppPalette.textMuted
                                font.pixelSize: Tokens.fontSm
                            }
                        }

                        // ── Bottom line: cache age + size ──────────────
                        Text {
                            width: parent.width
                            text: {
                                if (!dbInfo || !dbInfo.exists)
                                    return qsTr("Cache: empty")
                                var mb = (dbInfo.sizeBytes / (1024 * 1024)).toFixed(1)
                                var iso = dbInfo.created || dbInfo.modified || ""
                                var d = new Date(iso)
                                var dateStr = isNaN(d.getTime())
                                              ? iso
                                              : d.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
                                return qsTr("Cache since %1  •  %2 MB").arg(dateStr).arg(mb)
                            }
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXs
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        id: providerMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            core.setMapTileProvider(modelData.id)
                            // Re-query in case selection just created a new
                            // empty DB on disk for this provider.
                            dbInfo = core.getMapTileDbInfo(modelData.id)
                        }
                    }
                }
            }

            Item { width: parent.width; height: Tokens.spaceMd }

            Text {
                width: parent.width
                text: qsTr("Navigator")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                topPadding: Tokens.spaceXs
            }

            ParamCard {
                width: parent.width
                label: qsTr("Use angle")
                checked: root.store.useAngleEnabled
                onToggled: function(v) {
                    root.store.useAngleEnabled = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onUseAngleLocationButtonChanged(v)
                }
            }

            ParamCard {
                width: parent.width
                label: qsTr("Navigator view")
                checked: root.store.navigationViewEnabled
                onToggled: function(v) {
                    root.store.navigationViewEnabled = v
                    if (typeof Scene3dToolBarController !== "undefined")
                        Scene3dToolBarController.onNavigatorLocationButtonChanged(v)
                }
            }

            Component.onCompleted: {
                if (typeof MapViewControlMenuController !== "undefined")
                    MapViewControlMenuController.onVisibilityChanged(mapVisibilitySettings.mapViewCheckButton)
                core.setMapTileLoadingEnabled(mapVisibilitySettings.mapViewCheckButton)
                if (typeof Scene3dToolBarController !== "undefined") {
                    Scene3dToolBarController.onUseAngleLocationButtonChanged(root.store.useAngleEnabled)
                    Scene3dToolBarController.onNavigatorLocationButtonChanged(root.store.navigationViewEnabled)
                    Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(root.store.trackLastDataEnabled)
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(render3dSettings.forceSingleZoomCheckButton)
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(render3dSettings.syncLoupeCheckButton)
                    Scene3dToolBarController.onSyncLoupeSizeChanged(syncLoupeSizeSpinBox.value)
                    Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(syncLoupeZoomSpinBox.value))
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
                }
                if (typeof NavigationArrowControlMenuController !== "undefined") {
                    NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(render3dSettings.navigationArrowCheckButton)
                    NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(navigationArrowSizeSpinBox.value)
                }
            }
        }
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

        property var exportFolderUrl: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property string exportFolderSource: ""

        Component.onCompleted: exportPathField.text = root.displayPath(exportFolderSource)

        function currentExportPath() {
            var t = exportPathField.text
            if (!t.length) return ""
            if (exportFolderSource.length && t === root.displayPath(exportFolderSource))
                return root.localPath(exportFolderSource)
            return t
        }

        // Path row
        Row {
            width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

            Rectangle {
                width: parent.width - Math.round(44 * AppPalette.scale) - Tokens.spaceMd
                height: Tokens.controlHMd
                radius: Tokens.radiusMd
                color: AppPalette.bg
                border.width: 1
                border.color: exportPathField.activeFocus ? AppPalette.accentBorder : AppPalette.border

                TextInput {
                    id: exportPathField
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.spaceMd
                    anchors.rightMargin: Tokens.spaceMd
                    TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: exportPathField.selectAll() }
                    verticalAlignment: TextInput.AlignVCenter
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontSm
                    clip: true

                    Text {
                        visible: !exportPathField.text.length
                        text: qsTr("Export path...")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            KButton {
                width: Math.round(44 * AppPalette.scale); height: Tokens.controlHMd; text: "..."
                onClicked: {
                    exportFolderDialog.currentFolder = exportGroup.exportFolderUrl
                    exportFolderDialog.open()
                }
            }

            FolderDialog {
                id: exportFolderDialog
                title: qsTr("Export folder")
                onAccepted: {
                    exportGroup.exportFolderUrl = exportFolderDialog.currentFolder
                    exportGroup.exportFolderSource = root.localPath(exportFolderDialog.selectedFolder)
                    exportPathField.text = root.displayPath(exportGroup.exportFolderSource)
                }
            }
        }

        Settings { property alias exportFolder:     exportGroup.exportFolderUrl }
        Settings { property alias exportFolderText: exportGroup.exportFolderSource }

        // Decimation + CSV
        ParamCard {
            id: exportDecimation
            label: qsTr("Decimation, m:")
            slotWidth: 2 * Math.round(93 * AppPalette.scale) + Tokens.spaceXs

            KSpinBox {
                id: exportDecimationValue
                width: Math.round(93 * AppPalette.scale)
                height: Tokens.controlHMd
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 100; stepSize: 1; value: 10
            }

            KButton {
                width: Math.round(93 * AppPalette.scale); height: Tokens.controlHMd
                anchors.left: exportDecimationValue.right
                anchors.leftMargin: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("CSV")
                onClicked: {
                    if (root.targetPlot)
                        core.exportPlotAsCVS(exportGroup.currentExportPath(), root.targetPlot.plotDatasetChannel(),
                                             exportDecimation.checked ? exportDecimationValue.value : 0)
                }
            }
        }

        Settings { property alias exportDecimation:      exportDecimation.checked }
        Settings { property alias exportDecimationValue: exportDecimationValue.value }

        KButton {
            width: parent.width; text: qsTr("Export to XTF")
            onClicked: core.exportPlotAsXTF(exportGroup.currentExportPath())
        }

        KButton {
            width: parent.width; text: qsTr("Complex signal to CSV")
            onClicked: core.exportComplexToCSV(exportGroup.currentExportPath())
        }

        KButton {
            width: parent.width; text: qsTr("USBL to CSV")
            onClicked: core.exportUSBLToCSV(exportGroup.currentExportPath())
        }
    }

    // ── Сохранение UI ─────────────────────────────────────────────────────────

    SettingsGroup {
        id: uiSavingGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("UI Saving")
        description: qsTr("Export the whole interface (layout, panels, all echogram/3D settings) to a JSON file, or import it from one.")
        stateStore: root.store
        stateKey: "app.uistate"
        collapsedByDefault: true

        property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        function safeFolder(folderUrl) {
            var lp = root.localPath(folderUrl)
            if (lp.length && uiStateSerializer && uiStateSerializer.pathExists(lp))
                return folderUrl
            return StandardPaths.writableLocation(StandardPaths.HomeLocation)
        }

        Row {
            width: parent.width
            spacing: Tokens.spaceMd

            KButton {
                width: (parent.width - Tokens.spaceMd) / 2
                height: Tokens.controlHMd
                text: qsTr("Export…")
                onClicked: {
                    uiExportDialog.currentFolder = uiSavingGroup.safeFolder(uiSavingGroup.exportFolder)
                    uiExportDialog.open()
                }
            }

            KButton {
                width: (parent.width - Tokens.spaceMd) / 2
                height: Tokens.controlHMd
                text: qsTr("Import…")
                onClicked: {
                    uiImportDialog.currentFolder = uiSavingGroup.safeFolder(uiSavingGroup.importFolder)
                    uiImportDialog.open()
                }
            }
        }

        FileDialog {
            id: uiExportDialog
            title: qsTr("Export UI state")
            fileMode: FileDialog.SaveFile
            nameFilters: ["JSON (*.json)", "All Files (*)"]
            onAccepted: {
                uiSavingGroup.exportFolder = uiExportDialog.currentFolder
                var path = root.localPath(uiExportDialog.selectedFile)
                if (!path.length) return
                if (!path.toLowerCase().endsWith(".json")) path += ".json"
                if (root.store) root.store.saveLayoutState()
                uiStateSerializer.exportToJsonFile(path)
            }
        }

        FileDialog {
            id: uiImportDialog
            title: qsTr("Import UI state")
            fileMode: FileDialog.OpenFile
            nameFilters: ["JSON (*.json)", "All Files (*)"]
            onAccepted: {
                uiSavingGroup.importFolder = uiImportDialog.currentFolder
                var path = root.localPath(uiImportDialog.selectedFile)
                if (!path.length) return
                if (uiStateSerializer.importFromJsonFile(path) && root.store)
                    root.store.reapplyImportedUiState()
            }
        }

        Settings { property alias uiStateExportFolder: uiSavingGroup.exportFolder }
        Settings { property alias uiStateImportFolder: uiSavingGroup.importFolder }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: text.length > 0
            text: uiStateSerializer ? (uiStateSerializer.lastError.length ? uiStateSerializer.lastError : uiStateSerializer.lastStatus) : ""
            color: uiStateSerializer && uiStateSerializer.lastError.length ? "#FF6B6B" : AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }
    }

    // ── Quick action menu ─────────────────────────────────────────────────────

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Quick action menu")
        description: qsTr("Quick-action menu contents: favorite layouts and connected device icons.")
        stateStore: root.store
        stateKey: "app.hotkeysWindow"

        KSwitch {
            width: parent.width; text: qsTr("Show favorite layouts")
            checked: root.store.quickActionFavoritesEnabled
            onToggled: {
                root.store.quickActionFavoritesEnabled = checked
                // Skip the reveal animation if there's nothing to flash.
                if (root.store.favoriteLayouts && root.store.favoriteLayouts.length > 0)
                    root.store.requestHotkeysReveal("layouts")
            }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show connected devices")
            checked: root.store.quickActionConnectionStatusEnabled
            onToggled: {
                root.store.quickActionConnectionStatusEnabled = checked
                if (!deviceManagerWrapper || !deviceManagerWrapper.devs)
                    return
                for (var i = 0; i < deviceManagerWrapper.devs.length; ++i) {
                    var d = deviceManagerWrapper.devs[i]
                    if (d && d.devType !== 0) {
                        root.store.requestHotkeysReveal("connections")
                        break
                    }
                }
            }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show bottom track editing")
            checked: root.store.quickActionBottomTrackEnabled
            onToggled: {
                root.store.quickActionBottomTrackEnabled = checked
                root.store.requestHotkeysReveal("bottomTrack")
            }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show profiles button")
            checked: root.store.quickActionProfilesEnabled
            onToggled: {
                root.store.quickActionProfilesEnabled = checked
                root.store.requestHotkeysReveal("profiles")
            }
        }

        KSwitch {
            width: parent.width; text: qsTr("Show extra info button")
            checked: root.store.quickActionExtraInfoEnabled
            onToggled: {
                root.store.quickActionExtraInfoEnabled = checked
                root.store.requestHotkeysReveal("extraInfo")
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
            Settings { property alias appDoubleTapDistancePx: tapTolerSlider.value }

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

            Settings { property alias appSplitHitSizePx: splitHitSlider.value }

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

            Settings { property alias appSidebarAnimMs: sidebarAnimSlider.value }

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

            Settings { property alias appWorkspaceAnimMs: workspaceAnimSlider.value }

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

        Column {
            id: footerCol
            anchors.top: parent.top
            anchors.topMargin: Tokens.spaceMd
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Tokens.spaceXs

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "qrc:/kogger_app_logo.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                width: Math.round(150 * AppPalette.scale)
                sourceSize.width: Math.round(360 * AppPalette.scale)
                opacity: 0.85
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.application.displayName
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
            }
        }
    }
}

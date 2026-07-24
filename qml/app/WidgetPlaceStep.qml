import QtQuick 2.15
import kqml_types 1.0

Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    readonly property real _scale: store ? store.widgetDraftScale : 1.0
    readonly property real _cell: Math.round(84 * AppPalette.appScale * _scale)
    readonly property real _gap: Math.round(4 * AppPalette.appScale * _scale)

    readonly property bool _touch: (typeof inputDeviceTracker !== "undefined" && inputDeviceTracker)
                                   ? inputDeviceTracker.touchMode : false
    on_TouchChanged: if (_touch && store) store.widgetDraftClearPreview()

    readonly property bool _anyUnplaced: {
        if (!store) return true
        var f = DataFieldCatalog.fields
        for (var i = 0; i < f.length; ++i)
            if (!store.widgetDraftIsPlaced(f[i].key)) return true
        return false
    }

    readonly property var _typeOptions: {
        var opts = [{ label: qsTr("Value"), value: "value" }]
        if (store && store.widgetDraftRepAvailable("labelValueRow"))
            opts.push({ label: qsTr("Label + Value"), value: "labelValueRow" })
        opts.push({ label: qsTr("Label / Value"), value: "labelValueStacked" })
        return opts
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Pick a representation, then drag widgets onto the panel shown in the working area. Drag a placed widget back here to remove it, or tap it to change its type.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        Text {
            text: qsTr("Representation")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
        }
        KTabBar {
            width: parent.width
            fontPixelSize: Tokens.fontLg
            trackColor: AppPalette.bgDeep
            options: step._typeOptions
            currentValue: step.store ? step.store.widgetDraftRep : "value"
            onValueSelected: function(value) { if (step.store) step.store.widgetDraftRep = value }
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        Text {
            text: qsTr("Cell size")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
        }
        KTabBar {
            width: parent.width
            fontPixelSize: Tokens.fontLg
            trackColor: AppPalette.bgDeep
            enabled: !!(step.store && step.store.widgetDraftBigAllowed)
            opacity: enabled ? 1 : 0.5
            options: [{ label: qsTr("Standard"), value: false }, { label: qsTr("Large"), value: true }]
            currentValue: !!(step.store && step.store.widgetDraftBig)
            onValueSelected: function(value) { if (step.store) step.store.widgetDraftSetBig(value) }
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        Text {
            text: qsTr("Background transparency")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
        }
        KSlider {
            width: parent.width
            from: 0
            to: 100
            stepSize: 1
            showValueTip: false
            value: step.store ? step.store.widgetDraftTransparency : 0
            valueSuffix: "%"
            onValueModified: function(v) { if (step.store) step.store.widgetDraftTransparency = Math.round(v) }
        }
    }

    KButton {
        id: saveBtn
        width: parent.width
        text: qsTr("Save")
        enabled: !!(step.store && step.store.widgetDraftCells.length > 0)
        normalBg: saveBtn.enabled ? AppPalette.accentBg : AppPalette.card
        hoverBg: saveBtn.enabled ? AppPalette.accentBg : AppPalette.card
        normalBorder: saveBtn.enabled ? AppPalette.accentBorder : AppPalette.border
        onClicked: if (step.store) step.store.widgetDraftSave()
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs

        Text {
            text: qsTr("Widgets")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Drag onto the panel, or tap to add.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }

        DropArea {
            id: paletteDrop
            width: parent.width
            height: (step._anyUnplaced ? paletteGrid.implicitHeight
                                       : Math.max(emptyLabel.implicitHeight, Math.round(84 * AppPalette.scale)))
                    + Tokens.spaceSm * 2

            readonly property bool _dragActive: !!(step.store && step.store.widgetDragActive)
            readonly property bool _over: !!(step.store && step.store.widgetOverPalette)

            onEntered: function(drag) {
                if (!step.store) return
                step.store.widgetOverPalette = true
                step.store.widgetDropRow = -1
                step.store.widgetDropCol = -1
            }
            onExited: if (step.store) step.store.widgetOverPalette = false

            Rectangle {
                anchors.fill: parent
                radius: Tokens.radiusMd
                color: paletteDrop._over
                       ? Qt.rgba(AppPalette.accentBorder.r, AppPalette.accentBorder.g, AppPalette.accentBorder.b, 0.12)
                       : AppPalette.bgDeep
                border.width: paletteDrop._dragActive ? Math.max(2, Math.round(2 * AppPalette.scale)) : 1
                border.color: paletteDrop._dragActive ? AppPalette.accentBorder : AppPalette.border
                Behavior on color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
                Behavior on border.width { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                Behavior on border.color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
            }

            Text {
                id: emptyLabel
                anchors.centerIn: parent
                width: parent.width - Tokens.spaceMd * 2
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: !step._anyUnplaced
                text: qsTr("All widgets are on the panel.")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
            }

            Grid {
                id: paletteGrid
                y: Tokens.spaceSm

                readonly property real _availW: paletteDrop.width - Tokens.spaceSm * 2
                readonly property var _sp: step.store ? step.store._cellSpan(step.store.widgetDraftRep, step.store.widgetDraftBig)
                                                      : { sc: 1, sr: 1 }
                readonly property real _pcell: Math.min(step._cell, (_availW - (_sp.sc - 1) * step._gap) / _sp.sc)
                readonly property real _tileW: _sp.sc * _pcell + (_sp.sc - 1) * step._gap
                readonly property real _tileH: _sp.sr * _pcell + (_sp.sr - 1) * step._gap
                readonly property real _k: _pcell / 84
                readonly property real _minGap: Tokens.spaceSm
                readonly property real _hgap: (_availW - columns * _tileW) / (columns + 1)

                columns: Math.max(1, Math.floor((_availW + _minGap) / (_tileW + _minGap)))
                x: Tokens.spaceSm + _hgap
                columnSpacing: _hgap
                rowSpacing: Tokens.spaceMd

                move: Transition {
                    NumberAnimation { properties: "x,y"; duration: 160; easing.type: Easing.OutCubic }
                }
                add: Transition {
                    NumberAnimation { properties: "x,y"; duration: 160; easing.type: Easing.OutCubic }
                }

                Repeater {
                    model: DataFieldCatalog.fields
                    delegate: Item {
                        id: tile
                        required property var modelData
                        readonly property bool placed: step.store ? step.store.widgetDraftIsPlaced(modelData.key) : false
                        property string fieldKey: modelData.key
                        property string representationType: step.store ? step.store.widgetDraftRep : "value"
                        property bool big: !!(step.store && step.store.widgetDraftBig)

                        width: paletteGrid._tileW
                        height: paletteGrid._tileH + caption.height + Tokens.spaceXs
                        visible: !placed || dragHandle.dragActive

                        Drag.active: dragHandle.dragActive
                        Drag.source: tile
                        Drag.hotSpot.x: dragHandle.x + dragHandle.width / 2
                        Drag.hotSpot.y: dragHandle.y + dragHandle.height / 2

                        readonly property bool _dragging: dragHandle.dragActive
                        on_DraggingChanged: {
                            if (!step.store) return
                            if (_dragging) step.store.widgetDragBegin(representationType, big, "")
                            else step.store.widgetDragEnd()
                        }

                        states: State {
                            when: dragHandle.dragActive
                            ParentChange { target: tile; parent: step.store ? step.store.widgetDragLayer : null }
                        }

                        Rectangle {
                            id: preview
                            width: paletteGrid._tileW
                            height: paletteGrid._tileH
                            radius: Tokens.radiusMd
                            readonly property bool _hot: dragHandle.dragActive || tileHover.hovered
                            color: dragHandle.dragActive ? AppPalette.accentBg : AppPalette.card
                            border.width: _hot ? Math.max(2, Math.round(2 * AppPalette.scale)) : 1
                            border.color: _hot ? AppPalette.accentBorder : AppPalette.border
                            Behavior on border.width { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                            Behavior on border.color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
                            Behavior on color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }

                            WidgetCellContent {
                                anchors.fill: parent
                                rep: tile.representationType
                                label: DataFieldCatalog.label(tile.fieldKey)
                                value: DataFieldCatalog.sampleValue(tile.fieldKey, step.store)
                                k: paletteGrid._k
                                gap: step._gap
                            }
                        }

                        Text {
                            id: caption
                            anchors.top: preview.bottom
                            anchors.topMargin: Tokens.spaceXs
                            width: paletteGrid._tileW
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            text: DataFieldCatalog.label(tile.fieldKey)
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontSm
                        }

                        MouseArea {
                            id: tileMouse
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (step.store) step.store.widgetDraftAutoPlace(tile.fieldKey, tile.representationType, tile.big)
                        }

                        WidgetDragHandle {
                            id: dragHandle
                            z: 5
                            anchors.top: preview.top
                            anchors.left: preview.left
                            anchors.margins: Math.round(3 * AppPalette.appScale)
                            diameter: Math.round(28 * AppPalette.appScale)
                            dragTarget: tile
                            onReleased: if (step.store) step.store.widgetDraftCommitFromPalette(tile.fieldKey, tile.representationType, tile.big)
                        }

                        HoverHandler {
                            id: tileHover
                            enabled: !dragHandle.dragActive && !step._touch
                            onHoveredChanged: {
                                if (!step.store) return
                                if (hovered && !tile.placed && !step._touch)
                                    step.store.widgetDraftPreviewFor(tile.fieldKey, tile.representationType, tile.big)
                                else if (step.store.widgetPreviewField === tile.fieldKey)
                                    step.store.widgetDraftClearPreview()
                            }
                        }
                    }
                }
            }
        }
    }
}

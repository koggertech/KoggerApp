import QtQuick 2.15
import kqml_types 1.0

Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    readonly property var _typeOptions: {
        var opts = [{ label: qsTr("Value"), value: "value" }]
        if (store && store.widgetDraftCols >= 2)
            opts.push({ label: qsTr("Label + Value"), value: "labelValueRow" })
        opts.push({ label: qsTr("Label / Value"), value: "labelValueStacked" })
        return opts
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Pick a representation, then drag fields onto the widget shown in the working area. Drag a placed field back here to remove it, or tap it to change its type.")
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
            text: qsTr("Fields")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
        }

        DropArea {
            id: paletteDrop
            width: parent.width
            height: paletteFlow.implicitHeight + Tokens.spaceSm * 2

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
                color: AppPalette.bgDeep
                border.width: 1
                border.color: AppPalette.border
            }

            Flow {
                id: paletteFlow
                x: Tokens.spaceSm
                y: Tokens.spaceSm
                width: parent.width - Tokens.spaceSm * 2
                spacing: Tokens.spaceSm

                Repeater {
                    model: DataFieldCatalog.fields
                    delegate: Rectangle {
                        id: chip
                        required property var modelData
                        readonly property bool placed: step.store ? step.store.widgetDraftIsPlaced(modelData.key) : false
                        property string fieldKey: modelData.key
                        property string representationType: step.store ? step.store.widgetDraftRep : "value"

                        width: chipLabel.implicitWidth + Tokens.spaceMd * 2
                        height: Tokens.controlHMd
                        radius: Tokens.radiusMd
                        visible: !placed || chipMouse.drag.active
                        color: chipMouse.drag.active ? AppPalette.accentBg : AppPalette.card
                        border.width: 1
                        border.color: chipMouse.drag.active ? AppPalette.accentBorder : AppPalette.border

                        Drag.active: chipMouse.drag.active
                        Drag.source: chip
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        states: State {
                            when: chipMouse.drag.active
                            ParentChange { target: chip; parent: step.store ? step.store.widgetDragLayer : null }
                        }

                        Text {
                            id: chipLabel
                            anchors.centerIn: parent
                            text: chip.modelData.label
                            color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontSm
                        }

                        MouseArea {
                            id: chipMouse
                            anchors.fill: parent
                            cursorShape: Qt.OpenHandCursor
                            drag.target: chip
                            preventStealing: true
                            onReleased: if (step.store) step.store.widgetDraftCommitFromPalette(chip.fieldKey, chip.representationType)
                        }
                    }
                }
            }
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
}
